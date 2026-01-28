#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/interp.h"

#define SPI_PORT spi0

#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19
#define PIN_DC   20
#define PIN_RST  21

// RGB565 formatted colours
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// send command to lcd screen
void lcd_cmd(uint8_t cmd) {
    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 0);
    spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(PIN_CS, 1);
}

// send data to lcd screen
void lcd_data(uint8_t data) {
    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 1);
    spi_write_blocking(SPI_PORT, &data, 1);
    gpio_put(PIN_CS, 1);
}

// set window area on lcd screen
void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // set column address
    lcd_cmd(0x2A);
    lcd_data(x0 >> 8); lcd_data(x0 & 0xFF);
    lcd_data(x1 >> 8); lcd_data(x1 & 0xFF);

    // set column address
    lcd_cmd(0x2B);
    lcd_data(y0 >> 8); lcd_data(y0 & 0xFF);
    lcd_data(y1 >> 8); lcd_data(y1 & 0xFF);

    // memory write command
    lcd_cmd(0x2C);
}

// draw rectangle on lcd screen
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t colour) {
    lcd_set_window(x, y, x + w - 1, y + h - 1);

    uint8_t hi = colour >> 8;
    uint8_t lo = colour & 0xFF;

    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 1);

    for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
        spi_write_blocking(SPI_PORT, &hi, 1);
        spi_write_blocking(SPI_PORT, &lo, 1);
    }
    gpio_put(PIN_CS, 1);
}

void lcd_init() {
    // reset the chip
    gpio_put(PIN_RST, 1); sleep_ms(5);
    gpio_put(PIN_RST, 0); sleep_ms(20);
    gpio_put(PIN_RST, 1); sleep_ms(150);

    lcd_cmd(0x01); sleep_ms(150); // software Reset

    // power & voltage Settings
    lcd_cmd(0xCB); lcd_data(0x39); lcd_data(0x2C); lcd_data(0x00); lcd_data(0x34); lcd_data(0x02);
    lcd_cmd(0xCF); lcd_data(0x00); lcd_data(0xC1); lcd_data(0x30);
    lcd_cmd(0xE8); lcd_data(0x85); lcd_data(0x00); lcd_data(0x78);
    lcd_cmd(0xEA); lcd_data(0x00); lcd_data(0x00);
    lcd_cmd(0xED); lcd_data(0x64); lcd_data(0x03); lcd_data(0x12); lcd_data(0x81);
    lcd_cmd(0xF7); lcd_data(0x20);
    lcd_cmd(0xC0); lcd_data(0x23);
    lcd_cmd(0xC1); lcd_data(0x10);
    lcd_cmd(0xC5); lcd_data(0x3e); lcd_data(0x28);
    lcd_cmd(0xC7); lcd_data(0x86);

    lcd_cmd(0x36); lcd_data(0x48); // rotation/orientation
    lcd_cmd(0x3A); lcd_data(0x55); // pixel format (16-bit)
    lcd_cmd(0xB1); lcd_data(0x00); lcd_data(0x18);
    lcd_cmd(0xB6); lcd_data(0x08); lcd_data(0x82); lcd_data(0x27);

    lcd_cmd(0x11); sleep_ms(120); // sleep out
    lcd_cmd(0x29); sleep_ms(20);  // display on
}

// kernel main
int main() {
    // init
    stdio_init_all();
    spi_init(SPI_PORT, 10000000); // 10 MHz
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    // initialize GPIO Pins
    gpio_init(PIN_CS); gpio_set_dir(PIN_CS, GPIO_OUT); gpio_put(PIN_CS, 1);
    gpio_init(PIN_DC); gpio_set_dir(PIN_DC, GPIO_OUT); gpio_put(PIN_DC, 1);
    gpio_init(PIN_RST); gpio_set_dir(PIN_RST, GPIO_OUT); gpio_put(PIN_RST, 1);

    // wake up screen
    lcd_init();

    // demo scene
    lcd_fill_rect(0, 0, 240, 320, CYAN); // sky
    lcd_fill_rect(0, 220, 240, 100, GREEN); // ground
    lcd_fill_rect(100, 180, 40, 40, RED); // player

    // infinite loop
    while (1) {
        printf("System running...\n");
        sleep_ms(1000);
    }
}
