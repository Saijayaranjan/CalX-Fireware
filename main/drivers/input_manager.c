/**
 * =============================================================================
 * CalX ESP32 Firmware - Input Manager
 * =============================================================================
 * Matrix keypad scanning with debounce and long press detection.
 * =============================================================================
 */

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#include "event_manager.h"
#include "input_manager.h"
#include "logger.h"

static const char *TAG = "INPUT";

// =============================================================================
// Configuration
// =============================================================================
static const int row_pins[KEYPAD_ROWS] = KEYPAD_ROW_PINS;
static const int col_pins[KEYPAD_COLS] = KEYPAD_COL_PINS;

// Key mapping matrix (rows x cols)
// Adjust this based on your actual keypad layout
static const calx_key_t key_map[KEYPAD_ROWS][KEYPAD_COLS] = {
    // Col 0     Col 1      Col 2      Col 3      Col 4
    {KEY_7, KEY_8, KEY_9, KEY_DEL, KEY_AC},             // Row 0
    {KEY_4, KEY_5, KEY_6, KEY_MULTIPLY, KEY_DIVIDE},    // Row 1
    {KEY_1, KEY_2, KEY_3, KEY_PLUS, KEY_MINUS},         // Row 2
    {KEY_0, KEY_DOT, KEY_EQUALS, KEY_OK, KEY_NONE},     // Row 3
    {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_NONE},  // Row 4
    {KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_NONE}, // Row 5 (if used)
};

// =============================================================================
// State
// =============================================================================
static calx_key_t current_key = KEY_NONE;
static calx_key_t previous_key = KEY_NONE;
static uint32_t key_press_time = 0;
static uint32_t last_key_time = 0;
static bool long_press_sent = false;

// Key state for debouncing
static uint8_t key_state[KEYPAD_ROWS][KEYPAD_COLS] = {0};
#define DEBOUNCE_COUNT 3

// =============================================================================
// Initialization
// =============================================================================

void input_manager_init(void) {
  // Configure row pins as outputs
  for (int i = 0; i < KEYPAD_ROWS; i++) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << row_pins[i]),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(row_pins[i], 1); // High = inactive
  }

  // Configure column pins as inputs with pull-up
  for (int i = 0; i < KEYPAD_COLS; i++) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << col_pins[i]),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
  }

  memset(key_state, 0, sizeof(key_state));
  current_key = KEY_NONE;
  previous_key = KEY_NONE;

  LOG_INFO(TAG, "Input manager initialized (%dx%d matrix)", KEYPAD_ROWS,
           KEYPAD_COLS);
}

// =============================================================================
// Scanning
// =============================================================================

void input_manager_scan(void) {
  calx_key_t detected_key = KEY_NONE;

  // Scan matrix
  for (int row = 0; row < KEYPAD_ROWS; row++) {
    // Drive current row low
    gpio_set_level(row_pins[row], 0);

    // Small delay for signal to settle
    esp_rom_delay_us(10);

    // Read columns
    for (int col = 0; col < KEYPAD_COLS; col++) {
      bool pressed = (gpio_get_level(col_pins[col]) == 0);

      // Debounce using counter
      if (pressed) {
        if (key_state[row][col] < DEBOUNCE_COUNT) {
          key_state[row][col]++;
        }
      } else {
        if (key_state[row][col] > 0) {
          key_state[row][col]--;
        }
      }

      // Key is considered pressed if counter reached threshold
      if (key_state[row][col] >= DEBOUNCE_COUNT) {
        calx_key_t key = key_map[row][col];
        if (key != KEY_NONE) {
          detected_key = key;
        }
      }
    }

    // Drive row high again
    gpio_set_level(row_pins[row], 1);
  }

  // Process detected key
  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

  if (detected_key != KEY_NONE) {
    if (detected_key != current_key) {
      // New key pressed
      current_key = detected_key;
      key_press_time = now;
      long_press_sent = false;
      last_key_time = now;

      // Post key press event
      event_manager_post_key(current_key, false);
      LOG_DEBUG(TAG, "Key pressed: %d", current_key);
    } else {
      // Key still held - check for long press
      if (!long_press_sent && (now - key_press_time) >= KEYPAD_LONG_PRESS_MS) {
        long_press_sent = true;
        event_manager_post_key(current_key, true);
        LOG_DEBUG(TAG, "Key long pressed: %d", current_key);
      }
    }
  } else {
    // No key pressed
    if (current_key != KEY_NONE) {
      previous_key = current_key;
      current_key = KEY_NONE;
    }
  }
}

// =============================================================================
// Queries
// =============================================================================

calx_key_t input_manager_get_key(void) { return current_key; }

bool input_manager_is_key_pressed(calx_key_t key) { return current_key == key; }

bool input_manager_any_key_pressed(void) { return current_key != KEY_NONE; }

uint32_t input_manager_get_last_key_time(void) { return last_key_time; }

// =============================================================================
// Virtual Key Injection (for web-based testing)
// =============================================================================

void input_manager_inject_key(calx_key_t key) {
  if (key == KEY_NONE)
    return;

  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

  // Simulate a key press
  current_key = key;
  key_press_time = now;
  last_key_time = now;
  long_press_sent = false;

  // Post key press event
  event_manager_post_key(key, false);
  LOG_INFO(TAG, "Virtual key injected: %d", key);

  // Auto-release after a short delay (simulating physical press)
  vTaskDelay(pdMS_TO_TICKS(50));
  previous_key = current_key;
  current_key = KEY_NONE;
}
