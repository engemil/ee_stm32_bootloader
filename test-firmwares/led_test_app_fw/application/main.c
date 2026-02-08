#include "ch.h"
#include "hal.h"
#include "app_header.h"

int main(void) {
    halInit();
    chSysInit();
    while (true) {
        palSetPad(GPIOA, 5);
        chThdSleepMilliseconds(3000);
        palClearPad(GPIOA, 5);
        chThdSleepMilliseconds(3000);
    }
}
