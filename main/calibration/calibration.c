#include <stdint.h>
#include "lvgl.h"
#include "esp_log.h"
#include "calibration.h"

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
#define CALIB_SPINNER_SIZE    40    // spinner diameter in px; countdown box matches

typedef enum {
    STATE_WAIT_LEFT,
    STATE_COUNT_LEFT,
    STATE_WAIT_RIGHT,
    STATE_COUNT_RIGHT,
} calib_step_t;

typedef struct {
    int16_t min;
    int16_t max;
} axis_cal_t;

typedef struct {
    axis_cal_t throttle;
    axis_cal_t yaw;
    axis_cal_t pitch;
    axis_cal_t roll;
} joystick_cal_t;

typedef struct {
    calib_step_t   step;
    joystick_cal_t cal;
    uint32_t       record_start_ms;
    int8_t         last_shown_sec;
    lv_obj_t      *header;
    lv_obj_t      *subtext;
    lv_obj_t      *spinner;
    lv_obj_t      *count;
} calib_ctx_t;

extern control_packet_t control_packet;
extern SemaphoreHandle_t controlPacketMutexHandle;

// ---- helpers ---------------------------------------------------------------

// Returns remaining whole seconds (rounds up). Returns 0 when time is up.
// The +999 ensures the label shows "3" for the entire first second rather
// than jumping straight to "2" on the next 100 ms tick.
static int8_t recording_secs_left(calib_ctx_t *ctx)
{
    uint32_t elapsed = lv_tick_get() - ctx->record_start_ms;
    if (elapsed >= CALIB_RECORD_MS) return 0;
    return (int8_t)((CALIB_RECORD_MS - elapsed + 999) / 1000);
}

// Call when entering a WAIT state. Hides spinner/count, shows the hint.
static void enter_wait_state(calib_ctx_t *ctx, const char *header_text)
{
    lv_label_set_text(ctx->header, header_text);
    lv_obj_add_flag(ctx->spinner,   LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ctx->count,     LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ctx->subtext, LV_OBJ_FLAG_HIDDEN);
}

// Call when entering a COUNT state. Hides the hint, shows spinner + count.
static void enter_record_state(calib_ctx_t *ctx, const char *header_text)
{
    lv_label_set_text(ctx->header, header_text);
    ctx->record_start_ms = lv_tick_get();
    ctx->last_shown_sec  = 3;
    lv_label_set_text(ctx->count, "3");
    lv_obj_add_flag(ctx->subtext,   LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ctx->spinner, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ctx->count,   LV_OBJ_FLAG_HIDDEN);
}

// ---- timer callback (fires every 100 ms) -----------------------------------

static void calib_timer_cb(lv_timer_t *timer)
{
    calib_ctx_t *ctx = (calib_ctx_t *)timer->user_data;

    if (!lvgl_port_lock(0)) return;

    // Snapshot joystick values under the data mutex
    joysticks_values_t js = {0};
    if (xSemaphoreTake(controlPacketMutexHandle, 0) == pdTRUE) {
        js = control_packet.joysticks_values;
        xSemaphoreGive(controlPacketMutexHandle);
    }

    switch (ctx->step) {

        // Poll throttle and yaw. Stay here until one of them is moved past
        // the threshold, then kick off the left-stick recording phase.
        case STATE_WAIT_LEFT:
            if (abs(js.throttle - 2048) > CALIB_START_THRESHOLD ||
                abs(js.yaw      - 2048) > CALIB_START_THRESHOLD)
            {
                ctx->step = STATE_COUNT_LEFT;
                enter_record_state(ctx, "RECORDING LEFT...");
            }
            break;

        // Record throttle + yaw min/max. Update the countdown each second.
        // When the window expires, go wait for the right stick.
        case STATE_COUNT_LEFT:
            if (js.throttle < ctx->cal.throttle.min) ctx->cal.throttle.min = js.throttle;
            if (js.throttle > ctx->cal.throttle.max) ctx->cal.throttle.max = js.throttle;
            if (js.yaw      < ctx->cal.yaw.min)      ctx->cal.yaw.min      = js.yaw;
            if (js.yaw      > ctx->cal.yaw.max)      ctx->cal.yaw.max      = js.yaw;
            {
                int8_t secs = recording_secs_left(ctx);
                if (secs != ctx->last_shown_sec) {
                    ctx->last_shown_sec = secs;
                    lv_label_set_text_fmt(ctx->count, "%d", secs);
                }
                if (secs <= 0) {
                    ctx->step = STATE_WAIT_RIGHT;
                    enter_wait_state(ctx, "MOVE RIGHT STICK");
                }
            }
            break;

        // Poll pitch and roll. Stay here until one is moved past the
        // threshold, then kick off the right-stick recording phase.
        case STATE_WAIT_RIGHT:
            if (abs(js.pitch - 2048) > CALIB_START_THRESHOLD ||
                abs(js.roll  - 2048) > CALIB_START_THRESHOLD)
            {
                ctx->step = STATE_COUNT_RIGHT;
                enter_record_state(ctx, "RECORDING RIGHT...");
            }
            break;

        // Record pitch + roll min/max. When the window expires, finish.
        case STATE_COUNT_RIGHT:
            if (js.pitch < ctx->cal.pitch.min) ctx->cal.pitch.min = js.pitch;
            if (js.pitch > ctx->cal.pitch.max) ctx->cal.pitch.max = js.pitch;
            if (js.roll  < ctx->cal.roll.min)  ctx->cal.roll.min  = js.roll;
            if (js.roll  > ctx->cal.roll.max)  ctx->cal.roll.max  = js.roll;
            {
                int8_t secs = recording_secs_left(ctx);
                if (secs != ctx->last_shown_sec) {
                    ctx->last_shown_sec = secs;
                    lv_label_set_text_fmt(ctx->count, "%d", secs);
                }
                if (secs <= 0) {
                    lv_label_set_text(ctx->header, "CALIBRATION DONE");
                    lv_obj_add_flag(ctx->spinner, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(ctx->count,   LV_OBJ_FLAG_HIDDEN);
                    lvgl_port_unlock();
                    finish_calibration();
                    free(ctx);
                    lv_timer_del(timer);
                    return; // timer is deleted — must not touch it after this
                }
            }
            break;
    }

    lvgl_port_unlock();
}

// ---- public entry point ----------------------------------------------------

void start_calibration_sequence(void)
{
    calib_ctx_t *ctx = malloc(sizeof(calib_ctx_t));
    if (!ctx) {
        ESP_LOGE(TAG, "Failed to allocate calibration context");
        return;
    }

    *ctx = (calib_ctx_t){
        .step            = STATE_WAIT_LEFT,
        .record_start_ms = 0,
        .last_shown_sec  = -1,
        .cal = {
            .throttle = { 32767, -32768 },
            .yaw      = { 32767, -32768 },
            .pitch    = { 32767, -32768 },
            .roll     = { 32767, -32768 },
        },
    };

    if (!lvgl_port_lock(0)) {
        free(ctx);
        return;
    }

    lv_obj_clean(lv_screen_active());

    // Header — pinned to the top, shows which stick to move or "RECORDING..."
    ctx->header = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(ctx->header, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ctx->header, lv_color_white(), 0);
    lv_label_set_text(ctx->header, "MOVE LEFT STICK");
    lv_obj_align(ctx->header, LV_ALIGN_TOP_MID, 0, 5);

    // Subtext — centred, visible only during WAIT states
    ctx->subtext = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(ctx->subtext, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(ctx->subtext, lv_color_white(), 0);
    lv_obj_set_style_text_align(ctx->subtext, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(ctx->subtext, "Rotate around its\nedge for 3 seconds");
    lv_obj_align(ctx->subtext, LV_ALIGN_CENTER, 0, 0);

    // Spinner — thin arc ring, screen-centred, hidden until recording starts
    ctx->spinner = lv_spinner_create(lv_screen_active());
    lv_obj_set_size(ctx->spinner, CALIB_SPINNER_SIZE, CALIB_SPINNER_SIZE);
    lv_obj_center(ctx->spinner);
    lv_obj_set_style_arc_color(ctx->spinner, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_arc_width(ctx->spinner, 2, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ctx->spinner, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctx->spinner, 2, LV_PART_INDICATOR);
    lv_obj_add_flag(ctx->spinner, LV_OBJ_FLAG_HIDDEN);

    // Countdown — fixed box matching the spinner size so the digit stays
    // visually centred as its width changes between "3", "2", "1".
    // LV_LABEL_LONG_CLIP prevents the label from auto-resizing to content.
    ctx->count = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(ctx->count, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ctx->count, lv_color_white(), 0);
    lv_obj_set_style_text_align(ctx->count, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(ctx->count, LV_LABEL_LONG_CLIP);
    lv_obj_set_size(ctx->count, CALIB_SPINNER_SIZE, CALIB_SPINNER_SIZE);
    lv_obj_center(ctx->count);
    lv_label_set_text(ctx->count, "3");
    lv_obj_add_flag(ctx->count, LV_OBJ_FLAG_HIDDEN);

    lvgl_port_unlock();

    lv_timer_t *timer = lv_timer_create(calib_timer_cb, 100, NULL);
    timer->user_data = ctx;
}