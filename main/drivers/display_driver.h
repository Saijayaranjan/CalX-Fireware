/**
 * =============================================================================
 * CalX ESP32 Firmware - Display Driver Header
 * =============================================================================
 */

#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include "calx_config.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * Initialize the SSD1306 OLED display
 */
void display_driver_init(void);

/**
 * Clear the display buffer
 */
void display_driver_clear(void);

/**
 * Update display with buffer contents
 */
void display_driver_update(void);

/**
 * Set a pixel in the buffer
 */
void display_driver_set_pixel(int x, int y, bool on);

/**
 * Draw text at position with specified size
 * @param x X position
 * @param y Y position
 * @param text Text to draw
 * @param size Text size (TEXT_SIZE_SMALL, TEXT_SIZE_NORMAL, TEXT_SIZE_LARGE)
 */
void display_driver_draw_text(int x, int y, const char *text,
                              calx_text_size_t size);

/**
 * Draw text centered horizontally
 * @param y Y position
 * @param text Text to draw
 * @param size Text size
 */
void display_driver_draw_text_centered(int y, const char *text,
                                       calx_text_size_t size);

/**
 * Draw a horizontal line
 */
void display_driver_draw_hline(int x, int y, int width);

/**
 * Draw a vertical line
 */
void display_driver_draw_vline(int x, int y, int height);

/**
 * Draw a rectangle outline
 */
void display_driver_draw_rect(int x, int y, int width, int height);

/**
 * Fill a rectangle
 */
void display_driver_fill_rect(int x, int y, int width, int height, bool on);

/**
 * Invert a region (for selection highlight)
 */
void display_driver_invert_rect(int x, int y, int width, int height);

/**
 * Turn display on/off (for power saving)
 */
void display_driver_power(bool on);

/**
 * Set display contrast
 */
void display_driver_set_contrast(uint8_t contrast);

/**
 * Get character width for a text size
 */
int display_driver_get_char_width(calx_text_size_t size);

/**
 * Get line height for a text size
 */
int display_driver_get_line_height(calx_text_size_t size);

#endif // DISPLAY_DRIVER_H
