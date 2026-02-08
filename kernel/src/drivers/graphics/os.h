#ifndef KERNEL_GRAPHICS_OS_H
#define KERNEL_GRAPHICS_OS_H

#include <stdint.h>
#include <stdbool.h>

// RGB565 formatted colours
#define BLACK    0x0000
#define DARKGREY 0x2104
#define WHITE    0xFFFF
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0

void draw_menu_item(int y, int index, bool selected);
void draw_menu();

#endif
