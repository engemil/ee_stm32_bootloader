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

#include "crc32.h"
#include <stdbool.h>

/* CRC32 polynomial (IEEE 802.3) */
#define CRC32_POLYNOMIAL  0xEDB88320

/* CRC32 lookup table for faster calculation */
static uint32_t crc32_table[256];
static bool table_initialized = false;

/**
 * @brief Initialize CRC32 lookup table
 */
static void crc32_init_table(void)
{
    if (table_initialized) {
        return;
    }
    
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (uint32_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
    
    table_initialized = true;
}

/**
 * @brief Initialize CRC32 calculation
 */
uint32_t crc32_init(void)
{
    crc32_init_table();
    return 0xFFFFFFFF;
}

/**
 * @brief Update CRC32 with new data
 */
uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return crc;
    }
    
    for (size_t i = 0; i < len; i++) {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ crc32_table[index];
    }
    
    return crc;
}

/**
 * @brief Finalize CRC32 calculation
 */
uint32_t crc32_finalize(uint32_t crc)
{
    return crc ^ 0xFFFFFFFF;
}

/**
 * @brief Calculate CRC32 checksum (convenience function)
 */
uint32_t crc32_calculate(const uint8_t *data, size_t len)
{
    uint32_t crc = crc32_init();
    crc = crc32_update(crc, data, len);
    return crc32_finalize(crc);
}
