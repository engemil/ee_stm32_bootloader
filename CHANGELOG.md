# Changelog

All notable changes to the EngEmil STM32 Bootloader project will be documented in this file.

**Version Format:** MAJOR.MINOR.PATCH
- **MAJOR:** Incompatible API/protocol changes
- **MINOR:** New features (backward compatible)
- **PATCH:** Bug fixes (backward compatible)

[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Note: Update `BOOTLOADER_VERSION` in `bootloader.c` when publishing new version.

---

## [1.2.1] - (2026-02-22)

Added
- `USE_APP_HEADER_USB_IDS` macro to control USB VID/PID source behavior
  - When defined: Read VID/PID from app header, fallback to defaults
  - When undefined: Always use `USB_DEFAULT_VID` and `USB_DEFAULT_PID`


## [1.2.0] - (2026-02-21)

Added
- USB PID and VID values linked to application header.
- Default USB PID and VID values.
- Changed USB descriptor.


## [1.1.2] - (2026-02-11)

Added
- Added a "To Do", to change VID and PID for USB. Not yet clearified if `0483:df11` can be used.

Changed
- Changed USB Descriptor to show: `EngEmil.io Bootloader DFU Mode`

## [1.1.1] - (2026-02-11)

Changed
- Renamed post-compile script "patch" to "signed".
- Replaced `sign_app_header`-script from python to bash. Removing python as a dependency.


## [1.1.0] - (2026-02-11)

Added
- **Custom board file** to bootloader.
- New **Bootloader entry mode**: User button at start-up/power-cycle (was not yet implemented).

Changed
- Removed **Bootloader entry mode**: Watchdog reset detection removed/commented out (for now).
- Adjusted information in Markdown files.


## [1.0.0] - (2026-02-08)

Added
- **USB DFU 1.1 Protocol** with full compliance
  - VID:PID 0x0483:0xDF11 (STMicroelectronics DFU mode) (Must be changed later)
  - Transfer size: 1024 bytes per block
- **DFUSe Extensions** (ST-specific)
  - Set Address Pointer (0x21) for non-sequential writes
  - Erase Page (0x41) with auto-erase support
- **Application validation** via magic number (0xDEADBEEF), size, and CRC32 (IEEE 802.3)
- **Multiple bootloader entry modes**
  - Magic RAM value (0x20005FFC = 0xDEADBEEF)
  - Invalid firmware detection
  - Watchdog reset detection
- **Auto-jump timeout** - Automatically boots from bootloader to application after inactivity
- **Application header structure** (32-byte) with post-build patching script
- **Bootloader protection** - Address validation prevents self-overwrite
- **HSI48 crystal-less USB** with Clock Recovery System (CRS)
- **Memory layout** - 16KB bootloader, 112KB application space
- **ChibiOS RTOS integration** (master)
- **Custom linker scripts** (`STM32C071xB_bootloader.ld`). One for bootloader and one for application.
- **Integration Instructions** (`BOOTLOADER_INTEGRAITON.md`)
- **template** and **test application firmware**
- **VS Code tasks** for build, flash, debug workflows.
- **devcontainer and Docker files** for containerized development (`.devcontainer`-folder)
- **AI agent development files**, `opencode.json` and `AGENTS.md`
- **Static Analysis** (cppcheck) and zero-warning policy

---


