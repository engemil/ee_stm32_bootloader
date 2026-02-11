# EngEmil STM32 Bootloader (ee_stm32_bootloader)

**Version:** 1.0.0

A fully functional bootloader for STM32 microcontroller(s), featuring USB DFU (Device Firmware Upgrade), CRC32 firmware validation, safe flash programming, and multiple bootloader entry conditions. Built on ChibiOS framework, for the USB stack, RTOS kernel, and HAL layer.

**Supported chip(s):** `STM32C071RB`

**Features**
- **USB DFU 1.1 Protocol** - Full compliance, works with `dfu-util`.
- **DFUSe Extensions** - Set Address Pointer (0x21), Erase command (0x41) for advanced use cases.
- **Firmware Application Validation** - Magic number (`0xDEADBEEF`, firmware size, and CRC32 integrity check all)
- **Application Header Enforcement** - 32-byte mandatory header with magic (`0xDEADBEEF`), version, size, and CRC32.
- **Safe Flash Operations** - 64-bit double-word writes (STM32C0 compliant).
- **Bootloader Auto-jump Timeout** - Automatically jumps to application after timeout period if inactive in bootloader.
- **Bootloader Protection** - Address validation prevents self-overwrite.
- **Multiple Entry Modes** - Magic RAM value (enter from application), invalid firmware detection, user button. <!-- , watchdog reset detection. -->
- **Vector Table Relocation** - Bootloader automaticly selects the correct interrupt vector table. Two vector tables (bootloader and application).
- **ChibiOS RTOS** - Master branch for USB stack.
- **Crystal-less USB** - HSI48 internal oscillator with CRS.
- **Zero Warnings** - Compiles with `-Werror`, cppcheck clean.


## Dependencies

### Required Tools
- **arm-none-eabi-gcc** (13.2.1 or later) - ARM cross-compiler.
- **make** (4.3 or later) - Build system.
- **st-flash** or **openocd** - Hardware programming.
  - NB! **stlink** v1.8.0 does not have STM32C071xx included. On Ubuntu 24.04 MUST build from source with `develop`-branch [link](https://github.com/stlink-org/stlink).
  - NB! **openocd** v0.12.0 or newer. On Ubuntu 24.04 you MUST build from source [link](https://github.com/openocd-org/openocd).
- **dfu-util** (0.11 or later) - USB Device Firmware Upgrade (DFU). Firmware uploads over USB.

### Installation (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install git build-essential gcc-arm-none-eabi \
  binutils-arm-none-eabi libnewlib-arm-none-eabi \
  python3 dfu-util -y

# NB! We also need stlink-tools and openocd, but they are outdated on ubuntu 24.04.
# see the .devcontainer/Dockerfile for how to install from source, or use docker container
#sudo apt install stlink-tools openocd -y
```

### Setup Repository

```bash
git clone https://github.com/engemil/ee_stm32_bootloader.git
cd ee_stm32_bootloader
git submodule update --init --recursive
```

See `ext/README.md` for detailed submodule management.


## Quick Start

**Upload Bootloader Firmware via ST-Link (probe):**

```bash
# Build bootloader
cd bootloader && make clean && make
# Flash to device over debugger (the first stlink it sees)
st-flash --reset write build/bootloader.bin 0x08000000
# Verify entered USB DFU mode
lsusb | grep 0483:df11
```

**Upload Application Firmware via USB DFU (Bootloader on chip):**
```bash

# Upload application firmware over USB
cd ../test-firmwares/led_test_app_fw/application && make clean && make

# Upload test firmware
sudo dfu-util -a 0 --dfuse-address 0x08004000:leave -D build/led-test-app-fw_patched.bin
```

See sections below for detailed instructions.


## Project Structure

```
./
├── .devcontainer/               - Devcontainer and Docker files
├── .vscode/                     - VS Code configuration
├── bootloader/                  - Core bootloader code
│   ├── .dep/                    - Build dependencies outputs (generated)
│   ├── build/                   - Build outputs (generated)
│   ├── inc/                     - Header files
│   │   ├── config.h             - Memory map, constants, error codes
│   │   ├── bootloader.h         - Bootloader API and structures
│   │   ├── usb_dfu.h            - DFU protocol definitions and API
│   │   ├── flash_ops.h          - Flash operations API
│   │   ├── crc32.h              - CRC32 API
│   │   ├── chconf.h             - ChibiOS kernel configuration
│   │   ├── halconf.h            - ChibiOS HAL configuration
│   │   └── mcuconf.h            - MCU-specific config
│   ├── src/                     - Source files
│   │   ├── main.c               - Entry point, main loop
│   │   ├── bootloader.c         - Core bootloader logic
│   │   ├── usb_dfu.c            - USB DFU protocol implementation
│   │   ├── flash_ops.c          - Flash erase/write operations (64-bit writes)
│   │   └── crc32.c              - CRC32 calculation with lookup table
│   ├── .gitignore               - Git ignore file
│   ├── Makefile                 - Bootloader build system
│   ├── STM32C071.svd            - SVD file
│   └── stm32c071rb_bootloader.ld - Linker script (bootloader)
├── docs/                        - Documentation
│   ├── supplements              - Supplement files (datasheet(s), user manual(s), etc.
│   └── BOOTLOADER_INTEGRATION.md - Complete guide for application developers
├── ext/                         - External dependencies
├── scripts/                     - Build and utility scripts
│   ├── system/                  - System related scripts for Ubuntu (Linux)
│   └── patch_app_header.py      - Post-build script: calculate and patch firmware size/CRC32
├── test-firmwares/              - Test application firmwares for validation
│   ├── led_test_app_fw/         - LED example
│   ├── template/                - Ready-to-use integration template
│   ├── ws2812b_led_test_app_fw/ - WS2812B LED example
│   └── README.md                - Information about test firmwares and template
├── AGENTS.md                    - AI agent file
├── CHANGELOG.md                 - Version history and release notes
├── opencode.json                - opencode AI agent file
└── README.md                    - This file
```

**Note:** This bootloader uses ChibiOS-provided startup code (`crt0_v6m.S`, `crt1.c`, `vectors.S`) instead of ST's reference startup files. These are automatically included via the ChibiOS build system (see `Makefile` reference to `include $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32c0xx.mk`).


## Build Instructions

### Basic Build
```bash
cd bootloader
make clean              # Clean (previous) build artifacts
make                    # Build bootloader (release, optimized for size with debug symbols)
```


## Flash Instructions

### Method 1: OpenOCD (Recommended)
```bash
sudo openocd -f interface/stlink.cfg \
  -c "transport select swd" \
  -f target/stm32c0x.cfg \
  -c "program build/bootloader.bin 0x08000000 verify reset exit"
```

### Method 2: st-flash
```bash
st-flash --reset write build/bootloader.bin 0x08000000
```

### Method 3: VS Code
1. Open bootloader project in VS Code
2. Press `Ctrl+Shift+B` to build
3. Run task: **Flash Bootloader (OpenOCD)** or **Flash Bootloader (st-flash)**

**Note:** All approaches resets properly and enters into USB DFU mode automatically. If it does not work, try a power-cycle (press reset button or reconnect USB cable).

### More than one probe/ST-Link?

`st-flash` example:
```bash
# Use st-info to see how many you have connected
st-info probe

# You can extract the serial string as following
echo $(st-info --probe | grep -oP '(?<=serial:\s{5})[A-F0-9]+')

# Example on how to flash with specific probe/st-link
st-flash --reset --serial 0040003D3739541678553432 write build/bootloader.bin 0x08000000

# SUGGESTION: Make a script with this info to then be able to select which probe to use
```


## Usage

### Entering Bootloader Mode

The bootloader automatically enters USB DFU mode if:
1. **No valid application** detected (magic `0xDEADBEEF` missing or CRC32 mismatch)
2. **Magic RAM value** present (application writes `0xDEADBEEF` to `0x20005FFC` and resets)
<!-- 3. **Watchdog reset** detected (`RCC->CSR2` `IWDGRSTF` flag - STM32C0 specific register) -->

### Uploading Firmware via DFU

Test firmware placed under `test-firmwares`-folder.

```bash
# 1. Check if bootloader is in DFU mode
lsusb | grep 0483:df11
# Expected: Bus XXX Device YYY: ID 0483:df11 STMicroelectronics STM32 Bootloader DFU Mode

# 2. Verify DFU device detected
sudo dfu-util -l
# Expected: Found DFU: [0483:df11] ... alt=0, name="@Internal Flash  /0x08004000/112*001Kg"

# 3. Upload firmware (use _patched.bin file!)
sudo dfu-util -a 0 --dfuse-address 0x08004000:leave -D test-firmwares/led_test_app_fw/application/build/led-test-app-fw_patched.bin
# Expected: Erase done, Download done, File downloaded successfully

# 4. Device automatically resets and runs application
```

**Important: Patched vs. Unpatched Binaries**

Applications must be **patched** before upload to include valid size and CRC32 fields:

- **`test-app.bin`** (unpatched) - Raw compiler output with zeros in size/CRC32 fields → **WILL NOT BOOT** ❌
- **`test-app_patched.bin`** (patched) - Processed with valid size/CRC32 → **READY TO USE** ✅

The build system automatically generates both files. The `patch_app_header.py` script calculates the firmware size and CRC32 checksum after compilation and writes them into the application header. The bootloader validates these fields before jumping to the application.

**Always use the `_patched.bin` file for DFU uploads!**

Test application firmwares are available to be built in the `test-firmwares/` directory.



### DFUSe Extensions (Advanced)

The bootloader supports ST-specific DFUSe extensions for advanced use cases:

**Non-Sequential Writes (Set Address Pointer 0x21):**
```bash
# Upload to specific address (useful for partial updates)
sudo dfu-util -a 0 --dfuse-address 0x08008000 -D <firmware-bin-file>
```

**Explicit Erase (Erase Command 0x41):**
```bash
# Erase before upload (automatically done if omitted)
sudo dfu-util -a 0 -s 0x08004000:mass-erase -D <firmware-bin-file>
```

**Upload Without Auto-Reset:**
```bash
# Stay in bootloader after upload (omit :leave suffix)
sudo dfu-util -a 0 --dfuse-address 0x08004000 -D <firmware-bin-file>
```

### Application Development

Applications must:
1. **Include bootloader header** at 0x08004000 (32 bytes):
   ```c
   typedef struct __attribute__((packed)) {
       uint32_t magic;      // 0xDEADBEEF
       uint32_t version;    // Firmware version
       uint32_t size;       // Firmware size (bytes)
       uint32_t crc32;      // CRC32 checksum
       uint32_t reserved[4];
   } app_header_t;
   ```

2. **Vector table at 0x08004100** (256-byte aligned, ARM Cortex-M0+ requirement)

3. **Linker script** must place code at 0x08004100:
   ```ld
   MEMORY {
       FLASH (rx) : ORIGIN = 0x08004100, LENGTH = 112K - 256
       RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 24K
   }
   ```
   
   Note: 256-byte padding (0x08004020-0x080040FF) required for ARM Cortex-M0+ vector table alignment.

4. **Do NOT set SCB->VTOR** (memory address for the interrupt vector table). The bootloader handles this, application MUST NOT do it! The system will have two interrupt vector tables, one for bootloader and one for application. However, as stated the bootloader will control which one is active.


## Memory Layout

```
STM32C071RB: 128KB Flash, 24KB RAM

Flash Map:
├─ 0x08000000 - 0x08003FFF : Bootloader (16KB allocated, 8.6KB used)
└─ 0x08004000 - 0x0801FFFF : Application (112KB)
    ├─ 0x08004000 - 0x0800401F : Application header (32 bytes)
    ├─ 0x08004020 - 0x080040FF : Padding (224 bytes, for 256-byte alignment)
    └─ 0x08004100 - 0x0801FFFF : Vector table + code

RAM Map:
└─ 0x20000000 - 0x20005FFF : 24KB (used by bootloader and ChibiOS)
```

## Debugging

### GDB with OpenOCD

Terminal 1 (OpenOCD server):
```bash
sudo openocd -f interface/stlink.cfg \
  -c "transport select swd" \
  -f target/stm32c0x.cfg
```

Terminal 2 (GDB client):
```bash
cd bootloader
gdb-multiarch build/bootloader.elf
(gdb) target remote localhost:3333
(gdb) monitor reset halt
(gdb) load
(gdb) break main
(gdb) continue
```

### VS Code Debugging
1. Start OpenOCD server (Terminal 1 above)
2. In VS Code: Press `F5` or Run > Start Debugging
3. Select configuration: "Debug Bootloader (OpenOCD)"
4. Debugger will stop at `main()`


## Contributing

Remember for new version of bootloader, update `BOOTLOADER_VERSION`-value in `bootloader/src/bootloader.c`.


## License

MIT License, see `LICENSE`-file for details.

Submodule lincense(s):
- ChibiOS: Apache 2.0 (includes CMSIS from ARM which is Apache 2.0)
- Unity: MIT License
- CMock: MIT License

See individual repository LICENSE files for details.
