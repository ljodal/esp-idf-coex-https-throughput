/*
 * HTTPS Speed Test with WiFi/BLE Coexistence Testing
 *
 * Downloads from Cloudflare speed test server and measures throughput
 * under various configurations (TLS version, BLE, power save, coex settings).
 *
 * SPDX-FileCopyrightText: The Mbed TLS Contributors
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileContributor: 2015-2025 Espressif Systems (Shanghai) CO LTD
 */

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_tls.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "sdkconfig.h"
#include "time_sync.h"

#ifdef CONFIG_BT_ENABLED
#include "esp_coexist.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#endif

static const char *TAG = "speedtest";

/* Parsed URL components (filled at runtime) */
static char speed_test_host[128];
static char speed_test_path[256];
static char speed_test_request[512];

/* Parse URL into host and path components */
static void parse_url(const char *url) {
  const char *host_start = strstr(url, "://");
  if (!host_start) {
    ESP_LOGE(TAG, "Invalid URL: %s", url);
    return;
  }
  host_start += 3; /* Skip :// */

  const char *path_start = strchr(host_start, '/');
  if (path_start) {
    size_t host_len = path_start - host_start;
    if (host_len >= sizeof(speed_test_host))
      host_len = sizeof(speed_test_host) - 1;
    strncpy(speed_test_host, host_start, host_len);
    speed_test_host[host_len] = '\0';
    strncpy(speed_test_path, path_start, sizeof(speed_test_path) - 1);
  } else {
    strncpy(speed_test_host, host_start, sizeof(speed_test_host) - 1);
    strcpy(speed_test_path, "/");
  }

  /* Build HTTP request */
  snprintf(speed_test_request, sizeof(speed_test_request),
           "GET %s HTTP/1.1\r\n"
           "Host: %s\r\n"
           "User-Agent: esp-idf/1.0 esp32\r\n"
           "Connection: close\r\n"
           "\r\n",
           speed_test_path, speed_test_host);

  ESP_LOGI(TAG, "Speed test URL: %s", url);
  ESP_LOGI(TAG, "Host: %s, Path: %s", speed_test_host, speed_test_path);
}

/* TLS 1.3 ciphersuites to force TLS 1.3 */
static const int tls13_ciphersuites[] = {
    MBEDTLS_TLS1_3_AES_128_GCM_SHA256, MBEDTLS_TLS1_3_AES_256_GCM_SHA384,
    MBEDTLS_TLS1_3_CHACHA20_POLY1305_SHA256, 0};

/*
 * Test configuration
 */
typedef enum {
  TLS_VERSION_1_2,
  TLS_VERSION_1_3,
} tls_version_t;

typedef struct {
  const char *name;
  tls_version_t tls_version;
  bool ble_enabled;
  bool wifi_ps_enabled;
  esp_coex_prefer_t coex_pref;
  uint16_t ble_adv_interval_ms; /* BLE advertising interval in ms */
} test_config_t;

typedef struct {
  const char *name;
  bool success;
  size_t bytes_downloaded;
  int64_t duration_ms;
  float speed_kbps;
  float speed_mbps;
} test_result_t;

#define MAX_TESTS 16
static test_result_t results[MAX_TESTS];
static int num_results = 0;

/*
 * BLE advertising control
 */
#ifdef CONFIG_BT_ENABLED
static bool ble_initialized = false;
static bool ble_advertising = false;
static uint16_t current_adv_interval_ms = 100;

static void ble_host_task(void *param) {
  nimble_port_run();
  nimble_port_freertos_deinit();
}

static void start_ble_advertising(uint16_t interval_ms) {
  ble_addr_t addr;
  ble_hs_id_gen_rnd(1, &addr);
  ble_hs_id_set_rnd(addr.val);

  /* Convert ms to BLE units (0.625ms per unit) */
  uint16_t interval_units = (interval_ms * 1000) / 625;
  if (interval_units < 32)
    interval_units = 32; /* Min 20ms */
  if (interval_units > 16384)
    interval_units = 16384; /* Max ~10s */

  struct ble_gap_adv_params adv_params = {
      .conn_mode = BLE_GAP_CONN_MODE_UND,
      .disc_mode = BLE_GAP_DISC_MODE_GEN,
      .itvl_min = interval_units,
      .itvl_max = interval_units,
  };

  struct ble_hs_adv_fields fields = {
      .flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP,
      .name = (uint8_t *)"ESP32-SpeedTest",
      .name_len = 15,
      .name_is_complete = 1,
  };

  ble_gap_adv_set_fields(&fields);
  ble_gap_adv_start(BLE_OWN_ADDR_RANDOM, NULL, BLE_HS_FOREVER, &adv_params,
                    NULL, NULL);
  ble_advertising = true;
  current_adv_interval_ms = interval_ms;
  ESP_LOGI(TAG, "BLE advertising started (interval: %d ms)", interval_ms);
}

static void ble_on_sync(void) {
  start_ble_advertising(current_adv_interval_ms);
}

static void start_ble(uint16_t interval_ms) {
  current_adv_interval_ms = interval_ms;

  if (ble_initialized) {
    /* Already initialized, restart advertising with new interval */
    if (ble_advertising) {
      ble_gap_adv_stop();
      ble_advertising = false;
    }
    start_ble_advertising(interval_ms);
    return;
  }

  ESP_LOGI(TAG, "Initializing NimBLE...");
  esp_err_t ret = nimble_port_init();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to init NimBLE: %d", ret);
    return;
  }

  ble_svc_gap_device_name_set("ESP32-SpeedTest");
  ble_hs_cfg.sync_cb = ble_on_sync;
  nimble_port_freertos_init(ble_host_task);
  ble_initialized = true;
}

static void stop_ble(void) {
  if (ble_advertising) {
    ble_gap_adv_stop();
    ble_advertising = false;
    ESP_LOGI(TAG, "BLE advertising stopped");
  }
}
#else
static void start_ble(uint16_t interval_ms) {
  (void)interval_ms;
  ESP_LOGW(TAG, "BLE not enabled in config");
}
static void stop_ble(void) {}
#endif

/*
 * Apply test configuration
 */
static void apply_config(const test_config_t *config) {
  ESP_LOGI(TAG, "Applying config: %s", config->name);

  /* WiFi power save */
  wifi_ps_type_t ps_type =
      config->wifi_ps_enabled ? WIFI_PS_MIN_MODEM : WIFI_PS_NONE;
  esp_wifi_set_ps(ps_type);
  ESP_LOGI(TAG, "  WiFi PS: %s",
           config->wifi_ps_enabled ? "enabled" : "disabled");

  /* BLE */
  if (config->ble_enabled) {
    start_ble(config->ble_adv_interval_ms);
#ifdef CONFIG_BT_ENABLED
    esp_coex_preference_set(config->coex_pref);
    const char *coex_str = "unknown";
    switch (config->coex_pref) {
    case ESP_COEX_PREFER_WIFI:
      coex_str = "WiFi";
      break;
    case ESP_COEX_PREFER_BT:
      coex_str = "BT";
      break;
    case ESP_COEX_PREFER_BALANCE:
      coex_str = "Balance";
      break;
    default:
      break;
    }
    ESP_LOGI(TAG, "  Coex preference: %s", coex_str);
    ESP_LOGI(TAG, "  BLE adv interval: %d ms", config->ble_adv_interval_ms);
#endif
  } else {
    stop_ble();
  }

  ESP_LOGI(TAG, "  TLS version: %s",
           config->tls_version == TLS_VERSION_1_3 ? "1.3" : "1.2");

  /* Small delay to let settings take effect */
  vTaskDelay(pdMS_TO_TICKS(100));
}

/*
 * Run speed test with given TLS version
 */
static test_result_t run_speed_test(const test_config_t *config) {
  test_result_t result = {
      .name = config->name,
      .success = false,
      .bytes_downloaded = 0,
      .duration_ms = 0,
      .speed_kbps = 0,
      .speed_mbps = 0,
  };

  char buf[4096];
  int ret;
  size_t total_bytes = 0;
  int64_t start_time_us = 0;
  bool headers_done = false;

  ESP_LOGI(TAG, "----------------------------------------");
  ESP_LOGI(TAG, "Running: %s", config->name);
  ESP_LOGI(TAG, "----------------------------------------");

  /* Configure TLS */
  esp_tls_cfg_t cfg = {
      .crt_bundle_attach = esp_crt_bundle_attach,
  };

  if (config->tls_version == TLS_VERSION_1_3) {
    cfg.ciphersuites_list = tls13_ciphersuites;
  }

  esp_tls_t *tls = esp_tls_init();
  if (!tls) {
    ESP_LOGE(TAG, "Failed to allocate esp_tls handle!");
    return result;
  }

  int64_t connect_start = esp_timer_get_time();
  if (esp_tls_conn_http_new_sync(CONFIG_SPEEDTEST_URL, &cfg, tls) != 1) {
    ESP_LOGE(TAG, "Connection failed!");
    esp_tls_conn_destroy(tls);
    return result;
  }
  int64_t connect_ms = (esp_timer_get_time() - connect_start) / 1000;
  ESP_LOGI(TAG, "Connected in %" PRId64 " ms", connect_ms);

  /* Send request */
  size_t written = 0;
  size_t request_len = strlen(speed_test_request);
  while (written < request_len) {
    ret = esp_tls_conn_write(tls, speed_test_request + written,
                             request_len - written);
    if (ret > 0) {
      written += ret;
    } else if (ret != ESP_TLS_ERR_SSL_WANT_READ &&
               ret != ESP_TLS_ERR_SSL_WANT_WRITE) {
      ESP_LOGE(TAG, "Write failed");
      esp_tls_conn_destroy(tls);
      return result;
    }
  }

  /* Read response */
  while (1) {
    ret = esp_tls_conn_read(tls, buf, sizeof(buf) - 1);
    if (ret == ESP_TLS_ERR_SSL_WANT_WRITE || ret == ESP_TLS_ERR_SSL_WANT_READ) {
      continue;
    }
    if (ret <= 0) {
      break;
    }

    if (!headers_done) {
      buf[ret] = '\0';
      char *body = strstr(buf, "\r\n\r\n");
      if (body) {
        headers_done = true;
        total_bytes = ret - (body + 4 - buf);
        start_time_us = esp_timer_get_time();
      }
      continue;
    }

    total_bytes += ret;

    /* Progress every ~200KB */
    if (total_bytes % 204800 < sizeof(buf)) {
      int64_t elapsed_us = esp_timer_get_time() - start_time_us;
      if (elapsed_us > 0) {
        ESP_LOGI(TAG, "  %6zu bytes | %.1f Kbit/s", total_bytes,
                 (total_bytes * 8000.0f) / elapsed_us);
      }
    }
  }

  esp_tls_conn_destroy(tls);

  /* Calculate results */
  int64_t total_us = esp_timer_get_time() - start_time_us;
  if (total_us > 0 && total_bytes > 0) {
    result.success = true;
    result.bytes_downloaded = total_bytes;
    result.duration_ms = total_us / 1000;
    result.speed_kbps = (total_bytes * 8000.0f) / total_us;
    result.speed_mbps = result.speed_kbps / 1000.0f;

    ESP_LOGI(TAG, "RESULT: %zu bytes in %" PRId64 " ms = %.2f Mbit/s",
             result.bytes_downloaded, result.duration_ms, result.speed_mbps);
  }

  return result;
}

/*
 * Print summary table
 */
static void print_summary(void) {
  ESP_LOGI(TAG, "");
  ESP_LOGI(TAG, "========================================");
  ESP_LOGI(TAG, "           TEST SUMMARY");
  ESP_LOGI(TAG, "========================================");
  ESP_LOGI(TAG, "%-30s %10s %10s", "Test", "Mbit/s", "Status");
  ESP_LOGI(TAG, "----------------------------------------");

  for (int i = 0; i < num_results; i++) {
    if (results[i].success) {
      ESP_LOGI(TAG, "%-30s %10.2f %10s", results[i].name, results[i].speed_mbps,
               "OK");
    } else {
      ESP_LOGI(TAG, "%-30s %10s %10s", results[i].name, "-", "FAIL");
    }
  }

  ESP_LOGI(TAG, "========================================");
  ESP_LOGI(TAG, "Minimum free heap: %" PRIu32 " bytes",
           esp_get_minimum_free_heap_size());
}

/*
 * Test definitions - simplified for automated testing
 * Use pytest to test different sdkconfig settings (TCP_WND, mailbox, etc.)
 */
static const test_config_t tests[] = {
    /* WiFi only baseline */
    {
        .name = "WiFi only",
        .tls_version = TLS_VERSION_1_2,
        .ble_enabled = false,
        .wifi_ps_enabled = false,
        .coex_pref = ESP_COEX_PREFER_BALANCE,
        .ble_adv_interval_ms = 0,
    },
#ifdef CONFIG_BT_ENABLED
    /* BLE coexistence test */
    {
        .name = "BLE coex",
        .tls_version = TLS_VERSION_1_2,
        .ble_enabled = true,
        .wifi_ps_enabled = false,
        .coex_pref = ESP_COEX_PREFER_BALANCE,
        .ble_adv_interval_ms = 100,
    },
#endif
};

static const int num_tests = sizeof(tests) / sizeof(tests[0]);

/*
 * Main test task
 */
static void test_task(void *pvparameters) {
  ESP_LOGI(TAG, "Starting speed test suite (%d tests)", num_tests);

  for (int i = 0; i < num_tests && num_results < MAX_TESTS; i++) {
    apply_config(&tests[i]);
    results[num_results++] = run_speed_test(&tests[i]);

    /* Delay between tests */
    if (i < num_tests - 1) {
      ESP_LOGI(TAG, "Waiting before next test...");
      vTaskDelay(pdMS_TO_TICKS(3000));
    }
  }

  /* Stop BLE before summary */
  stop_ble();

  print_summary();

  ESP_LOGI(TAG, "Test suite complete.");
  vTaskDelete(NULL);
}

void app_main(void) {
  /* Parse speed test URL */
  parse_url(CONFIG_SPEEDTEST_URL);

  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  ESP_ERROR_CHECK(example_connect());

  /* Sync time for certificate validation */
  ESP_LOGI(TAG, "Syncing time from SNTP...");
  esp_err_t ret = fetch_and_store_time_in_nvs(NULL);
  if (ret != ESP_OK) {
    ESP_LOGW(TAG, "First sync failed, retrying...");
    ret = fetch_and_store_time_in_nvs(NULL);
  }
  if (ret == ESP_OK) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    ESP_LOGI(TAG, "Current time: %s", asctime(&timeinfo));
  } else {
    ESP_LOGW(TAG, "Time sync failed - certificate validation may fail");
  }

  xTaskCreate(&test_task, "test_task", 8192, NULL, 5, NULL);
}
