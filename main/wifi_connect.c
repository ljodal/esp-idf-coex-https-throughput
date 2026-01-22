#include "wifi_connect.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "sdkconfig.h"

static const char *TAG = "wifi_connect";

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  SemaphoreHandle_t sem = (SemaphoreHandle_t)arg;

  if (event_base == WIFI_EVENT) {
    if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
      ESP_LOGI(TAG, "Disconnected, reconnecting...");
      esp_wifi_connect();
    }
  } else if (event_base == IP_EVENT) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
      ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
      ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
      xSemaphoreGive(sem);
    }
  }
}

esp_err_t wifi_connect(void) {
  SemaphoreHandle_t sem = xSemaphoreCreateBinary();
  if (sem == NULL) {
    return ESP_ERR_NO_MEM;
  }

  /* Initialize WiFi */
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  esp_netif_create_default_wifi_sta();

  /* Set country code BEFORE start for proper regulatory settings */
  ESP_LOGI(TAG, "Setting WiFi country: %s", CONFIG_SPEEDTEST_WIFI_COUNTRY);
  ESP_ERROR_CHECK(
      esp_wifi_set_country_code(CONFIG_SPEEDTEST_WIFI_COUNTRY, true));

  /* Configure station mode */
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = CONFIG_SPEEDTEST_WIFI_SSID,
              .password = CONFIG_SPEEDTEST_WIFI_PASSWORD,
          },
  };
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

  /* Register event handlers */
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &wifi_event_handler, sem));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &wifi_event_handler, sem));

  /* Start and connect */
  ESP_LOGI(TAG, "Connecting to %s...", CONFIG_SPEEDTEST_WIFI_SSID);
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_connect());

  /* Wait for IP */
  xSemaphoreTake(sem, portMAX_DELAY);
  vSemaphoreDelete(sem);

  return ESP_OK;
}
