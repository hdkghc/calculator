// #define PROGMEM __attribute__((aligned(4), section(".rodata")))

extern "C" {
    #include <hardware/spi.h>
    #include <pico/stdlib.h>
}
// #include "fonts/CW.h"
// #include "fonts/CWMath.h"

using namespace std;

int main() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    bool flg = false;
    while(true) {
        gpio_put(PICO_DEFAULT_LED_PIN, flg = !flg);
        sleep_ms(1000);
    }
}
