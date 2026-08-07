idf_component_register(SRCS "eth5500.c"
                       PRIV_REQUIRES esp_driver_gpio esp_eth nvs_flash esp_system 
                       esp_netif esp_eth w5500
                       INCLUDE_DIRS ".")
