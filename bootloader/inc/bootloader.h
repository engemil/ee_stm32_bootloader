/*
MIT License

Copyright (c) 2026 EngEmil

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Application header structure
 * 
 * This header must be placed at the start of the application (0x08004000)
 * to enable firmware validation.
 * 
 * The usb_vid and usb_pid fields allow the application to specify USB
 * identifiers that the bootloader will use in DFU mode. If no valid
 * application is present (magic != 0xDEADBEEF), the bootloader falls
 * back to default VID/PID values defined in config.h.
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;          /* Magic number: 0xDEADBEEF */
    uint32_t version;        /* Firmware version */
    uint32_t size;           /* Firmware size in bytes */
    uint32_t crc32;          /* CRC32 of firmware (excluding this header) */
    uint16_t usb_vid;        /* USB Vendor ID */
    uint16_t usb_pid;        /* USB Product ID */
    uint32_t reserved[3];    /* Reserved for future use */
} app_header_t;

/**
 * @brief Bootloader state
 */
typedef enum {
    BOOTLOADER_STATE_IDLE,
    BOOTLOADER_STATE_UPDATING
} bootloader_state_t;

/**
 * @brief Initialize bootloader system
 * 
 * Initializes clocks, GPIO, USB, and other peripherals.
 * 
 * @return 0 on success, negative error code on failure
 */
int bootloader_init(void);

/**
 * @brief Check if bootloader should enter update mode
 * 
 * Checks various conditions:
 * - Magic value in RAM
 * - Invalid application firmware
 * - User button pressed
 * - Watchdog reset (commented out until watchdog implemented)
 * 
 * @return true if bootloader should run, false otherwise
 */
bool bootloader_should_enter(void);

/**
 * @brief Run bootloader main loop
 * 
 * Enters update mode and waits for firmware via USB DFU.
 * This function blocks until update is complete or timeout.
 */
void bootloader_run(void);

/**
 * @brief Validate application firmware
 * 
 * Checks application header and verifies CRC32.
 * 
 * @return true if application is valid, false otherwise
 */
bool bootloader_validate_app(void);

/**
 * @brief Jump to application firmware
 * 
 * Relocates vector table and transfers control to application.
 * This function does not return if successful.
 */
void bootloader_jump_to_app(void);

/**
 * @brief Get bootloader version
 * 
 * @return Bootloader version as uint32_t
 */
uint32_t bootloader_get_version(void);

/**
 * @brief Initialize bootloader timeout
 * 
 * Starts the timeout countdown timer.
 */
void bootloader_timeout_init(void);

/**
 * @brief Reset bootloader timeout
 * 
 * Resets the timeout timer to zero. Should be called on USB activity.
 */
void bootloader_timeout_reset(void);

/**
 * @brief Check if bootloader timeout has expired
 * 
 * @return true if timeout expired, false otherwise
 */
bool bootloader_timeout_expired(void);

/**
 * @brief Disable bootloader timeout
 * 
 * Disables the timeout mechanism (timeout will never expire).
 */
void bootloader_timeout_disable(void);

/**
 * @brief Enable and reset bootloader timeout
 * 
 * Re-enables the timeout mechanism and resets the timer.
 * Same as bootloader_timeout_init().
 */
void bootloader_timeout_enable(void);

#endif /* BOOTLOADER_H */
