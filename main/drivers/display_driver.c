/**
 * =============================================================================
 * CalX ESP32 Firmware - Display Driver
 * =============================================================================
 * SSD1306 OLED driver for 128x32 display via I2C.
 * =============================================================================
 */

#include "driver/i2c.h"
#include "esp_log.h"
#include <string.h>

#include "display_driver.h"
#include "logger.h"

static const char *TAG = "DISPLAY";

// =============================================================================
// I2C Configuration
// =============================================================================
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_TIMEOUT_MS 1000

// =============================================================================
// SSD1306 Commands
// =============================================================================
#define SSD1306_CMD 0x00
#define SSD1306_DATA 0x40

#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETVCOMDETECT 0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR 0x22
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_CHARGEPUMP 0x8D

// =============================================================================
// Display Buffer
// =============================================================================
#define BUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT / 8)
static uint8_t display_buffer[BUFFER_SIZE];

// =============================================================================
// Font Data (6x8 basic font)
// =============================================================================
// Basic 6x8 ASCII font (characters 32-127)
static const uint8_t font_6x8[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Space
    0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, // !
    0x00, 0x07, 0x00, 0x07, 0x00, 0x00, // "
    0x14, 0x7F, 0x14, 0x7F, 0x14, 0x00, // #
    0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x00, // $
    0x23, 0x13, 0x08, 0x64, 0x62, 0x00, // %
    0x36, 0x49, 0x56, 0x20, 0x50, 0x00, // &
    0x00, 0x08, 0x07, 0x03, 0x00, 0x00, // '
    0x00, 0x1C, 0x22, 0x41, 0x00, 0x00, // (
    0x00, 0x41, 0x22, 0x1C, 0x00, 0x00, // )
    0x2A, 0x1C, 0x7F, 0x1C, 0x2A, 0x00, // *
    0x08, 0x08, 0x3E, 0x08, 0x08, 0x00, // +
    0x00, 0x80, 0x70, 0x30, 0x00, 0x00, // ,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x00, // -
    0x00, 0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x20, 0x10, 0x08, 0x04, 0x02, 0x00, // /
    0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, // 0
    0x00, 0x42, 0x7F, 0x40, 0x00, 0x00, // 1
    0x72, 0x49, 0x49, 0x49, 0x46, 0x00, // 2
    0x21, 0x41, 0x49, 0x4D, 0x33, 0x00, // 3
    0x18, 0x14, 0x12, 0x7F, 0x10, 0x00, // 4
    0x27, 0x45, 0x45, 0x45, 0x39, 0x00, // 5
    0x3C, 0x4A, 0x49, 0x49, 0x31, 0x00, // 6
    0x41, 0x21, 0x11, 0x09, 0x07, 0x00, // 7
    0x36, 0x49, 0x49, 0x49, 0x36, 0x00, // 8
    0x46, 0x49, 0x49, 0x29, 0x1E, 0x00, // 9
    0x00, 0x00, 0x14, 0x00, 0x00, 0x00, // :
    0x00, 0x40, 0x34, 0x00, 0x00, 0x00, // ;
    0x00, 0x08, 0x14, 0x22, 0x41, 0x00, // <
    0x14, 0x14, 0x14, 0x14, 0x14, 0x00, // =
    0x00, 0x41, 0x22, 0x14, 0x08, 0x00, // >
    0x02, 0x01, 0x59, 0x09, 0x06, 0x00, // ?
    0x3E, 0x41, 0x5D, 0x59, 0x4E, 0x00, // @
    0x7C, 0x12, 0x11, 0x12, 0x7C, 0x00, // A
    0x7F, 0x49, 0x49, 0x49, 0x36, 0x00, // B
    0x3E, 0x41, 0x41, 0x41, 0x22, 0x00, // C
    0x7F, 0x41, 0x41, 0x41, 0x3E, 0x00, // D
    0x7F, 0x49, 0x49, 0x49, 0x41, 0x00, // E
    0x7F, 0x09, 0x09, 0x09, 0x01, 0x00, // F
    0x3E, 0x41, 0x41, 0x51, 0x73, 0x00, // G
    0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00, // H
    0x00, 0x41, 0x7F, 0x41, 0x00, 0x00, // I
    0x20, 0x40, 0x41, 0x3F, 0x01, 0x00, // J
    0x7F, 0x08, 0x14, 0x22, 0x41, 0x00, // K
    0x7F, 0x40, 0x40, 0x40, 0x40, 0x00, // L
    0x7F, 0x02, 0x1C, 0x02, 0x7F, 0x00, // M
    0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00, // N
    0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00, // O
    0x7F, 0x09, 0x09, 0x09, 0x06, 0x00, // P
    0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00, // Q
    0x7F, 0x09, 0x19, 0x29, 0x46, 0x00, // R
    0x26, 0x49, 0x49, 0x49, 0x32, 0x00, // S
    0x03, 0x01, 0x7F, 0x01, 0x03, 0x00, // T
    0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00, // U
    0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00, // V
    0x3F, 0x40, 0x38, 0x40, 0x3F, 0x00, // W
    0x63, 0x14, 0x08, 0x14, 0x63, 0x00, // X
    0x03, 0x04, 0x78, 0x04, 0x03, 0x00, // Y
    0x61, 0x59, 0x49, 0x4D, 0x43, 0x00, // Z
    0x00, 0x7F, 0x41, 0x41, 0x41, 0x00, // [
    0x02, 0x04, 0x08, 0x10, 0x20, 0x00, // backslash
    0x00, 0x41, 0x41, 0x41, 0x7F, 0x00, // ]
    0x04, 0x02, 0x01, 0x02, 0x04, 0x00, // ^
    0x40, 0x40, 0x40, 0x40, 0x40, 0x00, // _
    0x00, 0x03, 0x07, 0x08, 0x00, 0x00, // `
    0x20, 0x54, 0x54, 0x78, 0x40, 0x00, // a
    0x7F, 0x28, 0x44, 0x44, 0x38, 0x00, // b
    0x38, 0x44, 0x44, 0x44, 0x28, 0x00, // c
    0x38, 0x44, 0x44, 0x28, 0x7F, 0x00, // d
    0x38, 0x54, 0x54, 0x54, 0x18, 0x00, // e
    0x00, 0x08, 0x7E, 0x09, 0x02, 0x00, // f
    0x18, 0xA4, 0xA4, 0x9C, 0x78, 0x00, // g
    0x7F, 0x08, 0x04, 0x04, 0x78, 0x00, // h
    0x00, 0x44, 0x7D, 0x40, 0x00, 0x00, // i
    0x20, 0x40, 0x40, 0x3D, 0x00, 0x00, // j
    0x7F, 0x10, 0x28, 0x44, 0x00, 0x00, // k
    0x00, 0x41, 0x7F, 0x40, 0x00, 0x00, // l
    0x7C, 0x04, 0x78, 0x04, 0x78, 0x00, // m
    0x7C, 0x08, 0x04, 0x04, 0x78, 0x00, // n
    0x38, 0x44, 0x44, 0x44, 0x38, 0x00, // o
    0xFC, 0x18, 0x24, 0x24, 0x18, 0x00, // p
    0x18, 0x24, 0x24, 0x18, 0xFC, 0x00, // q
    0x7C, 0x08, 0x04, 0x04, 0x08, 0x00, // r
    0x48, 0x54, 0x54, 0x54, 0x24, 0x00, // s
    0x04, 0x04, 0x3F, 0x44, 0x24, 0x00, // t
    0x3C, 0x40, 0x40, 0x20, 0x7C, 0x00, // u
    0x1C, 0x20, 0x40, 0x20, 0x1C, 0x00, // v
    0x3C, 0x40, 0x30, 0x40, 0x3C, 0x00, // w
    0x44, 0x28, 0x10, 0x28, 0x44, 0x00, // x
    0x4C, 0x90, 0x90, 0x90, 0x7C, 0x00, // y
    0x44, 0x64, 0x54, 0x4C, 0x44, 0x00, // z
    0x00, 0x08, 0x36, 0x41, 0x00, 0x00, // {
    0x00, 0x00, 0x77, 0x00, 0x00, 0x00, // |
    0x00, 0x41, 0x36, 0x08, 0x00, 0x00, // }
    0x02, 0x01, 0x02, 0x04, 0x02, 0x00, // ~
};

#define FONT_CHAR_WIDTH 6
#define FONT_CHAR_HEIGHT 8
#define FONT_FIRST_CHAR 32

// =============================================================================
// I2C Helpers
// =============================================================================

static esp_err_t i2c_write_cmd(uint8_t cmd) {
  uint8_t data[2] = {SSD1306_CMD, cmd};
  return i2c_master_write_to_device(I2C_MASTER_NUM, DISPLAY_I2C_ADDR, data, 2,
                                    pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}

static esp_err_t i2c_write_data(const uint8_t *data, size_t len) {
  uint8_t *buf = malloc(len + 1);
  if (!buf)
    return ESP_ERR_NO_MEM;

  buf[0] = SSD1306_DATA;
  memcpy(buf + 1, data, len);

  esp_err_t ret =
      i2c_master_write_to_device(I2C_MASTER_NUM, DISPLAY_I2C_ADDR, buf, len + 1,
                                 pdMS_TO_TICKS(I2C_TIMEOUT_MS));
  free(buf);
  return ret;
}

// =============================================================================
// Initialization
// =============================================================================

void display_driver_init(void) {
  // Configure I2C
  i2c_config_t conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = DISPLAY_I2C_SDA_PIN,
      .scl_io_num = DISPLAY_I2C_SCL_PIN,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master.clk_speed = DISPLAY_I2C_FREQ_HZ,
  };

  ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
  ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));

  // Initialize SSD1306
  const uint8_t init_cmds[] = {
      SSD1306_DISPLAYOFF,
      SSD1306_SETDISPLAYCLOCKDIV,
      0x80,
      SSD1306_SETMULTIPLEX,
      0x1F, // 32 rows
      SSD1306_SETDISPLAYOFFSET,
      0x00,
      SSD1306_SETSTARTLINE | 0x00,
      SSD1306_CHARGEPUMP,
      0x14,
      SSD1306_MEMORYMODE,
      0x00, // Horizontal addressing
      SSD1306_SEGREMAP | 0x01,
      SSD1306_COMSCANDEC,
      SSD1306_SETCOMPINS,
      0x02,
      SSD1306_SETCONTRAST,
      0x8F,
      SSD1306_SETPRECHARGE,
      0xF1,
      SSD1306_SETVCOMDETECT,
      0x40,
      SSD1306_DISPLAYALLON_RESUME,
      SSD1306_NORMALDISPLAY,
      SSD1306_DISPLAYON,
  };

  for (int i = 0; i < sizeof(init_cmds); i++) {
    i2c_write_cmd(init_cmds[i]);
  }

  display_driver_clear();
  display_driver_update();

  LOG_INFO(TAG, "Display initialized (%dx%d)", DISPLAY_WIDTH, DISPLAY_HEIGHT);
}

// =============================================================================
// Buffer Operations
// =============================================================================

void display_driver_clear(void) { memset(display_buffer, 0, BUFFER_SIZE); }

void display_driver_update(void) {
  // Set column and page addresses
  i2c_write_cmd(SSD1306_COLUMNADDR);
  i2c_write_cmd(0);
  i2c_write_cmd(DISPLAY_WIDTH - 1);
  i2c_write_cmd(SSD1306_PAGEADDR);
  i2c_write_cmd(0);
  i2c_write_cmd((DISPLAY_HEIGHT / 8) - 1);

  // Write buffer
  i2c_write_data(display_buffer, BUFFER_SIZE);
}

void display_driver_set_pixel(int x, int y, bool on) {
  if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT) {
    return;
  }

  if (on) {
    display_buffer[x + (y / 8) * DISPLAY_WIDTH] |= (1 << (y & 7));
  } else {
    display_buffer[x + (y / 8) * DISPLAY_WIDTH] &= ~(1 << (y & 7));
  }
}

// =============================================================================
// Text Rendering
// =============================================================================

static void draw_char(int x, int y, char c, int scale) {
  if (c < FONT_FIRST_CHAR || c > 127) {
    c = ' ';
  }

  int idx = (c - FONT_FIRST_CHAR) * FONT_CHAR_WIDTH;

  for (int col = 0; col < FONT_CHAR_WIDTH; col++) {
    uint8_t line = font_6x8[idx + col];
    for (int row = 0; row < 8; row++) {
      if (line & (1 << row)) {
        // Draw scaled pixel
        for (int sx = 0; sx < scale; sx++) {
          for (int sy = 0; sy < scale; sy++) {
            display_driver_set_pixel(x + col * scale + sx, y + row * scale + sy,
                                     true);
          }
        }
      }
    }
  }
}

void display_driver_draw_text(int x, int y, const char *text,
                              calx_text_size_t size) {
  int scale = 1;
  switch (size) {
  case TEXT_SIZE_SMALL:
    scale = 1;
    break;
  case TEXT_SIZE_NORMAL:
    scale = 1;
    break; // Could use 1.5x with custom handling
  case TEXT_SIZE_LARGE:
    scale = 2;
    break;
  }

  int char_width = display_driver_get_char_width(size);

  while (*text) {
    draw_char(x, y, *text, scale);
    x += char_width;
    text++;

    if (x + char_width > DISPLAY_WIDTH) {
      break; // Clip to screen
    }
  }
}

void display_driver_draw_text_centered(int y, const char *text,
                                       calx_text_size_t size) {
  int char_width = display_driver_get_char_width(size);
  int text_width = strlen(text) * char_width;
  int x = (DISPLAY_WIDTH - text_width) / 2;
  if (x < 0)
    x = 0;

  display_driver_draw_text(x, y, text, size);
}

// =============================================================================
// Drawing Primitives
// =============================================================================

void display_driver_draw_hline(int x, int y, int width) {
  for (int i = 0; i < width; i++) {
    display_driver_set_pixel(x + i, y, true);
  }
}

void display_driver_draw_vline(int x, int y, int height) {
  for (int i = 0; i < height; i++) {
    display_driver_set_pixel(x, y + i, true);
  }
}

void display_driver_draw_rect(int x, int y, int width, int height) {
  display_driver_draw_hline(x, y, width);
  display_driver_draw_hline(x, y + height - 1, width);
  display_driver_draw_vline(x, y, height);
  display_driver_draw_vline(x + width - 1, y, height);
}

void display_driver_fill_rect(int x, int y, int width, int height, bool on) {
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      display_driver_set_pixel(x + i, y + j, on);
    }
  }
}

void display_driver_invert_rect(int x, int y, int width, int height) {
  for (int px = x; px < x + width && px < DISPLAY_WIDTH; px++) {
    for (int py = y; py < y + height && py < DISPLAY_HEIGHT; py++) {
      int idx = px + (py / 8) * DISPLAY_WIDTH;
      display_buffer[idx] ^= (1 << (py & 7));
    }
  }
}

// =============================================================================
// Power Control
// =============================================================================

void display_driver_power(bool on) {
  i2c_write_cmd(on ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
}

void display_driver_set_contrast(uint8_t contrast) {
  i2c_write_cmd(SSD1306_SETCONTRAST);
  i2c_write_cmd(contrast);
}

// =============================================================================
// Metrics
// =============================================================================

int display_driver_get_char_width(calx_text_size_t size) {
  switch (size) {
  case TEXT_SIZE_SMALL:
    return 6;
  case TEXT_SIZE_NORMAL:
    return 6;
  case TEXT_SIZE_LARGE:
    return 12;
  default:
    return 6;
  }
}

int display_driver_get_line_height(calx_text_size_t size) {
  switch (size) {
  case TEXT_SIZE_SMALL:
    return 8;
  case TEXT_SIZE_NORMAL:
    return 10;
  case TEXT_SIZE_LARGE:
    return 16;
  default:
    return 8;
  }
}

// =============================================================================
// Buffer Access (for web display streaming)
// =============================================================================

const uint8_t *display_driver_get_buffer(void) { return display_buffer; }

void display_driver_draw_bitmap(int x, int y, const uint8_t *bitmap, int w,
                                int h) {
  int byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte

  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      if (bitmap[j * byteWidth + i / 8] & (128 >> (i % 8))) {
        display_driver_set_pixel(x + i, y + j, true);
      }
    }
  }
}
