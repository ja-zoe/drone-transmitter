idf_component_register(SRCS "switches.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_driver_gpio
                    REQUIRES drone-software-common main)
