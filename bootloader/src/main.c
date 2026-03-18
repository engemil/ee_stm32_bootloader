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
    if (bootloader_init() != ERR_SUCCESS) {
        /* Fatal error - halt */
        while (true) {
            chThdSleepMilliseconds(1000);
        }
    }

    /* Fast path: no reason to enter bootloader, app is valid -> jump */
    if (!bootloader_should_enter()) {
        chSysDisable();
        bootloader_jump_to_app();
        /* bootloader_jump_to_app() only returns if validation fails,
         * fall through to DFU mode */
        chSysInit();
    }

    /* Enter DFU mode */
    usb_dfu_init();

    for (;;) {
        bool timeout_exit = bootloader_run();

        if (!timeout_exit) {
            /* Download completed - reset to boot new firmware cleanly */
            NVIC_SystemReset();
        }

        /* Inactivity timeout with valid app - tear down USB and jump */
        usbDisconnectBus(&USBD1);
        usbStop(&USBD1);
        chSysDisable();
        bootloader_jump_to_app();

        /* bootloader_jump_to_app() only returns if validation fails.
         * Re-init and loop back to DFU mode. */
        chSysInit();
        usb_dfu_init();
    }
}
