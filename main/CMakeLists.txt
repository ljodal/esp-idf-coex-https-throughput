idf_component_register(SRCS "main.c" "time_sync.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp-tls esp_wifi esp_timer nvs_flash bt esp_coex)
