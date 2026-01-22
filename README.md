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

Three pre-tuned profiles are available:

| Profile            | Throughput  | Free Heap | TCP_WND | Mailbox |
|--------------------|-------------|-----------|---------|---------|
| MINIMAL_RAM        |   ~3 Mbit/s |   ~100 KB |    8192 |      12 |
| BALANCED (default) |   ~5 Mbit/s |    ~90 KB |   16384 |      12 |
| MAX_SPEED          | ~6-7 Mbit/s |    ~55 KB |   32768 |      24 |

See [COEX_TUNING_RESULTS.md](COEX_TUNING_RESULTS.md) for detailed test results.

## Quick Start

1. Create `sdkconfig.wifi` with your WiFi credentials:
   ```bash
   echo 'CONFIG_EXAMPLE_WIFI_SSID="your_ssid"' > sdkconfig.wifi
   echo 'CONFIG_EXAMPLE_WIFI_PASSWORD="your_pass"' >> sdkconfig.wifi
   ```

2. Build with a profile:
   ```bash
   make PROFILE=balanced build
   ```

3. Flash and monitor:
   ```bash
   make PROFILE=balanced flash monitor
   ```

Available make targets:
- `make PROFILE=balanced build` - Best tradeoff (~5 Mbit/s, ~90KB heap)
- `make PROFILE=minimal_ram build` - For RAM-constrained apps (~3 Mbit/s, ~100KB heap)
- `make PROFILE=max_speed build` - Maximum throughput (~6-7 Mbit/s, ~55KB heap)
- `make PROFILE=balanced all` - Build, flash, and monitor in one command
- `make build-all` - Build all profiles
- `make clean-all` - Clean all profile build directories

## Automated Testing

Run throughput tests across all profiles using pytest-embedded:

1. Set up the test environment:
   ```bash
   python3 -m venv .venv
   .venv/bin/pip install -r requirements-test.txt
   ```

2. Build all profiles:
   ```bash
   make build-all
   ```

3. Run the tests:
   ```bash
   .venv/bin/pytest -v
   ```

   Or test a specific profile:
   ```bash
   .venv/bin/pytest -v -k balanced
   ```

Results are displayed in a summary table at the end of the test run.

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
- Python 3 (for automated testing)
