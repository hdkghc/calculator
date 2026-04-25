#define PROGMEM __attribute__((aligned(4), section(".rodata")))

extern "C" {
    #include "ST7735_TFT.h"
    #include "hardware/spi.h"
    #include "hw.h"
    #include "pico/stdlib.h"
}
#include <cstdio>
#include "fonts/CW.h"
#include "fonts/CWMath.h"

void init_hw()
{
    stdio_init_all();
    spi_init(SPI_PORT, 1000000);
    gpio_set_function(SPI_RX, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_TX, GPIO_FUNC_SPI);
    tft_spi_init();
}

int main() {
    init_hw();
    TFT_BlackTab_Initialize();
    setTextWrap(true);
#ifdef TEST
    fillScreen(ST7735_BLACK);
    drawText(0, 0, "Hello World!", ST7735_WHITE, ST7735_BLACK, 1);
    drawText(0, 10, "Testing CW font", ST7735_YELLOW, ST7735_BLACK, 1);
    setFont(&ClassWiz_CW_Display_Regular12pt);
    drawText(0, 30, "CW 12pt ABCDEFGHIJKLMNOPGRSTUVWXYZabcdefghijklmnopqrstuvwxyz", ST7735_CYAN, ST7735_BLACK, 1);
#endif
}
