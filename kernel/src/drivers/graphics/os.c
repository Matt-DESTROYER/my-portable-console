#include "os.h"

#include "lcd.h"

void draw_menu_item(int y, int index, bool selected) {
	uint16_t box_colour = selected ? WHITE : DARKGREY;

	lcd_fill_rect(40, y, 160, 40, box_colour);

	if (selected) {
		lcd_fill_rect(20, y + 10, 10, 20, YELLOW);
	} else {
		lcd_fill_rect(20, y + 10, 10, 20, BLACK);
	}
}

void draw_menu() {
	// clear screen
	lcd_fill_rect(0, 0, 240, 320, BLACK);

	// fake OS header
	lcd_fill_rect(0, 0, 240, 30, BLUE);
}
