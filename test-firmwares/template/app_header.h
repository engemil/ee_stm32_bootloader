#ifndef APP_HEADER_H
#define APP_HEADER_H

#include <stdint.h>

/**
 * @brief Application header structure for bootloader integration
 * 
 * This header must be placed at 0x08004000 (start of application flash).
 * The bootloader validates this before executing the application.
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;          /* Magic number: 0xDEADBEEF */
    uint32_t version;        /* Firmware version (e.g., 0x00010000 for v1.0.0) */
    uint32_t size;           /* Firmware size in bytes (excluding this header) */
    uint32_t crc32;          /* CRC32 checksum (excluding this header) */
    uint32_t reserved[4];    /* Reserved for future use */
} app_header_t;

/* Constants - customize these for your application */
#define APP_HEADER_MAGIC    0xDEADBEEF
#define APP_VERSION         0x00010000  /* Version 1.0.0 */

#endif /* APP_HEADER_H */
