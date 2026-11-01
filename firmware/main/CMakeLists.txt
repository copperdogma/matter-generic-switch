idf_component_register(SRCS              "app_main.cpp" "app_reset.cpp" "drivers/led_indicator.c"
                       INCLUDE_DIRS      "." "drivers/include"
                       PRIV_INCLUDE_DIRS "." 
                                       "drivers/include" 
                       REQUIRES espressif__esp_matter espressif__button esp_driver_ledc esp_driver_gpio esp_timer esp_wifi esp_netif nvs_flash console
                       )

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
