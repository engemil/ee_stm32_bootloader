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

/*
 * EngEmil STM32 Bootloader Main Entry Point
 * ChibiOS-based USB DFU Bootloader
 */

#include "ch.h"
#include "hal.h"

#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "bootloader.h"
#include "usb_dfu.h"

/**
 * @brief Main bootloader entry point
 * 
 * This function initializes ChibiOS, checks if bootloader should run,
 * and either enters DFU mode or jumps to application.
 * 
 * Bootloader entry conditions:
 * 1. Magic value in RAM (set by application for firmware update)
 * 2. Invalid application firmware (CRC check fails)
 * 3. User button pressed during reset
 * 4. Watchdog reset detected (commented out until watchdog implemented)
 */
int main(void) {
    int result;

    /*
     * System initializations.
     * - HAL initialization, this also initializes the configured device drivers
     *   and performs the board-specific initializations.
     * - Kernel initialization, the main() function becomes a thread and the
     *   RTOS is active.
     */
    halInit();
    chSysInit();

    /* Initialize bootloader */
    result = bootloader_init();
    if (result != ERR_SUCCESS) {
        /* Fatal error - stay in bootloader */
        while (true) {
            /* Blink error pattern or just halt */
            chThdSleepMilliseconds(1000);
        }
    }

    /* Check bootloader entry conditions */
    if (bootloader_should_enter()) {
        /* Initialize USB DFU */
        usb_dfu_init();

        /* Run bootloader - wait for firmware update via USB DFU */
        bootloader_run();

        /* After successful firmware update, reset to boot new firmware */
        NVIC_SystemReset();
    }

    /* Validate application before jumping */
    if (bootloader_validate_app()) {
        /* Deinitialize ChibiOS and jump to application */
        chSysDisable();
        bootloader_jump_to_app();
    }

    /* If we reach here, application jump failed or validation failed */
    /* Initialize USB DFU and stay in bootloader mode */
    usb_dfu_init();
    bootloader_run();

    /* Should never reach here */
    while (true) {
        chThdSleepMilliseconds(1000);
    }

    return 0;
}
