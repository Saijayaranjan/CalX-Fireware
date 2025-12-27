/**
 * =============================================================================
 * CalX ESP32 Firmware - Text Renderer Header
 * =============================================================================
 */

#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include "calx_config.h"

/**
 * Initialize text renderer
 */
void text_renderer_init(void);

/**
 * Set content for rendering
 * @param content Text content
 * @param size Text size to use
 */
void text_renderer_set_content(const char *content, calx_text_size_t size);

/**
 * Render content to display at given scroll offset
 * @param scroll_line Starting line (for scrolling)
 */
void text_renderer_render_content(int scroll_line);

/**
 * Get total number of lines for current content
 * @return Number of wrapped lines
 */
int text_renderer_get_line_count(void);

/**
 * Get number of pages for current content
 * @param lines_per_page Lines visible per page
 * @return Number of pages
 */
int text_renderer_get_page_count(int lines_per_page);

/**
 * Word-wrap text to fit display width
 * @param input Input text
 * @param output Output buffer
 * @param max_output_len Maximum output length
 * @param chars_per_line Characters per line
 */
void text_renderer_wrap(const char *input, char *output, int max_output_len,
                        int chars_per_line);

#endif // TEXT_RENDERER_H
