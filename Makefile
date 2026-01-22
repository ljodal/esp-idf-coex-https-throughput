# ESP32-C5 WiFi/BLE Coexistence Throughput Test
#
# Usage:
#   make PROFILE=balanced build       # Build specific profile
#   make PROFILE=balanced flash       # Flash specific profile
#   make PROFILE=balanced monitor     # Monitor serial output
#   make PROFILE=balanced all         # Build, flash, and monitor
#   make build-all                    # Build all profiles (for testing)
#   make menuconfig                   # Configure via menuconfig
#   make clean                        # Clean current profile
#   make clean-all                    # Clean all profile builds
#
# WiFi credentials (required):
#   Create sdkconfig.wifi (gitignored) with your credentials:
#     echo 'CONFIG_EXAMPLE_WIFI_SSID="your_ssid"' > sdkconfig.wifi
#     echo 'CONFIG_EXAMPLE_WIFI_PASSWORD="your_pass"' >> sdkconfig.wifi
#
# Profiles are auto-discovered from sdkconfig.* files

# Default profile
PROFILE ?= balanced

# Auto-discover profiles from sdkconfig.* files (excluding defaults and old)
PROFILES := $(shell ls sdkconfig.* 2>/dev/null | sed 's/sdkconfig\.//' | grep -v -E '^(defaults|old|wifi)$$')

# Target chip
TARGET := esp32c5

# Build directory for current profile
BUILD_DIR := build_$(PROFILE)

# Validate profile
ifeq ($(filter $(PROFILE),$(PROFILES)),)
$(error Invalid PROFILE '$(PROFILE)'. Available: $(PROFILES))
endif

.PHONY: all build flash monitor menuconfig clean clean-all build-all

# Default: build, flash, and monitor
all: build flash monitor

# Build current profile
build:
	@echo "Building profile: $(PROFILE)"
	@echo "Build directory: $(BUILD_DIR)"
	@test -f sdkconfig.wifi || (echo "Error: sdkconfig.wifi not found. Create it with your WiFi credentials:" && \
		echo "  echo 'CONFIG_EXAMPLE_WIFI_SSID=\"your_ssid\"' > sdkconfig.wifi" && \
		echo "  echo 'CONFIG_EXAMPLE_WIFI_PASSWORD=\"your_pass\"' >> sdkconfig.wifi" && exit 1)
	SDKCONFIG_DEFAULTS="sdkconfig.wifi;sdkconfig.defaults;sdkconfig.$(PROFILE)" \
	idf.py -B $(BUILD_DIR) set-target $(TARGET) build

# Flash current profile
flash:
	idf.py -B $(BUILD_DIR) flash

# Monitor serial output
monitor:
	idf.py -B $(BUILD_DIR) monitor

# Open menuconfig for current profile
menuconfig:
	idf.py -B $(BUILD_DIR) menuconfig

# Clean current profile
clean:
	idf.py -B $(BUILD_DIR) fullclean

# Build all profiles (for test preparation)
build-all:
	@for profile in $(PROFILES); do \
		echo ""; \
		echo "========================================"; \
		echo "Building profile: $$profile"; \
		echo "========================================"; \
		$(MAKE) PROFILE=$$profile build || exit 1; \
	done
	@echo ""
	@echo "All profiles built successfully!"

# Clean all profile build directories
clean-all:
	@for profile in $(PROFILES); do \
		if [ -d "build_$$profile" ]; then \
			echo "Removing build_$$profile"; \
			rm -rf "build_$$profile"; \
		fi; \
	done
	@echo "All build directories cleaned"
