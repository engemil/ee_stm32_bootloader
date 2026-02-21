# Bootloader Integration Guide

**Target:** STM32C071RB (128KB Flash, 24KB RAM)  
**Bootloader:** USB DFU 1.1 with DFUSe extensions
**Framework/RTOS:** ChibiOS

Complete guide for integrating (ChibiOS) applications with the STM32C071RB bootloader.



## Table of Contents

1. [Overview](#overview)
2. [Memory Layout](#memory-layout)
3. [Integration Steps](#integration-steps)
4. [Build & Upload](#build--upload)
5. [Bootloader Re-entry](#bootloader-re-entry)
6. [Verification](#verification)
7. [Troubleshooting](#troubleshooting)



## Overview

### What You Need

To make your ChibiOS application bootloader-compatible, you need:

1. **Application header** (32 bytes at 0x08004000)
2. **Custom linker script** (places header and vectors correctly)
3. **Build automation** (auto-sign CRC32 after compilation)

### Why?

The bootloader validates firmware before execution:
- ✅ Checks magic number (0xDEADBEEF)
- ✅ Validates firmware size
- ✅ Verifies CRC32 checksum
- ❌ Won't boot if any check fails

### Memory Layout

```
┌─────────────────────────────────────────┐
│ 0x08000000 - 0x08003FFF: Bootloader     │  16KB (Protected)
├─────────────────────────────────────────┤
│ 0x08004000 - 0x0800401F: App Header     │  32 bytes
│   [0x00] magic:   0xDEADBEEF            │
│   [0x04] version: 0xMMNNPPPP            │
│   [0x08] size:    (auto-signed)         │
│   [0x0C] crc32:   (auto-signed)         │
│   [0x10] usb_vid: USB Vendor ID         │
│   [0x12] usb_pid: USB Product ID        │
│   [0x14] reserved[3]                    │
├─────────────────────────────────────────┤
│ 0x08004020 - 0x080040FF: Padding        │  224 bytes
│   [Reserved for 256-byte alignment]     │
├─────────────────────────────────────────┤
│ 0x08004100 - 0x0801FFFF: Application    │  ~112KB
│   [Vectors + Code + Data]               │
└─────────────────────────────────────────┘
```

**USB VID/PID:** The bootloader reads VID/PID from the application header when entering DFU mode (if valid magic present). This allows the application to define its USB identifiers, used consistently in both DFU mode and normal CDC operation. Default fallback: VID=0x0483, PID=0xDF11.

**Key Rules:**
- ❌ **Never** write to 0x08000000-0x08003FFF (bootloader region)
- ✅ Application **must** start at 0x08004000
- ✅ Vector table **must** be at 0x08004100 (256-byte aligned, ARM Cortex-M0+ requirement!)
- ✅ Padding region (0x08004020-0x080040FF) is reserved for alignment - do not use
- ✅ **Do not** set `SCB->VTOR` manually (ChibiOS handles this)



## Integration Steps


### Step 1: Copy Template Files

Get the template files:
```bash
cd test-firmwares/template/
cp app_header.h app_header.c STM32C071xB_bootloader.ld your_project/
```

Files:
- `app_header.h` - Header structure definition
- `app_header.c` - Header instance (placed at 0x08004000)
- `STM32C071xB_bootloader.ld` - ChibiOS linker script with bootloader support

**NB!** The linker script needs to be modified for your chip!


### Step 2: Update Makefile

Note: If you do not have an `Makefile` from before, look to the `test-firmwares`-folder for example(s).

Open your project's `Makefile` and make these changes:


#### Change 1: Linker Script (around line 124)
```makefile
# BEFORE:
LDSCRIPT= $(STARTUPLD)/STM32C071xB.ld

# AFTER:
LDSCRIPT= STM32C071xB_bootloader.ld
```

#### Change 2: Add app_header.c (around line 128)
```makefile
# BEFORE:
CSRC = $(ALLCSRC) \
       $(TESTSRC) \
       main.c

# AFTER:
CSRC = $(ALLCSRC) \
       $(TESTSRC) \
       main.c \
       app_header.c
```


#### Change 3: Add Auto-Signing (at end of Makefile)

Find the "Custom rules" section and add:

```makefile
##############################################################################
# Custom rules
#

# Path to signing script (adjust if needed)
SIGN_SCRIPT = ../../../scripts/sign_app_header.sh

# Automatic firmware signing after build
POST_MAKE_ALL_RULE_HOOK: sign_firmware

sign_firmware: $(BUILDDIR)/$(PROJECT).bin
	@echo ""
	@echo "=========================================="
	@echo "Signing firmware with size and CRC32..."
	@echo "=========================================="
	@python3 $(SIGN_SCRIPT) $(BUILDDIR)/$(PROJECT).bin $(BUILDDIR)/$(PROJECT)_signed.bin
	@echo ""
	@echo "✓ Build
.PHONY: sign_firmware

#
# Custom rules
##############################################################################
```

**Note:** Adjust `SIGN_SCRIPT` path based on your project location relative to `scripts/sign_app_header.sh` in the workspace root.

### Step 3: Update main.c

Add the app_header include at the top of your `main.c`:

```c
#include "ch.h"
#include "hal.h"
#include "app_header.h"  // <-- Add this line

int main(void) {
    halInit();
    chSysInit();
    
    // Your application code here
    // DO NOT set SCB->VTOR - ChibiOS handles this automatically!
    
    while (true) {
        // Your main loop
    }
}
```

### Step 4: Build

In the folder where the `Makefile` is:

```bash
make clean
make
```

Expected output:
```
...
Linking build/your_project.elf
Creating build/your_project.bin

==========================================
Signing firmware with size and CRC32...
==========================================
============================================================
Application Binary Signing Complete
============================================================
Input file:       build/your_project.bin
Output file:      build/your_project_signed.bin
Total size:       XXXXX bytes
Firmware size:    XXXXX bytes
Magic:            0xDEADBEEF
Version:          0x00010000 (1.0.0)
CRC32:            0xXXXXXXXX
============================================================

✓ Build and signing complete!
  Unsigned: build/your_project.bin (do not upload)
  Signed:   build/your_project_signed.bin (ready to upload)
```



## Build & Upload

### Upload via USB DFU (with USB cable and Bootloader installed)

**Step 1: Enter bootloader**
- Power cycle with no valid app, OR
- Trigger from application (see "Bootloader Re-entry" below), OR  
- Reset after n seconds of inactivity (auto-jump timeout)

**Step 2: Verify device detected**
```bash
sudo dfu-util -l
```

Expected output:
```
Found DFU: [0483:df11] ver=0200, devnum=X, cfg=1, intf=0, path="X-X", alt=0, name="@Internal Flash  /0x08004000/112*001Kg", serial="XXXXXXXXXXXX"
```

**Step 3: Upload firmware**
```bash
sudo dfu-util -a 0 --dfuse-address 0x08004000:leave -D build/your_project_signed.bin
```

The `:leave` suffix makes the device reset automatically after upload.

### Upload via OpenOCD (with debugger (ST-Link) on SWD port)

**For rapid development iterations:**
```bash
sudo openocd -f interface/stlink.cfg -f target/stm32c0x.cfg \
  -c "program build/your_project_signed.bin 0x08004000 verify reset exit"
```

**With debugging session:**
```bash
# Terminal 1: Start OpenOCD
sudo openocd -f interface/stlink.cfg -f target/stm32c0x.cfg

# Terminal 2: GDB session
arm-none-eabi-gdb build/your_project.elf
(gdb) target remote localhost:3333
(gdb) load
(gdb) monitor reset halt
(gdb) continue
```



## Bootloader Re-entry

### Method 1: Magic RAM Value

Write `0xDEADBEEF` to the last 4 bytes of RAM (`0x20005FFC`) then reset. Example:

```c
#include "ch.h"
#include "hal.h"

#define BOOTLOADER_MAGIC_ADDR   0x20005FFC
#define BOOTLOADER_MAGIC        0xDEADBEEF

void enter_bootloader(void) {
    // Disable interrupts
    chSysDisable();
    
    // Write magic value to RAM
    *(volatile uint32_t *)BOOTLOADER_MAGIC_ADDR = BOOTLOADER_MAGIC;
    
    // System reset
    NVIC_SystemReset();
    
    // Never reached
}
```

**Call from your application:**
```c
// Example: Enter bootloader on button press
if (button_pressed()) {
    enter_bootloader();
}
```

The bootloader checks for this magic value on every boot and enters DFU mode if found.

### Method 2: Invalid Firmware

Erase the application region:
```bash
# Via OpenOCD
sudo openocd -f interface/stlink.cfg -f target/stm32c0x.cfg \
  -c "init" -c "halt" -c "flash erase_address 0x08004000 0x1C000" \
  -c "reset" -c "exit"

# Via st-flash
st-flash erase 0x08004000 0x1C000
```

Bootloader detects invalid/missing firmware and stays in DFU mode.

**NB!** If you wipe the bootloader, just flash it again.

### Method 3: User Button

By holder in the User button at power-up or a power-cycle, you can enter the bootloader and stays in DFU mode.

- Disconnect the USB cable (power). Hold in the User button while you reconnect the USB cable, and release.
- Hold in the User button as you press the reset button (with a pin), and release.



## Verification

### Check Binary Structure

**Verify header is present:**
```bash
od -A x -t x4z -N 64 build/your_project_signed.bin
```

Expected output:
```
000000 deadbeef 00010000 <size> <crc32>  >................<
000010 <vid+pid> 00000000 00000000 00000000  >................<
000020 20xxxxxx 08xxxxxx 08xxxxxx 08xxxxxx  > ...............<
000030 08xxxxxx 08xxxxxx 08xxxxxx 08xxxxxx  >................<
```

Key points:
- `0x00`: `0xDEADBEEF` (magic)
- `0x04`: `0x00010000` (version 1.0.0)
- `0x08`: Size in bytes (little-endian)
- `0x0C`: CRC32 (little-endian)
- `0x10`: USB VID (2 bytes, little-endian)
- `0x12`: USB PID (2 bytes, little-endian)
- `0x20`: Initial stack pointer (should be `0x20xxxxxx`)
- `0x24`: Reset handler (should be `0x08xxxxxx`, odd address for Thumb mode)

**Check ELF sections:**
```bash
arm-none-eabi-objdump -h build/your_project.elf | grep -E "app_header|vectors"
```

Expected output:
```
  0 .app_header   00000020  08004000  08004000  00001000  2**2
  1 .vectors      000000c0  08004020  08004020  00001020  2**4
```

**Check firmware size:**
```bash
arm-none-eabi-size -A -d build/your_project.elf
```

Make sure total is < 114,688 bytes (112KB).

### Hardware Checklist

After uploading firmware:

- [ ] Device resets automatically (if using `:leave` option)
- [ ] Application starts within n seconds
- [ ] Application functionality works as expected
- [ ] Can re-enter bootloader via magic RAM method (if implemented)



## Troubleshooting

### Build Issues

**Error: "undefined reference to `__main_thread_stack_base__`"**

Your linker script is missing ChibiOS stack symbols. Solution: Use the provided `STM32C071xB_bootloader.ld` template which includes all required symbols.

**Error: "multiple definition of `app_header`"**

You defined `app_header` in both `app_header.c` AND `main.c`. Solution: Keep it only in `app_header.c`.

**Warning: "section `.vectors` loaded at [08004400]"**

Vector table is at wrong address! You're using the default ChibiOS linker script which has `ALIGN(1024)`. Solution: Use the provided custom linker script with `ALIGN(4)`.

### Upload Issues

**dfu-util: "No DFU capable USB device available"**

Bootloader not running. Solutions:
- Check USB cable (must support data, not just power)
- Enter bootloader manually (erase app or use magic RAM method)
- Try different USB port
- Check `lsusb` output for device

**Bootloader rejects firmware (stays in DFU mode)**

CRC32 validation failed. Solutions:
- Verify you uploaded the **signed** binary (`.._signed.bin`)
- Check binary header with `od` command (see Verification section)
- Rebuild: `make clean && make`

### Runtime Issues

**Application doesn't start (bootloader loops)**

Possible causes:
1. **Invalid vector table address**: Check `.vectors` is at 0x08004100 (256-byte aligned)
   - Verify with: `objdump -h app.elf | grep vectors`
   - VMA should end in 00 or 100 (e.g., 0x08004100)
   - Check alignment: `(address % 256) == 0`
2. **Stack pointer invalid**: Check offset 0x100 in binary is valid RAM address (0x20xxxxxx)
3. **Reset handler invalid**: Check offset 0x104 is odd address in flash (0x08xxxxxx)

Debug:
```bash
sudo openocd -f interface/stlink.cfg -f target/stm32c0x.cfg \
  -c "init" -c "halt" -c "reg pc" -c "exit"
```
PC should be in application range (0x08004000+).

**Application crashes on startup**

Did you set `SCB->VTOR` manually? **Don't!** ChibiOS HAL sets this automatically based on the linker script. Remove any manual `SCB->VTOR` assignments.

<!-- 
**Watchdog resets entering bootloader unexpectedly**

This is intentional! If your application's watchdog times out, the bootloader detects it and enters DFU mode for recovery. This prevents brick scenarios.
-->
