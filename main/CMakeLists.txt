idf_component_register(SRCS "main.c" "tasks/input_read_task.c" "tasks/oled_update_task.c" "tasks/transmit_task.c" "tasks/receive_task.c" "espnow/espnow.c" "config/config.c" "calibration/calibration.c"
                    INCLUDE_DIRS "." "tasks" "espnow" "config" "calibration"
                    REQUIRES driver joysticks switches drone-software-common nvs_flash esp_event esp_netif esp_wifi esp_driver_dac
                    )
