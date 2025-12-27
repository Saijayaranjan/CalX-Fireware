/**
 * =============================================================================
 * CalX ESP32 Firmware - Text Renderer
 * =============================================================================
 * Word wrapping, pagination, and text rendering for OLED display.
 * =============================================================================
 */

#include "text_renderer.h"
#include "calx_config.h"
#include "display_driver.h"
#include <ctype.h>
#include <string.h>

// =============================================================================
// Configuration
// =============================================================================
#define MAX_CONTENT_SIZE 4096
#define MAX_LINES 100
#define MAX_LINE_LENGTH 32

// =============================================================================
// State
// =============================================================================
static char content_buffer[MAX_CONTENT_SIZE];
static char *line_starts[MAX_LINES];
static int line_lengths[MAX_LINES];
static int total_lines = 0;
static calx_text_size_t current_size = TEXT_SIZE_NORMAL;

// =============================================================================
// Initialization
// =============================================================================

void text_renderer_init(void) {
  memset(content_buffer, 0, sizeof(content_buffer));
  memset(line_starts, 0, sizeof(line_starts));
  total_lines = 0;
}

// =============================================================================
// Get Characters Per Line
// =============================================================================

static int get_chars_per_line(calx_text_size_t size) {
  switch (size) {
  case TEXT_SIZE_SMALL:
    return TEXT_SMALL_CHARS_LINE;
  case TEXT_SIZE_NORMAL:
    return TEXT_NORMAL_CHARS_LINE;
  case TEXT_SIZE_LARGE:
    return TEXT_LARGE_CHARS_LINE;
  default:
    return TEXT_NORMAL_CHARS_LINE;
  }
}

static int get_lines_per_screen(calx_text_size_t size) {
  switch (size) {
  case TEXT_SIZE_SMALL:
    return TEXT_SMALL_LINES;
  case TEXT_SIZE_NORMAL:
    return TEXT_NORMAL_LINES;
  case TEXT_SIZE_LARGE:
    return TEXT_LARGE_LINES;
  default:
    return TEXT_NORMAL_LINES;
  }
}

// =============================================================================
// Word Wrapping
// =============================================================================

void text_renderer_wrap(const char *input, char *output, int max_output_len,
                        int chars_per_line) {
  int in_pos = 0;
  int out_pos = 0;
  int line_pos = 0;
  int last_space = -1;
  int last_space_out = -1;

  while (input[in_pos] && out_pos < max_output_len - 1) {
    char c = input[in_pos];

    // Handle newlines in input
    if (c == '\n') {
      output[out_pos++] = '\n';
      line_pos = 0;
      last_space = -1;
      in_pos++;
      continue;
    }

    // Track last space for word wrapping
    if (c == ' ') {
      last_space = in_pos;
      last_space_out = out_pos;
    }

    // Check if we need to wrap
    if (line_pos >= chars_per_line) {
      // Try to wrap at last space
      if (last_space > 0 && last_space_out > 0) {
        // Replace space with newline
        output[last_space_out] = '\n';
        line_pos = out_pos - last_space_out - 1;
      } else {
        // Hard wrap
        output[out_pos++] = '\n';
        line_pos = 0;
      }
      last_space = -1;
      last_space_out = -1;
    }

    output[out_pos++] = c;
    line_pos++;
    in_pos++;
  }

  output[out_pos] = '\0';
}

// =============================================================================
// Set Content
// =============================================================================

void text_renderer_set_content(const char *content, calx_text_size_t size) {
  current_size = size;

  // Word wrap the content
  int chars_per_line = get_chars_per_line(size);
  text_renderer_wrap(content, content_buffer, MAX_CONTENT_SIZE, chars_per_line);

  // Parse into lines
  total_lines = 0;
  char *line_start = content_buffer;

  for (int i = 0; content_buffer[i] && total_lines < MAX_LINES; i++) {
    if (content_buffer[i] == '\n' || content_buffer[i + 1] == '\0') {
      line_starts[total_lines] = line_start;

      if (content_buffer[i] == '\n') {
        line_lengths[total_lines] = &content_buffer[i] - line_start;
        content_buffer[i] = '\0'; // Null-terminate line
      } else {
        line_lengths[total_lines] = &content_buffer[i + 1] - line_start;
      }

      total_lines++;
      line_start = &content_buffer[i + 1];
    }
  }
}

// =============================================================================
// Render Content
// =============================================================================

void text_renderer_render_content(int scroll_line) {
  if (scroll_line < 0)
    scroll_line = 0;
  if (scroll_line >= total_lines)
    scroll_line = total_lines > 0 ? total_lines - 1 : 0;

  int lines_per_screen = get_lines_per_screen(current_size);
  int line_height = display_driver_get_line_height(current_size);

  display_driver_clear();

  for (int i = 0; i < lines_per_screen && (scroll_line + i) < total_lines;
       i++) {
    int line_idx = scroll_line + i;
    if (line_starts[line_idx]) {
      display_driver_draw_text(0, i * line_height, line_starts[line_idx],
                               current_size);
    }
  }

  // Show scroll indicator if there's more content
  if (scroll_line + lines_per_screen < total_lines) {
    // Draw down arrow indicator
    display_driver_draw_text(122, 24, "v", TEXT_SIZE_SMALL);
  }
  if (scroll_line > 0) {
    // Draw up arrow indicator
    display_driver_draw_text(122, 0, "^", TEXT_SIZE_SMALL);
  }
}

// =============================================================================
// Queries
// =============================================================================

int text_renderer_get_line_count(void) { return total_lines; }

int text_renderer_get_page_count(int lines_per_page) {
  if (lines_per_page <= 0)
    return 1;
  return (total_lines + lines_per_page - 1) / lines_per_page;
}
