#include "os.h"

#include "lcd.h"

/**
 * Render a single menu item row at the given vertical position.
 *
 * Draws a 160x40 item box at x=40 and the small left indicator bar at x=20.
 *
 * @param y Vertical pixel coordinate where the top of the menu item is drawn.
 * @param index Index of the menu item in the menu (logical identifier; not used for layout).
 * @param selected If `true`, the item is drawn in its selected state and the left indicator is highlighted; if `false`, the item is drawn in its normal state.
 */
void draw_menu_item(int y, int index, bool selected) {
	uint16_t box_colour = selected ? WHITE : DARKGREY;
	uint16_t dot_colour = selected ? RED : DARKGREY;

	lcd_fill_rect(40, y, 160, 40, box_colour);

	if (selected) {
		lcd_fill_rect(20, y + 10, 10, 20, YELLOW);
	} else {
		lcd_fill_rect(20, y + 10, 10, 20, BLACK);
	}
}

/**
 * Render the basic menu background and header bar.
 *
 * Clears the display to BLACK and draws a BLUE header bar across the top of the screen.
 */
void draw_menu() {
	// clear screen
	lcd_fill_rect(0, 0, 240, 320, BLACK);

	// fake OS header
	lcd_fill_rect(0, 0, 240, 30, BLUE);
}