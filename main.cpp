// #define PROGMEM __attribute__((aligned(4), section(".rodata")))

extern "C" {
    #include "ST7735_TFT.h"
    #include "hardware/spi.h"
    #include "hw.h"
    #include "pico/stdlib.h"
}
#include <cstdio>
#include "fonts/CW.h"
#include "fonts/CWMath.h"

// Backlight pin (GPIO 22)
#define TFT_BL 22


void init_hw()
{
    stdio_init_all();
    
    // Initialize backlight pin
    gpio_init(TFT_BL);
    gpio_set_dir(TFT_BL, GPIO_OUT);
    gpio_put(TFT_BL, 1);  // Turn on backlight
    
    spi_init(SPI_PORT, 1000000);
    gpio_set_function(SPI_RX, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_TX, GPIO_FUNC_SPI);
    tft_spi_init();
}

int main() {
    init_hw();
    TFT_ST7735S_Initialize();
    setTextWrap(true);
#ifdef TEST
    while(true) {
        // Test 1: Fill screen with red
        fillScreen(ST7735_RED);
        __delay_ms(500);
        
        // Test 2: Fill screen with green
        fillScreen(ST7735_GREEN);
        __delay_ms(500);
        
        // Test 3: Fill screen with blue
        fillScreen(ST7735_BLUE);
        __delay_ms(500);
        
        // Test 4: Fill screen with white
        fillScreen(ST7735_WHITE);
        __delay_ms(500);
        
        // Test 5: Fill screen black and draw text
        fillScreen(ST7735_BLACK);
        __delay_ms(100);
    }
    drawText(0, 0, "Hello World!", ST7735_WHITE, ST7735_BLACK, 1);
    drawText(0, 10, "Testing CW font", ST7735_YELLOW, ST7735_BLACK, 1);
    setFont(&ClassWiz_CW_Display_Regular12pt);
    drawText(0, 30, "CW 12pt ABC", ST7735_CYAN, ST7735_BLACK, 1);
#endif
}
