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
