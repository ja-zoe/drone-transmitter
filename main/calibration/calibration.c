#include "freertos/FreeRTOS.h"
#include <stdint.h>
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "calibration.h"
#include "drone_common.h"
#include "joysticks.h"
#include "portmacro.h"
#include <string.h>

// ---------------------------------------------------------------------------
// Joystick Calibration
// ---------------------------------------------------------------------------
//
// STATE_WAIT_LEFT:
//   Shows "MOVE LEFT STICK" + hint subtext. The timer polls throttle and yaw
//   every 100 ms. As soon as either exceeds CALIB_START_THRESHOLD from centre,
//   we advance to STATE_COUNT_LEFT.
//
// STATE_COUNT_LEFT:
//   Hides subtext, shows spinner + countdown digit centred inside it. Records
//   throttle/yaw min/max for CALIB_RECORD_MS ms, then advances to
//   STATE_WAIT_RIGHT.
//
// STATE_WAIT_RIGHT:
//   Same as WAIT_LEFT but polls pitch and roll instead.
//
// STATE_COUNT_RIGHT:
//   Same recording behaviour as COUNT_LEFT but for pitch/roll. On completion
//   calls finish_calibration(), frees the context, and deletes the timer.
//
// Timer period: 100 ms — fast enough for the spinner to animate smoothly and
// for the countdown label to update within one tick of each second boundary.
//
// All state, calibration data, and UI handles live in a heap-allocated
// calib_ctx_t passed through lv_timer_t.user_data. No file-scope UI globals.
// ---------------------------------------------------------------------------

#define CALIB_START_THRESHOLD 500   // ADC counts from centre (12-bit, centre = 2048)
#define CALIB_RECORD_MS       3000  // recording window per stick, in ms

#define RAW_APPROX_MIN 5600
#define RAW_APPROX_MAX 21000

static const char *TAG = "Calibration_Module";

void countdown(lv_timer_t * timer) {
  lv_obj_t *count = lv_timer_get_user_data(timer);
  char * numberText = lv_label_get_text(count);
  if (strcmp(numberText, "3") == 0) {
    if (lvgl_port_lock(portMAX_DELAY)) {
      lv_label_set_text(count, "2");
      lvgl_port_unlock();
    }
  } else if (strcmp(numberText, "2") == 0) {
    if (lvgl_port_lock(portMAX_DELAY)) {
      lv_label_set_text(count, "1");
      lvgl_port_unlock();
    }
  } else {
    lv_timer_pause(timer);
  }
};

// ---- public entry point ----------------------------------------------------

void start_calibration_sequence(void)
{
    joystick_cal_t cal = {
        .throttle = { 32767, -32768 },
        .yaw      = { 32767, -32768 },
        .pitch    = { 32767, -32768 },
        .roll     = { 32767, -32768 },
    };

    // ---- Build the UI ----
    if (!lvgl_port_lock(portMAX_DELAY)) return;
    lv_obj_clean(lv_screen_active());

    lv_obj_t *header = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(header, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(header, lv_color_white(), 0);
    lv_label_set_text(header, "MOVE LEFT STICK");
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_t *subtext = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(subtext, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(subtext, lv_color_white(), 0);
    lv_obj_set_style_text_align(subtext, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(subtext, "Rotate around its\nedge for 3 seconds");
    lv_obj_align(subtext, LV_ALIGN_CENTER, 0, 10);

    lv_obj_t *spinner = lv_spinner_create(lv_screen_active());
    lv_obj_set_size(spinner, 40, 40);
    lv_obj_center(spinner);
    lv_obj_set_style_arc_color(spinner, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 2, LV_PART_MAIN);
    lv_obj_set_style_arc_color(spinner, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spinner, 2, LV_PART_INDICATOR);
    lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *count = lv_label_create(spinner);
    lv_obj_set_style_text_font(count, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(count, lv_color_white(), 0);
    lv_obj_center(count);
    lv_label_set_text(count, "3");
    lv_obj_add_flag(count, LV_OBJ_FLAG_HIDDEN);

    lvgl_port_unlock();
    
    joysticks_values_t js;
    // ---- Phase 1: wait for left stick movement ----
    while (true) {
        get_joysticks_raw(&js);
        if (js.throttle < RAW_APPROX_MIN + CALIB_START_THRESHOLD ||
            js.throttle > RAW_APPROX_MAX - CALIB_START_THRESHOLD)
            break;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // ---- Phase 2: record left stick for 3 seconds ----
    if (lvgl_port_lock(portMAX_DELAY)) {
        lv_obj_add_flag(header, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(subtext, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(spinner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(count, LV_OBJ_FLAG_HIDDEN);
        lvgl_port_unlock();
    }

    lv_timer_t *timer = lv_timer_create(countdown, 1000, count);

    while (lv_timer_get_paused(timer) != true) {
      get_joysticks_raw(&js);
      if (js.throttle < cal.throttle.min) cal.throttle.min = js.throttle;
      if (js.throttle > cal.throttle.max) cal.throttle.max = js.throttle;
      if (js.yaw      < cal.yaw.min)      cal.yaw.min      = js.yaw;
      if (js.yaw      > cal.yaw.max)      cal.yaw.max      = js.yaw;
      vTaskDelay(pdMS_TO_TICKS(10));
    }

    // ---- Phase 3: wait for right stick movement ----
    if (lvgl_port_lock(portMAX_DELAY)) {
        lv_label_set_text(header, "MOVE RIGHT STICK");
        lv_obj_clear_flag(header, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(count, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(subtext, LV_OBJ_FLAG_HIDDEN);
        lvgl_port_unlock();
    }

    while (true) {
        get_joysticks_raw(&js);
        if (js.pitch < RAW_APPROX_MIN + CALIB_START_THRESHOLD ||
            js.pitch > RAW_APPROX_MAX - CALIB_START_THRESHOLD)
            break;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // ---- Phase 4: record right stick for 3 seconds ----
    if (lvgl_port_lock(portMAX_DELAY)) {
        lv_obj_add_flag(header, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(subtext, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(spinner, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(count, "3");
        lv_obj_clear_flag(count, LV_OBJ_FLAG_HIDDEN);
        lvgl_port_unlock();
    }

    lv_timer_reset(timer);
    lv_timer_resume(timer);

    while (lv_timer_get_paused(timer) != true) {
      get_joysticks_raw(&js);
      if (js.roll < cal.roll.min) cal.roll.min = js.roll;
      if (js.roll > cal.roll.max) cal.roll.max = js.roll;
      if (js.pitch < cal.pitch.min) cal.pitch.min = js.pitch;
      if (js.pitch > cal.pitch.max) cal.pitch.max = js.pitch;
      vTaskDelay(pdMS_TO_TICKS(10));
    }

    // ---- Done ----
    if (lvgl_port_lock(portMAX_DELAY)) {
        lv_label_set_text(header, "CALIBRATION DONE");
        lv_obj_clear_flag(header, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(count, LV_OBJ_FLAG_HIDDEN);
        lvgl_port_unlock();
    }

    set_calibration_values(&cal);
    // finish_calibration(&cal);
    vTaskDelay(pdMS_TO_TICKS(500)); // brief pause so user sees "DONE"
}