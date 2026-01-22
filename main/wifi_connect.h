#pragma once

#include <esp_err.h>

/**
 * Initialize WiFi and connect to the configured AP.
 * Sets country code before starting WiFi for proper regulatory settings.
 */
esp_err_t wifi_connect(void);
