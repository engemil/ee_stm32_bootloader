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
