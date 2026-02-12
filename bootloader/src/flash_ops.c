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

#include "flash_ops.h"
#include "config.h"
#include "stm32c071xx.h"

/* Flash unlock keys */
#define FLASH_KEY1  0x45670123
#define FLASH_KEY2  0xCDEF89AB

/**
 * @brief Wait for flash operation to complete
 */
static int flash_wait_ready(void)
{
    uint32_t timeout = 100000;  /* Approximate timeout */
    
    while ((FLASH->SR & FLASH_SR_BSY1) && timeout > 0) {
        timeout--;
    }
    
    if (timeout == 0) {
        return ERR_TIMEOUT;
    }
    
    /* Check for errors */
    if (FLASH->SR & (FLASH_SR_WRPERR | FLASH_SR_PROGERR)) {
        /* Clear error flags */
        FLASH->SR = FLASH_SR_WRPERR | FLASH_SR_PROGERR;
        return ERR_FLASH_WRITE;
    }
    
    /* Clear EOP flag (success indicator) */
    if (FLASH->SR & FLASH_SR_EOP) {
        FLASH->SR = FLASH_SR_EOP;
    }
    
    return ERR_SUCCESS;
}

/**
 * @brief Unlock flash for programming
 */
int flash_unlock(void)
{
    /* Check if already unlocked */
    if (!(FLASH->CR & FLASH_CR_LOCK)) {
        return ERR_SUCCESS;
    }
    
    /* Unlock flash */
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
    
    /* Verify unlock */
    if (FLASH->CR & FLASH_CR_LOCK) {
        return ERR_FLASH_UNLOCK;
    }
    
    return ERR_SUCCESS;
}

/**
 * @brief Lock flash after programming
 */
int flash_lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
    return ERR_SUCCESS;
}

/**
 * @brief Erase flash pages
 */
int flash_erase_pages(uint32_t addr, size_t len)
{
    /* Calculate number of pages */
    uint32_t start_page = (addr - FLASH_BASE_ADDRESS) / FLASH_PAGE_SIZE;
    uint32_t num_pages = (len + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;
    
    for (uint32_t i = 0; i < num_pages; i++) {
        /* Wait for previous operation */
        int result = flash_wait_ready();
        if (result != ERR_SUCCESS) {
            return result;
        }
        
        /* Set page erase */
        FLASH->CR |= FLASH_CR_PER;
        FLASH->CR &= ~FLASH_CR_PNB;
        FLASH->CR |= ((start_page + i) << FLASH_CR_PNB_Pos);
        
        /* Start erase */
        FLASH->CR |= FLASH_CR_STRT;
        
        /* Wait for completion */
        result = flash_wait_ready();
        if (result != ERR_SUCCESS) {
            FLASH->CR &= ~FLASH_CR_PER;
            return result;
        }
        
        /* Clear page erase bit */
        FLASH->CR &= ~FLASH_CR_PER;
    }
    
    return ERR_SUCCESS;
}

/**
 * @brief Write double-word (64-bit / 8 bytes) to flash
 * @note STM32C0 flash requires writing 64 bits at a time
 */
int flash_write_doubleword(uint32_t addr, uint32_t word1, uint32_t word2)
{
    int result;
    
    /* Wait for flash ready */
    result = flash_wait_ready();
    if (result != ERR_SUCCESS) {
        return result;
    }
    
    /* Ensure no other operation bits are set */
    FLASH->CR &= ~(FLASH_CR_PER | FLASH_CR_MER1);
    
    /* Enable programming */
    FLASH->CR |= FLASH_CR_PG;
    
    /* Write first 32-bit word */
    *(volatile uint32_t *)addr = word1;
    
    /* Barrier to ensure programming is performed in 2 steps */
    __ISB();
    
    /* Write second 32-bit word */
    *(volatile uint32_t *)(addr + 4) = word2;
    
    /* Wait for operation to complete */
    result = flash_wait_ready();
    if (result != ERR_SUCCESS) {
        FLASH->CR &= ~FLASH_CR_PG;
        return result;
    }
    
    /* Disable programming */
    FLASH->CR &= ~FLASH_CR_PG;
    
    /* Verify */
    if (*(volatile uint32_t *)addr != word1 || *(volatile uint32_t *)(addr + 4) != word2) {
        return ERR_FLASH_WRITE;
    }
    
    return ERR_SUCCESS;
}

/**
 * @brief Write word to flash (wrapper for backward compatibility)
 * @note Writes 8 bytes (pads with 0xFFFFFFFF for second word)
 */
int flash_write_word(uint32_t addr, uint32_t word)
{
    return flash_write_doubleword(addr, word, 0xFFFFFFFF);
}

/**
 * @brief Write data to flash
 * @note STM32C0 requires writing 64 bits (8 bytes) at a time
 */
int flash_write(uint32_t addr, const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return ERR_INVALID_PARAM;
    }
    
    /* Write double-word (8 bytes) by double-word */
    for (size_t i = 0; i < len; i += 8) {
        uint32_t word1, word2;
        
        /* Build first word from bytes (safe for unaligned access) */
        if (i + 4 <= len) {
            word1 = ((uint32_t)data[i + 0] << 0)  |
                   ((uint32_t)data[i + 1] << 8)  |
                   ((uint32_t)data[i + 2] << 16) |
                   ((uint32_t)data[i + 3] << 24);
        } else {
            /* Handle partial word at end */
            word1 = 0xFFFFFFFF;  /* Flash erased value */
            for (size_t j = 0; j < (len - i) && j < 4; j++) {
                word1 &= ~(0xFF << (j * 8));
                word1 |= ((uint32_t)data[i + j] << (j * 8));
            }
        }
        
        /* Build second word from bytes */
        if (i + 8 <= len) {
            word2 = ((uint32_t)data[i + 4] << 0)  |
                   ((uint32_t)data[i + 5] << 8)  |
                   ((uint32_t)data[i + 6] << 16) |
                   ((uint32_t)data[i + 7] << 24);
        } else if (i + 4 < len) {
            /* Partial second word */
            word2 = 0xFFFFFFFF;
            for (size_t j = 0; j < (len - i - 4); j++) {
                word2 &= ~(0xFF << (j * 8));
                word2 |= ((uint32_t)data[i + 4 + j] << (j * 8));
            }
        } else {
            /* No second word data */
            word2 = 0xFFFFFFFF;
        }
        
        int result = flash_write_doubleword(addr + i, word1, word2);
        if (result != ERR_SUCCESS) {
            return result;
        }
    }
    
    return ERR_SUCCESS;
}

/**
 * @brief Check if address is in application region
 */
bool flash_is_app_region(uint32_t addr, size_t len)
{
    if (addr < APP_BASE || addr >= FLASH_END) {
        return false;
    }
    
    if (addr + len > FLASH_END) {
        return false;
    }
    
    /* Ensure not writing to bootloader */
    if (addr < APP_BASE) {
        return false;
    }
    
    return true;
}


