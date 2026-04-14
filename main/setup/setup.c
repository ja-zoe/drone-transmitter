#include "setup.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_lvgl_port.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_vendor.h"
#include "lvgl.h"

i2c_master_bus_handle_t i2c_bus;

void nvs_init(void) {
  esp_err_t ret;
  /*---- Initialize NVS for persistent wifi credentials ----*/ 
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK( nvs_flash_erase() );
      ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
}

void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start());

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif
}

void i2c_init(void) {
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .i2c_port = I2C_PORT_CONF,
        .sda_io_num = SDA_GPIO_CONF,
        .scl_io_num = SCL_GPIO_CONF,
        .flags.enable_internal_pullup = false,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));
};

void oled_init(void) {
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = OLED_I2C_ADDR_CONF,
        .scl_speed_hz = OLED_FREQ_CONF,
        .control_phase_bytes = 1, // According to SSD1306 datasheet
        .lcd_cmd_bits = 8,        // According to SSD1306 datasheet
        .lcd_param_bits = 8,      // According to SSD1306 datasheet
        .dc_bit_offset = 6,       // According to SSD1306 datasheet
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle));


    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = -1,
    };

    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = OLED_V_RES_CONF,
    };
    panel_config.vendor_config = &ssd1306_config;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = OLED_H_RES_CONF * OLED_V_RES_CONF,
        .double_buffer = true,
        .hres = OLED_H_RES_CONF,
        .vres = OLED_V_RES_CONF,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
          .sw_rotate = false,
        }
    };
    
    lv_disp_t *disp = lvgl_port_add_disp(&disp_cfg);

    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_port_lock(0)) {
        /* Rotation of the screen */
        lv_disp_set_rotation(disp, LV_DISPLAY_ROTATION_180); //
        
        lv_obj_t *scr = lv_disp_get_scr_act(disp);
        lv_obj_t *label = lv_label_create(scr);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR); /* Circular scroll */
        lv_label_set_text(label, "Hello Espressif, Hello LVGL.");
        lv_obj_set_width(label, OLED_H_RES_CONF);
        lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

        // Release the mutex
        lvgl_port_unlock();
    }
};