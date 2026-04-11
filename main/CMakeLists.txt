idf_component_register(SRCS "main.c" "tasks/input_read_task.c" "tasks/oled_update_task.c" "tasks/transmit_task.c"
                    INCLUDE_DIRS "." "tasks"
                    REQUIRES driver joysticks switches nvs_flash esp_event esp_netif esp_wifi esp_driver_dac
                    )
