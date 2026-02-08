# WS2812B LED Test Application Firmware

WS2812B LED test application firmware for validating the EngEmil STM32 Bootloader functionality.

**Purpose:** Minimal WS2812B LED (ChibiOS) test application for bootloader validation.

**Features:**
- Blink with WS2812B LED (connected to PC10) every 3 seconds
- Valid bootloader header (0xDEADBEEF magic + CRC32)
- Demonstrates application jump from bootloader


## Libraries and Drivers

- `application/libs/ee_ws2812b_chibios_driver` - EngEmil WS2812B ChibiOS Driver


## Build Instructions

```bash
cd test-firmwares/ws2812b_led_test_app_fw/application
# or
cd application
make clean              # Clean (previous) build artifacts
make                    # Build and auto-patch test application
```

**Note:** The build process automatically patches the firmware binary with:
- Firmware size (excluding 32-byte header)
- CRC32 checksum (for bootloader validation)

**Output files:**
- `build/ws2812b-led-test-app-fw.bin` - Unpatched binary (do not upload)
- `build/ws2812b-led-test-app-fw_patched.bin` - **Ready to upload** (includes CRC32)

**Always upload the `_patched.bin` file!** The bootloader requires a valid CRC32 to execute the firmware.

