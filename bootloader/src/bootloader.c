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

#include "bootloader.h"
#include "config.h"
#include "flash_ops.h"
#include "crc32.h"
#include "usb_dfu.h"
#include "stm32c071xx.h"
#include "ch.h"
#include "hal.h"

#define BOOTLOADER_VERSION 0x00010201  /* Version 1.2.1 */

static bootloader_state_t state = BOOTLOADER_STATE_IDLE;
static systime_t timeout_start = 0;
static bool timeout_enabled = false;

/**
 * @brief Initialize bootloader system
 */
int bootloader_init(void)
{
    // clocks (HSI48 for USB) initialized by ChibiOS startup code
    // USB peripheral will be initialized by USB DFU code when needed

    state = BOOTLOADER_STATE_IDLE;
    
    return ERR_SUCCESS;
}

/**
 * @brief Check if bootloader should enter update mode
 */
bool bootloader_should_enter(void)
{
    /* Check for magic value in RAM */
    volatile uint32_t *magic_ptr = (volatile uint32_t *)BOOTLOADER_MAGIC_ADDR;
    if (*magic_ptr == BOOTLOADER_MAGIC) {
        /* Clear magic value */
        *magic_ptr = 0;
        return true;
    }
    
    /* Check if application is valid */
    if (!bootloader_validate_app()) {
        return true;  /* No valid application, stay in bootloader */
    }
    
    /* Check if user button is pressed (active low - externally pulled up) */
    if (palReadLine(LINE_USER_BUTTON) == PAL_LOW) {
        return true;  /* User button held during reset, enter bootloader */
    }
    
    
    /* TODO: Implement, when watchdog is implemented */
    //if (RCC->CSR2 & RCC_CSR2_IWDGRSTF) {
    //    /* Clear watchdog reset flag to prevent false triggers on next boot */
    //    RCC->CSR2 |= RCC_CSR2_IWDGRSTF;
    //    /* Watchdog reset - enter bootloader */
    //    return true;
    //}
    
    return false;  /* Application is valid, jump to it */
}

/**
 * @brief Initialize bootloader timeout
 */
void bootloader_timeout_init(void)
{
    timeout_start = chVTGetSystemTime();
    timeout_enabled = true;
}

/**
 * @brief Reset bootloader timeout (call on USB activity)
 */
void bootloader_timeout_reset(void)
{
    timeout_start = chVTGetSystemTime();
}

/**
 * @brief Check if bootloader timeout has expired
 * @return true if timeout expired, false otherwise
 */
bool bootloader_timeout_expired(void)
{
    if (!timeout_enabled) {
        return false;
    }
    
    systime_t elapsed = chVTTimeElapsedSinceX(timeout_start);
    sysinterval_t timeout_ticks = TIME_MS2I(BOOTLOADER_TIMEOUT_MS);
    
    /* Check if elapsed time exceeds timeout */
    return elapsed >= timeout_ticks;
}

/**
 * @brief Disable bootloader timeout
 */
void bootloader_timeout_disable(void)
{
    timeout_enabled = false;
}

/**
 * @brief Enable and reset bootloader timeout
 */
void bootloader_timeout_enable(void)
{
    timeout_start = chVTGetSystemTime();
    timeout_enabled = true;
}

/**
 * @brief Run bootloader main loop
 */
void bootloader_run(void)
{
    state = BOOTLOADER_STATE_UPDATING;
    
    /* Initialize timeout */
    bootloader_timeout_init();
    
    /* Main bootloader loop - process DFU commands until download completes */
    while (state == BOOTLOADER_STATE_UPDATING) {
        /* Process USB DFU state machine and flash operations */
        usb_dfu_process();
        
        /* Check if firmware download completed successfully */
        if (usb_dfu_download_complete()) {
            state = BOOTLOADER_STATE_IDLE;
            break;
        }
        
        /* Check timeout - if expired, try to jump to app */
        if (bootloader_timeout_expired()) {
            /* Timeout expired - try to jump to application */
            if (bootloader_validate_app()) {
                /* Valid application exists, jump to it */
                state = BOOTLOADER_STATE_IDLE;
                break;
            }
            /* No valid application - reset timeout and stay in bootloader */
            bootloader_timeout_reset();
        }
        
        /* Yield to ChibiOS scheduler - check every 10ms */
        chThdSleepMilliseconds(10);
    }
}

/**
 * @brief Validate application firmware
 */
bool bootloader_validate_app(void)
{
    const app_header_t *header = (const app_header_t *)APP_BASE;
    
    /* Check magic number */
    if (header->magic != APP_HEADER_MAGIC) {
        return false;
    }
    
    /* Check size */
    if (header->size == 0 || header->size > APP_MAX_SIZE) {
        return false;
    }
    
    /* Verify CRC32
     * CRC is calculated over firmware starting at vector table (0x08004100)
     * NOT from 0x08004020 (old layout)
     */
    uint32_t calc_crc = crc32_calculate(
        (uint8_t *)(APP_BASE + APP_VECTOR_TABLE_OFFSET),
        header->size
    );
    
    if (calc_crc != header->crc32) {
        return false;
    }
    
    return true;
}

/**
 * @brief Jump to application firmware
 */
void bootloader_jump_to_app(void)
{
    /* Validate application before jumping */
    if (!bootloader_validate_app()) {
        return;  /* Invalid application, stay in bootloader */
    }
    
    /* Disable interrupts */
    __disable_irq();
    
    /* Calculate vector table address
     * ARM Cortex-M0+ requires 256-byte alignment for vector tables
     * Vector table is at APP_BASE + 0x100 (256 bytes from start)
     */
    uint32_t vector_table = APP_BASE + APP_VECTOR_TABLE_OFFSET;
    
    /* Relocate vector table to application */
    SCB->VTOR = vector_table;
    
    /* Get application stack pointer and reset handler */
    uint32_t app_stack = *(volatile uint32_t *)vector_table;
    uint32_t app_entry = *(volatile uint32_t *)(vector_table + 4);
    
    /* Set stack pointer */
    __set_MSP(app_stack);
    
    /* Jump to application */
    void (*app_reset_handler)(void) = (void (*)(void))app_entry;
    app_reset_handler();
    
    /* Should never reach here */
}

/**
 * @brief Get bootloader version
 */
uint32_t bootloader_get_version(void)
{
    return BOOTLOADER_VERSION;
}
