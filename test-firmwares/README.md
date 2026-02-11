# Test Application Firmwares

Test application firmware(s) and template.

### led_test_app_fw/
**Purpose:** LED test application  
**Features:**
- Full ChibiOS RTOS integration
- Bootloader-compatible

**See:** [led_test_app_fw/README.md](led_test_app_fw/README.md)

### ws2812b_led_test_app_fw/
**Purpose:** ChibiOS + WS2812B LED test application  
**Features:**
- Full ChibiOS RTOS integration
- WS2812B RGB LED driver (cycles through colors)
- Bootloader-compatible

**See:** [ws2812b_led_test_app_fw/README.md](ws2812b_led_test_app_fw/README.md)

### template/
**Purpose:** Ready-to-use template for bootloader integration  
**Contents:**
- Application header files (app_header.h/c)
- ChibiOS bootloader-compatible linker script
- Makefile snippet with auto-patching
- Quick reference guide (README.md)

**Use the template to integrate your own firmware with the bootloader.**

**See:** [template/README.md](template/README.md)



## Quick Start

### Build Test Firmware
```bash
cd led_test_app_fw/application && make clean && make
```

### Upload via USB DFU (Bootloader)
```bash
# First upload the bootloader (see other notes)

# Upload application firmware over USB
cd test-firmwares/led_test_app_fw/application
sudo dfu-util -a 0 --dfuse-address 0x08004000:leave -D build/led-test-app-fw_patched.bin
```


## Common Memory Layout

All test firmwares share the same memory layout:

```
Application Region: 0x08004000 - 0x0801FFFF (112KB)

├─ 0x08004000 - 0x0800401F : Application Header (32 bytes)
│   ├─ +0x00: magic (0xDEADBEEF)
│   ├─ +0x04: version
│   ├─ +0x08: size
│   ├─ +0x0C: crc32
│   └─ +0x10: reserved[4]
│
├─ 0x08004020 - 0x080040FF : Padding (224 bytes, for 256-byte alignment)
│   └─ Reserved for vector table alignment (NOT included in CRC)
│
└─ 0x08004100 - 0x0801FFFF : Vector Table + Code (256-byte aligned)
    ├─ +0x00: Initial Stack Pointer
    ├─ +0x04: Reset Handler
    ├─ +0x08+: Exception/Interrupt Vectors
    └─ Application code follows

RAM: 0x20000000 - 0x20005FFF (24KB)
```


## Bootloader Re-entry

All test firmwares support bootloader re-entry via magic RAM value. Example:

```c
#define BOOTLOADER_MAGIC_ADDR  ((uint32_t*)0x20005FFC)
#define BOOTLOADER_MAGIC       0xDEADBEEF

void enter_bootloader(void) {
    *BOOTLOADER_MAGIC_ADDR = BOOTLOADER_MAGIC;
    NVIC_SystemReset();
}
```


## Build System

Test firmware uses:
- **Toolchain:** ARM GCC (`arm-none-eabi-gcc`)
- **Build Tool:** GNU Make
- **CRC32 Patching:** `scripts/patch_app_header.py` (in workspace root)
