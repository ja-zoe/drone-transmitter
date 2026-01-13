idf_component_register(SRCS "drone-transmitter.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver joysticks switches nvs_flash esp_event esp_netif esp_wifi esp_driver_dac
                    )
