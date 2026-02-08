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
