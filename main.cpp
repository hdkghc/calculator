#define TFT_ENABLE_TEXT
#define TFT_ENABLE_FONTS

#include "ST7735_TFT.h"
#include "hardware/spi.h"
#include "hw.h"
#include "pico/stdlib.h"
#include <cstdio>

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
    
}
