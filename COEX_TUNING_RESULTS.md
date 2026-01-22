# WiFi/BLE Coexistence Tuning Results

ESP32-C5, ESP-IDF v5.5.2, 1MB HTTPS download from speed.cloudflare.com

## Test Configurations

### 1. Original baseline (ESP-IDF defaults)
```
IRAM optimizations: enabled (default)
Buffers: default
```
| Test | Mbit/s |
|------|--------|
| WiFi only | ~6.2 |
| BLE 20ms | ~2.4 |
| BLE 100ms | ~2.4 |
| BLE 1000ms | ~2.4 |

### 2. Large buffers + IRAM enabled (iperf-style config)
```
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=20
CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=48
CONFIG_ESP_WIFI_DYNAMIC_TX_BUFFER_NUM=48
CONFIG_ESP_WIFI_TX_BA_WIN=32
CONFIG_ESP_WIFI_RX_BA_WIN=22
CONFIG_LWIP_TCP_SND_BUF_DEFAULT=41760
CONFIG_LWIP_TCP_WND_DEFAULT=65535
CONFIG_LWIP_IRAM_OPTIMIZATION=y (default)
CONFIG_ESP_WIFI_IRAM_OPT=y (default)
CONFIG_ESP_WIFI_RX_IRAM_OPT=y (default)
```
| Test | Mbit/s | Notes |
|------|--------|-------|
| WiFi only | 17.10 | Great! |
| BLE 20ms, Balance | 0.38 | Stalls after first few KB |
| BLE 100ms, Balance | 0.35 | Stalls |
| BLE 1000ms, Balance | 0.27 | Stalls |
| BLE 20ms, WiFi prio | 1.11 | Stalls |
| BLE 100ms, WiFi prio | 0.95 | Stalls |
| BLE 1000ms, WiFi prio | 0.59 | Stalls |

**Conclusion:** IRAM optimizations break coex - causes blocking/stalling

### 3. Large buffers + IRAM disabled
```
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=20
CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=48
CONFIG_ESP_WIFI_DYNAMIC_TX_BUFFER_NUM=48
CONFIG_ESP_WIFI_TX_BA_WIN=32
CONFIG_ESP_WIFI_RX_BA_WIN=22
CONFIG_LWIP_TCP_SND_BUF_DEFAULT=41760
CONFIG_LWIP_TCP_WND_DEFAULT=65535
CONFIG_LWIP_IRAM_OPTIMIZATION=n
CONFIG_ESP_WIFI_IRAM_OPT=n
CONFIG_ESP_WIFI_RX_IRAM_OPT=n
```
| Test | Mbit/s | Free heap |
|------|--------|-----------|
| WiFi only | 0.78 | 2860 bytes |
| BLE 20ms, Balance | 7.46 | |
| BLE 100ms, Balance | 5.16 | |
| BLE 1000ms, Balance | 7.59 | |
| BLE 20ms, WiFi prio | 0.40 | |
| BLE 100ms, WiFi prio | 2.49 | |
| BLE 1000ms, WiFi prio | 2.00 | |

**Conclusion:** Critically low heap causes erratic behavior

### 4. Medium buffers + IRAM disabled
```
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=16
CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=32
CONFIG_ESP_WIFI_DYNAMIC_TX_BUFFER_NUM=32
CONFIG_ESP_WIFI_TX_BA_WIN=32
CONFIG_ESP_WIFI_RX_BA_WIN=22
CONFIG_LWIP_TCP_SND_BUF_DEFAULT=16384
CONFIG_LWIP_TCP_WND_DEFAULT=32768
CONFIG_LWIP_IRAM_OPTIMIZATION=n
CONFIG_ESP_WIFI_IRAM_OPT=n
CONFIG_ESP_WIFI_RX_IRAM_OPT=n
```
| Test | Mbit/s | Free heap |
|------|--------|-----------|
| WiFi only | 12.79 | 55128 bytes |
| BLE 20ms, Balance | 5.94 | |
| BLE 100ms, Balance | 5.90 | |
| BLE 1000ms, Balance | 5.94 | |
| BLE 20ms, WiFi prio | 5.11 | |
| BLE 100ms, WiFi prio | 6.43 | |
| BLE 1000ms, WiFi prio | 6.30 | |

**Conclusion:** Good balance of performance and memory

### 5. Default buffers + IRAM disabled
```
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=10 (default)
CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=32 (default)
CONFIG_ESP_WIFI_DYNAMIC_TX_BUFFER_NUM=32 (default)
CONFIG_ESP_WIFI_TX_BA_WIN=6 (default)
CONFIG_ESP_WIFI_RX_BA_WIN=6 (default)
CONFIG_LWIP_TCP_SND_BUF_DEFAULT=5744 (default)
CONFIG_LWIP_TCP_WND_DEFAULT=5744 (default)
CONFIG_LWIP_IRAM_OPTIMIZATION=n
CONFIG_ESP_WIFI_IRAM_OPT=n
CONFIG_ESP_WIFI_RX_IRAM_OPT=n
```
| Test | Mbit/s | Free heap |
|------|--------|-----------|
| WiFi only | 7.28 | 104072 bytes |
| BLE 20ms, Balance | 2.65 | |
| BLE 100ms, Balance | 3.37 | |
| BLE 1000ms, Balance | 2.89 | |
| BLE 20ms, WiFi prio | 3.25 | |
| BLE 100ms, WiFi prio | 2.87 | |
| BLE 1000ms, WiFi prio | 3.13 | |

**Conclusion:** Maximum memory savings, still reasonable throughput

### 6. Default buffers + increased BA windows + IRAM disabled
```
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=10 (default)
CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=32 (default)
CONFIG_ESP_WIFI_DYNAMIC_TX_BUFFER_NUM=32 (default)
CONFIG_ESP_WIFI_TX_BA_WIN=16
CONFIG_ESP_WIFI_RX_BA_WIN=12
CONFIG_LWIP_TCP_SND_BUF_DEFAULT=5744 (default)
CONFIG_LWIP_TCP_WND_DEFAULT=5744 (default)
CONFIG_LWIP_IRAM_OPTIMIZATION=n
CONFIG_ESP_WIFI_IRAM_OPT=n
CONFIG_ESP_WIFI_RX_IRAM_OPT=n
```
| Test | Mbit/s | Free heap |
|------|--------|-----------|
| WiFi only | 6.46 | 100756 bytes |
| BLE 20ms, Balance | 2.96 | |
| BLE 100ms, Balance | 3.02 | |
| BLE 1000ms, Balance | 3.39 | |
| BLE 20ms, WiFi prio | 2.87 | |
| BLE 100ms, WiFi prio | 3.39 | |
| BLE 1000ms, WiFi prio | 2.86 | |

**Conclusion:** BA windows alone don't help much - need TCP buffers too

### 7. Config #4 with TCP_WND halved
```
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=16
CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=32
CONFIG_ESP_WIFI_DYNAMIC_TX_BUFFER_NUM=32
CONFIG_ESP_WIFI_TX_BA_WIN=32
CONFIG_ESP_WIFI_RX_BA_WIN=22
CONFIG_LWIP_TCP_SND_BUF_DEFAULT=16384
CONFIG_LWIP_TCP_WND_DEFAULT=16384  <-- halved
CONFIG_LWIP_IRAM_OPTIMIZATION=n
CONFIG_ESP_WIFI_IRAM_OPT=n
CONFIG_ESP_WIFI_RX_IRAM_OPT=n
```
| Test | Mbit/s | Free heap |
|------|--------|-----------|
| WiFi only | 10.17 | 78300 bytes |
| BLE 20ms, Balance | 5.08 | |
| BLE 100ms, Balance | 5.55 | |
| BLE 1000ms, Balance | 5.09 | |
| BLE 20ms, WiFi prio | 4.26 | |
| BLE 100ms, WiFi prio | 4.09 | |
| BLE 1000ms, WiFi prio | 4.76 | |

**Conclusion:** Good tradeoff - gained ~23KB heap, BLE still ~5 Mbit/s

### 8. Config #7 with TCP_SND_BUF halved too
```
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=16
CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=32
CONFIG_ESP_WIFI_DYNAMIC_TX_BUFFER_NUM=32
CONFIG_ESP_WIFI_TX_BA_WIN=32
CONFIG_ESP_WIFI_RX_BA_WIN=22
CONFIG_LWIP_TCP_SND_BUF_DEFAULT=8192  <-- halved
CONFIG_LWIP_TCP_WND_DEFAULT=16384
CONFIG_LWIP_IRAM_OPTIMIZATION=n
CONFIG_ESP_WIFI_IRAM_OPT=n
CONFIG_ESP_WIFI_RX_IRAM_OPT=n
```
| Test | Mbit/s | Free heap |
|------|--------|-----------|
| WiFi only | 10.05 | 78904 bytes |
| BLE 20ms, Balance | 4.51 | |
| BLE 100ms, Balance | 4.26 | |
| BLE 1000ms, Balance | 4.24 | |
| BLE 20ms, WiFi prio | 4.43 | |
| BLE 100ms, WiFi prio | 4.77 | |
| BLE 1000ms, WiFi prio | 5.12 | |

**Conclusion:** Only ~600 bytes saved, but BLE dropped to ~4.5 Mbit/s - not worth it

### 9. Config #7 with BA windows halved instead
```
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=16
CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=32
CONFIG_ESP_WIFI_DYNAMIC_TX_BUFFER_NUM=32
CONFIG_ESP_WIFI_TX_BA_WIN=16  <-- halved
CONFIG_ESP_WIFI_RX_BA_WIN=12  <-- halved
CONFIG_LWIP_TCP_SND_BUF_DEFAULT=16384
CONFIG_LWIP_TCP_WND_DEFAULT=16384
CONFIG_LWIP_IRAM_OPTIMIZATION=n
CONFIG_ESP_WIFI_IRAM_OPT=n
CONFIG_ESP_WIFI_RX_IRAM_OPT=n
```
| Test | Mbit/s | Free heap |
|------|--------|-----------|
| WiFi only | 9.94 | 78912 bytes |
| BLE 20ms, Balance | 4.79 | |
| BLE 100ms, Balance | 3.93 | |
| BLE 1000ms, Balance | 4.45 | |
| BLE 20ms, WiFi prio | 4.55 | |
| BLE 100ms, WiFi prio | 4.66 | |
| BLE 1000ms, WiFi prio | 4.87 | |

**Conclusion:** No heap savings, slight throughput loss - BA windows don't use much RAM

### 10. Config #7 with reduced STATIC_RX_BUFFER
```
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=10  <-- reduced to default
CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=32
CONFIG_ESP_WIFI_DYNAMIC_TX_BUFFER_NUM=32
CONFIG_ESP_WIFI_TX_BA_WIN=32
CONFIG_ESP_WIFI_RX_BA_WIN=16  <-- reduced to fit constraint
CONFIG_LWIP_TCP_SND_BUF_DEFAULT=16384
CONFIG_LWIP_TCP_WND_DEFAULT=16384
CONFIG_LWIP_IRAM_OPTIMIZATION=n
CONFIG_ESP_WIFI_IRAM_OPT=n
CONFIG_ESP_WIFI_RX_IRAM_OPT=n
```
| Test | Mbit/s | Free heap |
|------|--------|-----------|
| WiFi only | 9.83 | 83004 bytes |
| BLE 20ms, Balance | 4.85 | |
| BLE 100ms, Balance | 4.57 | |
| BLE 1000ms, Balance | 4.55 | |
| BLE 20ms, WiFi prio | 4.28 | |
| BLE 100ms, WiFi prio | 4.46 | |
| BLE 1000ms, WiFi prio | 4.62 | |

**Conclusion:** Gained ~5KB more heap, BLE ~4.5 Mbit/s - reasonable tradeoff

### 11. Config #10 with reduced mailboxes
```
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=10
CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=32
CONFIG_ESP_WIFI_DYNAMIC_TX_BUFFER_NUM=32
CONFIG_ESP_WIFI_TX_BA_WIN=32
CONFIG_ESP_WIFI_RX_BA_WIN=16
CONFIG_LWIP_TCP_SND_BUF_DEFAULT=16384
CONFIG_LWIP_TCP_WND_DEFAULT=16384
CONFIG_LWIP_TCPIP_RECVMBOX_SIZE=6  <-- reduced from 48
CONFIG_LWIP_TCP_RECVMBOX_SIZE=6   <-- reduced from 48
CONFIG_LWIP_IRAM_OPTIMIZATION=n
CONFIG_ESP_WIFI_IRAM_OPT=n
CONFIG_ESP_WIFI_RX_IRAM_OPT=n
```
| Test | Mbit/s | Free heap |
|------|--------|-----------|
| WiFi only | 0.61 | 91644 bytes |
| BLE 20ms, Balance | 0.29 | |
| BLE 100ms, Balance | 0.38 | |
| BLE 1000ms, Balance | 0.23 | |
| BLE 20ms, WiFi prio | 0.69 | |
| BLE 100ms, WiFi prio | 0.26 | |
| BLE 1000ms, WiFi prio | 0.33 | |

**Conclusion:** TERRIBLE - mailbox size 6 kills throughput completely! Only saved 8KB.

### 12. Config #10 with mailboxes=24
```
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=10
CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=32
CONFIG_ESP_WIFI_DYNAMIC_TX_BUFFER_NUM=32
CONFIG_ESP_WIFI_TX_BA_WIN=32
CONFIG_ESP_WIFI_RX_BA_WIN=16
CONFIG_LWIP_TCP_SND_BUF_DEFAULT=16384
CONFIG_LWIP_TCP_WND_DEFAULT=16384
CONFIG_LWIP_TCPIP_RECVMBOX_SIZE=24  <-- middle ground
CONFIG_LWIP_TCP_RECVMBOX_SIZE=24   <-- middle ground
CONFIG_LWIP_IRAM_OPTIMIZATION=n
CONFIG_ESP_WIFI_IRAM_OPT=n
CONFIG_ESP_WIFI_RX_IRAM_OPT=n
```
| Test | Mbit/s | Free heap |
|------|--------|-----------|
| WiFi only | 9.59 | 88024 bytes |
| BLE 20ms, Balance | 4.32 | |
| BLE 100ms, Balance | 4.58 | |
| BLE 1000ms, Balance | 5.58 | |
| BLE 20ms, WiFi prio | 4.15 | |
| BLE 100ms, WiFi prio | 3.77 | |
| BLE 1000ms, WiFi prio | 4.12 | |

**Conclusion:** Good tradeoff - gained 5KB from #10, throughput similar (~4-5 Mbit/s)

**Notable:** BLE interval matters in Balance mode (1000ms=5.58 vs 20ms=4.32, +26%), but not in WiFi prio mode (all ~4 Mbit/s). This pattern was consistent across multiple runs.

### 13. Config #10 with mailboxes=16
```
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=10
CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=32
CONFIG_ESP_WIFI_DYNAMIC_TX_BUFFER_NUM=32
CONFIG_ESP_WIFI_TX_BA_WIN=32
CONFIG_ESP_WIFI_RX_BA_WIN=16
CONFIG_LWIP_TCP_SND_BUF_DEFAULT=16384
CONFIG_LWIP_TCP_WND_DEFAULT=16384
CONFIG_LWIP_TCPIP_RECVMBOX_SIZE=16
CONFIG_LWIP_TCP_RECVMBOX_SIZE=16
CONFIG_LWIP_IRAM_OPTIMIZATION=n
CONFIG_ESP_WIFI_IRAM_OPT=n
CONFIG_ESP_WIFI_RX_IRAM_OPT=n
```
| Test | Mbit/s | Free heap |
|------|--------|-----------|
| WiFi only | 10.50 | 89376 bytes |
| BLE 20ms, Balance | 4.35 | |
| BLE 100ms, Balance | 5.29 | |
| BLE 1000ms, Balance | 5.14 | |
| BLE 20ms, WiFi prio | 4.08 | |
| BLE 100ms, WiFi prio | 4.07 | |
| BLE 1000ms, WiFi prio | 5.17 | |

**Conclusion:** Similar to mailbox=24, gained another ~1.3KB - good sweet spot

### 14. Config #10 with mailboxes=12
```
CONFIG_LWIP_TCPIP_RECVMBOX_SIZE=12
CONFIG_LWIP_TCP_RECVMBOX_SIZE=12
(other settings same as #13)
```
| Test | Mbit/s | Free heap |
|------|--------|-----------|
| WiFi only | 10.23 | 90348 bytes |
| BLE 20ms, Balance | 4.79 | |
| BLE 100ms, Balance | 4.74 | |
| BLE 1000ms, Balance | 4.24 | |
| BLE 20ms, WiFi prio | 4.24 | |
| BLE 100ms, WiFi prio | 4.97 | |
| BLE 1000ms, WiFi prio | 4.82 | |

**Conclusion:** Still performs well, gained ~1KB more - THIS IS THE MINIMUM

### 15. Config #10 with mailboxes=10
```
CONFIG_LWIP_TCPIP_RECVMBOX_SIZE=10
CONFIG_LWIP_TCP_RECVMBOX_SIZE=10
(other settings same as #13)
```
| Test | Mbit/s | Free heap |
|------|--------|-----------|
| WiFi only | 0.88 | 89592 bytes |
| BLE 20ms, Balance | 0.78 | |
| BLE 100ms, Balance | 0.73 | |
| BLE 1000ms, Balance | 0.42 | |
| BLE 20ms, WiFi prio | 0.73 | |
| BLE 100ms, WiFi prio | 0.59 | |
| BLE 1000ms, WiFi prio | 0.66 | |

**Conclusion:** CLIFF - mailbox=10 stalls like mailbox=6. Minimum is 12!

### 16. Config #14 with DYNAMIC buffers=24
```
CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM=24  <-- reduced from 32
CONFIG_ESP_WIFI_DYNAMIC_TX_BUFFER_NUM=24  <-- reduced from 32
(other settings same as #14)
```
| Test | Mbit/s | Free heap |
|------|--------|-----------|
| WiFi only | 9.47 | 89256 bytes |
| BLE 20ms, Balance | 4.25 | |
| BLE 100ms, Balance | 4.00 | |
| BLE 1000ms, Balance | 4.49 | |
| BLE 20ms, WiFi prio | 4.86 | |
| BLE 100ms, WiFi prio | 4.55 | |
| BLE 1000ms, WiFi prio | 4.35 | |

**Conclusion:** No heap savings - dynamic buffers allocated on-demand, weren't fully used

### 17. Config #14 with TCP_WND=12288
```
CONFIG_LWIP_TCP_WND_DEFAULT=12288  <-- reduced from 16384
(other settings same as #14)
```
| Test | Mbit/s | Free heap |
|------|--------|-----------|
| WiFi only | 9.10 | 93876 bytes |
| BLE 20ms, Balance | 3.52 | |
| BLE 100ms, Balance | 4.09 | |
| BLE 1000ms, Balance | 4.82 | |
| BLE 20ms, WiFi prio | 3.90 | |
| BLE 100ms, WiFi prio | 3.42 | |
| BLE 1000ms, WiFi prio | 4.30 | |

**Conclusion:** Gained ~3.5KB heap, BLE dropped to ~3.5-4.8 Mbit/s

### 18. Config #14 with TCP_WND=8192
```
CONFIG_LWIP_TCP_WND_DEFAULT=8192  <-- reduced further
(other settings same as #14)
```
| Test | Mbit/s | Free heap |
|------|--------|-----------|
| WiFi only | 7.57 | 100736 bytes |
| BLE 20ms, Balance | 3.37 | |
| BLE 100ms, Balance | 2.95 | |
| BLE 1000ms, Balance | 3.09 | |
| BLE 20ms, WiFi prio | 3.57 | |
| BLE 100ms, WiFi prio | 2.89 | |
| BLE 1000ms, WiFi prio | 3.72 | |

**Conclusion:** Gained ~7KB more (101KB total), BLE ~3 Mbit/s - similar to defaults but optimized

### 19. Config #14 with TCP_SND_BUF=8192
```
CONFIG_LWIP_TCP_SND_BUF_DEFAULT=8192  <-- halved
CONFIG_LWIP_TCP_WND_DEFAULT=16384    <-- restored
(other settings same as #14)
```
| Test | Mbit/s | Free heap |
|------|--------|-----------|
| WiFi only | 10.71 | 90304 bytes |
| BLE 20ms, Balance | 4.09 | |
| BLE 100ms, Balance | 4.61 | |
| BLE 1000ms, Balance | 5.09 | |
| BLE 20ms, WiFi prio | 4.14 | |
| BLE 100ms, WiFi prio | 3.90 | |
| BLE 1000ms, WiFi prio | 4.80 | |

**Conclusion:** No impact - TCP_SND_BUF is for upload, not download. Can safely reduce.

### 20. Config #14 with TCP_SND_BUF=5744 (default)
```
CONFIG_LWIP_TCP_SND_BUF_DEFAULT=5744  <-- default value
CONFIG_LWIP_TCP_WND_DEFAULT=16384
(other settings same as #14)
```
| Test | Mbit/s | Free heap |
|------|--------|-----------|
| WiFi only | 9.91 | 90384 bytes |
| BLE 20ms, Balance | 4.57 | |
| BLE 100ms, Balance | 5.14 | |
| BLE 1000ms, Balance | 5.44 | |
| BLE 20ms, WiFi prio | 4.56 | |
| BLE 100ms, WiFi prio | 4.06 | |
| BLE 1000ms, WiFi prio | 4.22 | |

**Conclusion:** Confirmed - TCP_SND_BUF can stay at default for download workloads

### 21. MAX_SPEED profile (optimized)
```
CONFIG_LWIP_TCP_WND_DEFAULT=32768
CONFIG_LWIP_TCP_SND_BUF_DEFAULT=5744   <-- default (not wasted)
CONFIG_LWIP_TCPIP_RECVMBOX_SIZE=24     <-- must scale with TCP_WND!
CONFIG_LWIP_TCP_RECVMBOX_SIZE=24
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=16
CONFIG_ESP_WIFI_RX_BA_WIN=22
CONFIG_ESP_WIFI_TX_BA_WIN=32
(IRAM disabled)
```
| Test | Mbit/s | Free heap |
|------|--------|-----------|
| WiFi only | 13.62 | 55520 bytes |
| BLE 20ms, Balance | 6.49 | |
| BLE 100ms, Balance | 6.37 | |
| BLE 1000ms, Balance | 6.36 | |
| BLE 20ms, WiFi prio | 6.57 | |
| BLE 100ms, WiFi prio | 6.97 | |
| BLE 1000ms, WiFi prio | 7.07 | |

**Conclusion:** Best throughput! Mailbox=24 required for TCP_WND=32768 (mailbox=12 stalls)

## Key Findings

1. **IRAM optimizations must be disabled for stable coex** - This is the critical setting
2. **TCP_WND is the main memory vs throughput lever for downloads:**
   - 8192: 101KB heap, ~3 Mbit/s BLE
   - 16384: 90KB heap, ~5 Mbit/s BLE
   - 32768: 55KB heap, ~6-7 Mbit/s BLE
3. **Mailbox must scale with TCP_WND:**
   - TCP_WND ≤16384: mailbox=12 OK
   - TCP_WND=32768: mailbox=24 required (12 causes stalling!)
4. **TCP_SND_BUF has no impact on downloads** - can use default (5744) safely
5. **BA windows have minimal impact on heap** - keep them high for better AMPDU
6. **STATIC_RX_BUFFER saves ~5KB** when reduced from 16 to 10
7. **DYNAMIC buffers don't save heap** - allocated on-demand
8. **BLE advertising interval matters in Balance mode** - 1000ms can be 26% faster than 20ms
9. **Coex preference (Balance vs WiFi) has minimal impact** - Balance + long interval is slightly better
10. **BLE coex costs ~50% WiFi throughput** regardless of settings

## Summary Table

| Config | Key changes | WiFi only | BLE coex | Free heap |
|--------|-------------|-----------|----------|-----------|
| #21 | MAX_SPEED (optimized) | 13.6 Mbit/s | 6-7 Mbit/s | 55 KB |
| #4 | Full buffers, IRAM off | 13 Mbit/s | 5-6 Mbit/s | 55 KB |
| #14/20 | BALANCED profile | 10 Mbit/s | ~5 Mbit/s | 90 KB |
| #17 | TCP_WND=12288 | 9 Mbit/s | ~4 Mbit/s | 94 KB |
| #18 | MINIMAL_RAM profile | 7.5 Mbit/s | ~3 Mbit/s | 101 KB |
| #5 | All defaults, IRAM off | 7 Mbit/s | ~3 Mbit/s | 104 KB |

## Recommended Settings

**MINIMAL_RAM (config #18):**
- ~3 Mbit/s with BLE, ~100KB free heap
- TCP_WND=8192, mailbox=12

**BALANCED (config #14/20):**
- ~5 Mbit/s with BLE, ~90KB free heap
- TCP_WND=16384, mailbox=12
- Use Balance mode + longer advertising interval for best results

**MAX_SPEED (config #21):**
- ~6-7 Mbit/s with BLE, ~55KB free heap
- TCP_WND=32768, mailbox=24 (must scale together!)
- STATIC_RX=16, RX_BA_WIN=22

## Setting Relationships & Dependencies

### Hard Constraints (build will fail)
- **RX_BA_WIN <= 2 × STATIC_RX_BUFFER_NUM** - e.g., if STATIC_RX=10, max RX_BA_WIN=20

### Coupled Settings (change together)
- **TCP_WND ↔ Download throughput** - Direct linear relationship. Each 4KB reduction = ~0.5 Mbit/s loss
- **TCP_WND ↔ Mailbox size** - TCP_WND=32768 requires mailbox≥24, TCP_WND≤16384 works with mailbox=12
- **STATIC_RX_BUFFER ↔ RX_BA_WIN** - Must reduce RX_BA_WIN when reducing STATIC_RX

### Independent Settings (no coupling)
- **TCP_SND_BUF** - Only affects uploads, independent of download settings
- **DYNAMIC_RX/TX buffers** - Allocated on-demand, don't affect heap unless actually used
- **TX_BA_WIN** - Can be set independently (no constraint)

### Settings that use RAM but DON'T help downloads
| Setting | RAM used | Download impact |
|---------|----------|-----------------|
| TCP_SND_BUF (above default) | ~10KB per 10KB increase | None |
| DYNAMIC buffers (max) | 0 (on-demand) | None |

### Settings that help downloads but cost RAM
| Setting | RAM cost | Download impact |
|---------|----------|-----------------|
| TCP_WND | ~4KB per 4KB increase | ~0.5 Mbit/s per 4KB |
| STATIC_RX_BUFFER | ~1.6KB per buffer | Enables larger RX_BA_WIN |
| Mailboxes | ~100 bytes per entry | Cliff below 12 |

### Settings that help downloads but cost NO RAM
| Setting | Download impact |
|---------|-----------------|
| BA windows (TX/RX) | Better AMPDU aggregation |
| IRAM disabled | Required for coex stability |

### Cliff Effects (sudden performance drop)
- **Mailbox < 12**: Throughput drops to <1 Mbit/s (packets dropped)
- **Mailbox too small for TCP_WND**: TCP_WND=32768 needs mailbox>=24, mailbox=12 stalls
- **Heap < 10KB**: System becomes unstable, erratic behavior

### Mailbox vs TCP_WND Scaling
| TCP_WND | Min Mailbox |
|---------|-------------|
| 8192 | 12 |
| 16384 | 12 |
| 32768 | 24 |

## Optimal RAM-Saving Config
```
# Critical for coex stability
CONFIG_LWIP_IRAM_OPTIMIZATION=n
CONFIG_ESP_WIFI_IRAM_OPT=n
CONFIG_ESP_WIFI_RX_IRAM_OPT=n

# Memory-optimized settings
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=10
CONFIG_LWIP_TCP_WND_DEFAULT=16384      # or 8192 for more RAM
CONFIG_LWIP_TCP_SND_BUF_DEFAULT=5744   # default is fine
CONFIG_LWIP_TCPIP_RECVMBOX_SIZE=12     # minimum viable
CONFIG_LWIP_TCP_RECVMBOX_SIZE=12       # minimum viable

# Keep these for performance
CONFIG_ESP_WIFI_TX_BA_WIN=32
CONFIG_ESP_WIFI_RX_BA_WIN=16
```
