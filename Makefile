# ESP32-C5 WiFi/BLE Coexistence Throughput Test
#
# Usage:
#   make menuconfig              # Configure WiFi credentials
#   make balanced                # Build with balanced profile
#   make minimal_ram             # Build with minimal RAM profile
#   make max_speed               # Build with max speed profile
#   make flash monitor           # Flash and monitor
#
# With WiFi credentials:
#   WIFI_SSID=mynet WIFI_PASSWORD=secret make balanced
#
# Or create a .env file (gitignored):
#   echo 'WIFI_SSID=mynet' > .env
#   echo 'WIFI_PASSWORD=secret' >> .env

# Load .env file if it exists
-include .env

.PHONY: all balanced minimal_ram max_speed flash monitor menuconfig clean fullclean

all: balanced

# Ensure sdkconfig exists
sdkconfig:
	idf.py reconfigure

# Apply a profile file to sdkconfig
define apply_profile
	@echo "Applying profile: $(1)"
	@while IFS='=' read -r key value; do \
		[ -z "$$key" ] && continue; \
		case "$$key" in \#*) continue;; esac; \
		sed -i '' "s|^$$key=.*|$$key=$$value|" sdkconfig; \
	done < $(1)
endef

# Apply WiFi credentials if set
define apply_wifi
	@if [ -n "$(WIFI_SSID)" ]; then \
		sed -i '' 's|^CONFIG_EXAMPLE_WIFI_SSID=.*|CONFIG_EXAMPLE_WIFI_SSID="$(WIFI_SSID)"|' sdkconfig; \
	fi
	@if [ -n "$(WIFI_PASSWORD)" ]; then \
		sed -i '' 's|^CONFIG_EXAMPLE_WIFI_PASSWORD=.*|CONFIG_EXAMPLE_WIFI_PASSWORD="$(WIFI_PASSWORD)"|' sdkconfig; \
	fi
endef

balanced: sdkconfig
	$(call apply_profile,sdkconfig.balanced)
	$(apply_wifi)
	idf.py build

minimal_ram: sdkconfig
	$(call apply_profile,sdkconfig.minimal_ram)
	$(apply_wifi)
	idf.py build

max_speed: sdkconfig
	$(call apply_profile,sdkconfig.max_speed)
	$(apply_wifi)
	idf.py build

flash:
	idf.py flash

monitor:
	idf.py monitor

menuconfig:
	idf.py menuconfig

clean:
	idf.py clean

fullclean:
	idf.py fullclean
