# ESP32-C5 WiFi/BLE Coexistence HTTPS Throughput Test

An ESP-IDF example for testing HTTPS download throughput on ESP32-C5 with
WiFi/BLE coexistence.

> [!NOTE]
> I've used Claude Code to develop this example, for finding relevant config
> options to tweak, and to summarize the findings.

## Key Findings

**Buffer matching matters**: Following the ESP-IDF WiFi performance guide rules
improves BLE coexistence throughput. Key rules:
- `TCP_WND_DEFAULT` should match `DYNAMIC_RX_BUFFER_NUM` in KB
- `TCP_SND_BUF_DEFAULT` should match `DYNAMIC_TX_BUFFER_NUM` in KB
- `RX_BA_WIN` should be min(2×`STATIC_RX_BUFFER_NUM`, `DYNAMIC_RX_BUFFER_NUM`)

**IRAM optimizations**: These improve throughput but consume significant RAM
(~24KB less free heap). Disabled by default to preserve heap for applications.
Test with the `iram_enabled` profile if you have RAM to spare.

```
CONFIG_LWIP_IRAM_OPTIMIZATION=n
CONFIG_ESP_WIFI_IRAM_OPT=n
CONFIG_ESP_WIFI_RX_IRAM_OPT=n
```

## Profiles

| Profile            | WiFi only | BLE coex | Free Heap |
|--------------------|-----------|----------|-----------|
| minimal_ram        | ~6 Mbit/s | ~2 Mbit/s |   ~110 KB |
| balanced (default) | ~8 Mbit/s | ~4 Mbit/s |    ~92 KB |
| max_speed          | ~11 Mbit/s | ~5 Mbit/s |    ~33 KB |
| iram_enabled       | ~10 Mbit/s | ~4 Mbit/s |    ~68 KB |

> [!NOTE]
> Throughput numbers vary with WiFi conditions (distance to router, interference,
> network congestion). Use these as relative comparisons between profiles rather
> than absolute expectations.

See [COEX_TUNING_RESULTS.md](COEX_TUNING_RESULTS.md) for detailed test results.

## Quick Start

1. Create `sdkconfig.local` with your WiFi credentials (and any other overrides):
   ```bash
   echo 'CONFIG_EXAMPLE_WIFI_SSID="your_ssid"' > sdkconfig.local
   echo 'CONFIG_EXAMPLE_WIFI_PASSWORD="your_pass"' >> sdkconfig.local
   # Optional: custom test URL
   # echo 'CONFIG_SPEEDTEST_URL="https://example.com/testfile"' >> sdkconfig.local
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
- `make PROFILE=balanced build` - Best tradeoff (~4 Mbit/s BLE coex, ~90KB heap)
- `make PROFILE=minimal_ram build` - For RAM-constrained apps (~3 Mbit/s, ~100KB heap)
- `make PROFILE=max_speed build` - Maximum throughput (~5 Mbit/s BLE coex, ~35KB heap)
- `make PROFILE=iram_enabled build` - Test IRAM optimizations (~4 Mbit/s, ~64KB heap)
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
