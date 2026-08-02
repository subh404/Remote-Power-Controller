idf_component_register(SRCS "main.c" 
                    "mfg_data.c"
                    "provision_data.c"
                    "app_wifi.c" 
                    "app_mqtt.c"
                    "app_usb_hid.c"
                    "app_usb_msc.c"
                    "app_usb.c"
                    "app_fs.c"
                    "app_event_loop.c"
                    "app_pwr_sw.c"
                    INCLUDE_DIRS "." 
                    PRIV_REQUIRES  esp_wifi 
                    mqtt 
                    nvs_flash 
                    spi_flash
                    esp_driver_gpio
                    vfs fatfs
                    )
