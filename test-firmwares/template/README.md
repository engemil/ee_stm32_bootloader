# EngEmil STM32 Bootloader Integration Template

Minimal files needed to integrate your ChibiOS application with the EngEmil STM32 Bootloader.

## Quick Start (3 Steps)

### 1. Copy Template Files

```bash
# Copy to your project
cp app_header.h your_project/
cp app_header.c your_project/
cp STM32C071xB_bootloader.ld your_project/
```

### 2. Update Your Makefile

Open `Makefile.snippet` and apply the 4 changes to your project's Makefile:
- Change linker script to `STM32C071xB_bootloader.ld`
- Add `app_header.c` to `CSRC`
- Add post-build patching rules
- Adjust `PATCH_SCRIPT` path if needed (include also the script `scripts/patch_app_header.py`)

### 3. Update Your main.c

Add this include at the top:
```c
#include "app_header.h"
```

### 4. Build

```bash
make clean
make
```

Output files:
- `build/your_project.bin` - Unpatched (do not upload)
- `build/your_project_patched.bin` - **Upload this one!**

## Upload Firmware

### Via DFU Bootloader (Production)
```bash
sudo dfu-util -a 0 --dfuse-address 0x08004000:leave -D build/your_project_patched.bin
```

### Via OpenOCD (Development)
```bash
sudo openocd -f interface/stlink.cfg -f target/stm32c0x.cfg \
  -c "program build/your_project_patched.bin 0x08004000 verify reset exit"
```

## File Descriptions

### app_header.h
Header file defining the bootloader-required structure. Customize `APP_VERSION` as needed.

### app_header.c
Implementation that places the header at 0x08004000. The `size` and `crc32` fields are auto-patched during build.

### STM32C071xB_bootloader.ld
Complete ChibiOS-compatible linker script with:
- Application flash starting at 0x08004100 (256-byte aligned)
- `.app_header` section at 0x08004000
- Padding region 0x08004020-0x080040FF (for alignment)
- `.vectors` aligned to 256 bytes (ARM Cortex-M0+ requirement)
- All required ChibiOS symbols

### Makefile.snippet
Example changes needed in your Makefile. Copy-paste the relevant sections.

## Detailed Guide

For complete documentation, see:
- `docs/BOOTLOADER_INTEGRATION.md`
