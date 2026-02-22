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

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/* Memory Map Configuration */
#define FLASH_BASE_ADDRESS      0x08000000
#define FLASH_TOTAL_SIZE        (128 * 1024)  /* 128KB */
#define FLASH_PAGE_SIZE         2048          /* 2KB pages */

#define BOOTLOADER_BASE         0x08000000
#define BOOTLOADER_SIZE         (16 * 1024)   /* 16KB */

#define APP_BASE                0x08004000
#define APP_MAX_SIZE            (112 * 1024)  /* 112KB */
#define FLASH_END               (FLASH_BASE_ADDRESS + FLASH_TOTAL_SIZE)

#define RAM_BASE                0x20000000
#define RAM_SIZE                (24 * 1024)   /* 24KB */

/* USB Configuration */
#define USB_PACKET_SIZE         64

/* USB VID/PID Source Configuration
 * When defined: Bootloader reads VID/PID from application header (if valid magic),
 *               falling back to USB_DEFAULT_VID/PID if no valid application.
 * When undefined: Always use USB_DEFAULT_VID and USB_DEFAULT_PID, ignoring
 *                 any application header values.
 */
//#define USE_APP_HEADER_USB_IDS

/* Default USB VID/PID (used when no valid application is present,
 * or when USE_APP_HEADER_USB_IDS is not defined) */
#define USB_DEFAULT_VID         0x0483  /* STMicroelectronics */
#define USB_DEFAULT_PID         0xDF11  /* DFU mode */

/* Bootloader Entry Conditions */
#define BOOTLOADER_MAGIC        0xDEADBEEF
#define BOOTLOADER_MAGIC_ADDR   (RAM_BASE + RAM_SIZE - 4)

/* Application Header Magic */
#define APP_HEADER_MAGIC        0xDEADBEEF

/* Application Memory Layout */
/* ARM Cortex-M0+ requires vector table aligned to 256-byte boundary
 * (next power-of-2 >= vector table size of 192 bytes = 256 bytes)
 * 
 * Layout:
 * 0x08004000: Application header (32 bytes)
 * 0x08004020-0x080040FF: Reserved/padding (224 bytes)
 * 0x08004100: Vector table (256-byte aligned)
 */
#define APP_HEADER_SIZE         32
#define APP_VECTOR_ALIGNMENT    256
#define APP_VECTOR_TABLE_OFFSET 0x100  /* 256 bytes from APP_BASE */

/* Timeouts (in milliseconds) */
#define BOOTLOADER_TIMEOUT_MS   60000  /* 60 seconds - auto-jump to app if no USB activity */

/* Error Codes */
#define ERR_SUCCESS             0
#define ERR_INVALID_PARAM      -1
#define ERR_TIMEOUT            -2
#define ERR_FLASH_UNLOCK       -3
#define ERR_FLASH_ERASE        -4
#define ERR_FLASH_WRITE        -5
#define ERR_INVALID_ADDRESS    -6
#define ERR_INVALID_CRC        -7
#define ERR_USB_ERROR          -8
#define ERR_INVALID_HEADER     -9

#endif /* CONFIG_H */
