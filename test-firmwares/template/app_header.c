/*
 * Application Header for EngEmil STM32 Bootloader
 * 
 * Place this at 0x08004000 via linker script's .app_header section.
 * The size and crc32 fields are automatically patched during build.
 */

#include "app_header.h"

__attribute__((section(".app_header")))
__attribute__((used))
const app_header_t app_header = {
    .magic = APP_HEADER_MAGIC,
    .version = APP_VERSION,
    .size = 0,                       /* Auto-patched by build script */
    .crc32 = 0,                      /* Auto-patched by build script */
    .reserved = {0, 0, 0, 0}
};
