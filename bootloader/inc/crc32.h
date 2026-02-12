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

#ifndef CRC32_H
#define CRC32_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Calculate CRC32 checksum
 * 
 * @param data Pointer to data buffer
 * @param len Length of data in bytes
 * @return CRC32 checksum
 */
uint32_t crc32_calculate(const uint8_t *data, size_t len);

/**
 * @brief Initialize CRC32 calculation
 * 
 * @return Initial CRC value
 */
uint32_t crc32_init(void);

/**
 * @brief Update CRC32 with new data
 * 
 * @param crc Current CRC value
 * @param data Pointer to data buffer
 * @param len Length of data in bytes
 * @return Updated CRC value
 */
uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len);

/**
 * @brief Finalize CRC32 calculation
 * 
 * @param crc Current CRC value
 * @return Final CRC value
 */
uint32_t crc32_finalize(uint32_t crc);

#endif /* CRC32_H */
