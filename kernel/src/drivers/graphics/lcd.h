#ifndef KERNEL_GRAPHICS_LCD_H
#define KERNEL_GRAPHICS_LCD_H

#include <stdbool.h>
#include <stdint.h>

void lcd_cmd(uint8_t cmd);
void lcd_data(uint8_t data);
void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t colour);
void lcd_init();

#endif
