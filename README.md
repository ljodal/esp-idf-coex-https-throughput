# ESP32-C5 WiFi/BLE Coexistence HTTPS Throughput Test

An ESP-IDF example for testing HTTPS download throughput on ESP32-C5 with
WiFi/BLE coexistence.

> [!NOTE]
> I've used Claude Code to develop this example, for finding relevant config
> options to tweak, and to summarize the findings.

## Key Findings

IRAM optimalizations seem to significantly slow down throughput when BLE is
active, so those should be disabled:

```
CONFIG_LWIP_IRAM_OPTIMIZATION=n
CONFIG_ESP_WIFI_IRAM_OPT=n
CONFIG_ESP_WIFI_RX_IRAM_OPT=n
```

## Profiles

Three pre-tuned profiles in `sdkconfig.defaults`:

| Profile            | Throughput  | Free Heap | TCP_WND | Mailbox |
|--------------------|-------------|-----------|---------|---------|
| MINIMAL_RAM        |   ~3 Mbit/s |   ~100 KB |    8192 |      12 |
| BALANCED (default) |   ~5 Mbit/s |    ~90 KB |   16384 |      12 |
| MAX_SPEED          | ~6-7 Mbit/s |    ~55 KB |   32768 |      24 |

These profiles are based on testing throughput in various cases, see
[COEX_TUNING_RESULTS.md](COEX_TUNING_RESULTS.md) for detailed test results.

## Quick Start

1. Set your WiFi credentials in `sdkconfig.defaults` or with `idf.py menuconfig`:
   ```
   CONFIG_EXAMPLE_WIFI_SSID="your_ssid"
   CONFIG_EXAMPLE_WIFI_PASSWORD="your_password"
   ```

2. Build and flash:
   ```bash
   idf.py set-target esp32c5
   idf.py build flash monitor
   ```

## What It Does

1. Connects to WiFi
2. Starts BLE advertising (NimBLE)
3. Downloads 1MB from Cloudflare speed test server
4. Reports throughput and heap usage

## Test Output

```
I (12345) speedtest: WiFi only: 1000000 bytes in 1234 ms = 6.48 Mbit/s
I (23456) speedtest: BLE coex: 1000000 bytes in 1876 ms = 4.26 Mbit/s
I (23456) speedtest: Minimum free heap: 92456 bytes
```

## Requirements

- ESP-IDF v5.4+ (tested with v5.5.2)
- ESP32-C5 development board
