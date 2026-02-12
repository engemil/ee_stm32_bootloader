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

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "ee_ws2812b_chibios_driver.h"
#include "app_header.h"

int main(void) {
    halInit();
    chSysInit();
    ee_ws2812b_init_driver();
    while (true) {
        
        //palSetPad(GPIOA, 5);
        //chThdSleepMilliseconds(3000);
        //palClearPad(GPIOA, 5);
        //chThdSleepMilliseconds(3000);
        
        // NB! The first bit is the LSB, not the MSB, hence 0x80 is the same as 1 for the LED.
        // Add a function to handle MSB/LSB first in the ee_ws2812b driver, and add option to initialize with a bool for MSB/LSBfirst and a default of LSBfirst.
        ee_ws2812b_set_color_rgb_and_render(0xFF, 0x00, 0x00); //ee_ws2812b_set_color_rgb_and_render(0x80, 0x00, 0x00);
        chThdSleepMilliseconds(500);
        ee_ws2812b_set_color_rgb_and_render(0x00, 0xFF, 0x00); //ee_ws2812b_set_color_rgb_and_render(0x00, 0x80, 0x00);
        chThdSleepMilliseconds(500);
        ee_ws2812b_set_color_rgb_and_render(0x00, 0x00, 0xFF); //ee_ws2812b_set_color_rgb_and_render(0x00, 0x00, 0x80);
        chThdSleepMilliseconds(500);
    }
}
