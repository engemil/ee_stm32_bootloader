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

#ifndef FLASH_OPS_H
#define FLASH_OPS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Unlock flash for programming
 * 
 * @return 0 on success, negative error code on failure
 */
int flash_unlock(void);

/**
 * @brief Lock flash after programming
 * 
 * @return 0 on success, negative error code on failure
 */
int flash_lock(void);

/**
 * @brief Erase flash pages
 * 
 * @param addr Start address (must be page-aligned)
 * @param len Number of bytes to erase
 * @return 0 on success, negative error code on failure
 */
int flash_erase_pages(uint32_t addr, size_t len);

/**
 * @brief Write data to flash
 * 
 * @param addr Destination address (must be aligned to 8 bytes)
 * @param data Source data buffer
 * @param len Number of bytes to write (must be multiple of 8)
 * @return 0 on success, negative error code on failure
 */
int flash_write(uint32_t addr, const uint8_t *data, size_t len);

/**
 * @brief Write word to flash
 * 
 * @param addr Destination address (must be 4-byte aligned)
 * @param word Word to write
 * @return 0 on success, negative error code on failure
 */
int flash_write_word(uint32_t addr, uint32_t word);

/**
 * @brief Check if address is in application region
 * 
 * @param addr Address to check
 * @param len Length in bytes
 * @return true if address is in application region, false otherwise
 */
bool flash_is_app_region(uint32_t addr, size_t len);

#endif /* FLASH_OPS_H */
