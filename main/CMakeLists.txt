idf_component_register(SRCS "main.c" "tasks/input_read_task.c" "tasks/oled_update_task.c" "tasks/transmit_task.c" "tasks/receive_task.c" "espnow/espnow.c" "setup/setup.c" "calibration/calibration.c"
                    INCLUDE_DIRS "." "tasks" "espnow" "setup" "calibration"
                    REQUIRES driver joysticks switches drone_common nvs_flash esp_event esp_netif esp_wifi esp_driver_dac
                    )
