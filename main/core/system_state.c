/**
 * =============================================================================
 * CalX ESP32 Firmware - System State Machine
 * =============================================================================
 * Central state machine controlling application flow.
 * =============================================================================
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

#include "api_client.h"
#include "event_manager.h"
#include "logger.h"
#include "system_state.h"
#include "ui_manager.h"
#include "wifi_manager.h"

static const char *TAG = "SYS_STATE";

// =============================================================================
// State Variables
// =============================================================================
static calx_state_t current_state = STATE_BOOT;
static calx_state_t previous_state = STATE_BOOT;
static SemaphoreHandle_t state_mutex = NULL;
static char error_message[64] = {0};

// Menu state
static int menu_selection = 0;
static int settings_selection = 0;

// Busy state tracking
static bool is_busy = false;
static TickType_t last_heartbeat = 0;

// Forward declarations
static void handle_menu_key(calx_key_t key);
static void select_menu_item(int item);

// =============================================================================
// State Transition Table
// =============================================================================

void system_state_init(void) {
  state_mutex = xSemaphoreCreateMutex();
  current_state = STATE_BOOT;
  previous_state = STATE_BOOT;
  menu_selection = 0;
  LOG_INFO(TAG, "System state initialized");
}

void system_state_set(calx_state_t state) {
  if (xSemaphoreTake(state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    if (state != current_state) {
      LOG_INFO(TAG, "State: %d -> %d", current_state, state);
      previous_state = current_state;
      current_state = state;

      // Notify UI of state change
      ui_manager_on_state_change(state);
    }
    xSemaphoreGive(state_mutex);
  }
}

calx_state_t system_state_get(void) {
  calx_state_t state = STATE_BOOT;
  if (xSemaphoreTake(state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    state = current_state;
    xSemaphoreGive(state_mutex);
  }
  return state;
}

calx_state_t system_state_get_previous(void) { return previous_state; }

void system_state_go_back(void) {
  calx_state_t state = system_state_get();

  switch (state) {
  case STATE_MENU:
    system_state_set(STATE_IDLE);
    break;
  case STATE_CHAT:
  case STATE_FILE:
  case STATE_AI:
  case STATE_SETTINGS:
    system_state_set(STATE_MENU);
    break;
  case STATE_ERROR:
    system_state_set(previous_state);
    break;
  default:
    // Stay in current state
    break;
  }
}

void system_state_go_idle(void) {
  system_state_set(STATE_IDLE);
  menu_selection = 0;
}

bool system_state_is_busy(void) { return is_busy; }

void system_state_set_error(const char *error_msg) {
  strncpy(error_message, error_msg, sizeof(error_message) - 1);
  error_message[sizeof(error_message) - 1] = '\0';
  system_state_set(STATE_ERROR);
}

const char *system_state_get_error(void) { return error_message; }

// =============================================================================
// Key Handling
// =============================================================================

void system_state_handle_key(calx_key_t key, bool long_press) {
  calx_state_t state = system_state_get();

  // Global key handling
  if (key == KEY_AC) {
    if (long_press) {
      // AC long press always goes to idle
      system_state_go_idle();
      return;
    } else {
      // AC short press goes back
      system_state_go_back();
      return;
    }
  }

  // State-specific key handling
  switch (state) {
  case STATE_NOT_BOUND:
    // Any key starts WiFi/bind setup
    if (key != KEY_NONE) {
      wifi_manager_start_ap();
      system_state_set(STATE_WIFI_SETUP);
    }
    break;

  case STATE_IDLE:
    // Any key goes to menu
    if (key != KEY_NONE && key != KEY_AC) {
      system_state_set(STATE_MENU);
    }
    break;

  case STATE_MENU:
    handle_menu_key(key);
    break;

  case STATE_CHAT:
    ui_manager_handle_chat_key(key);
    break;

  case STATE_FILE:
    ui_manager_handle_file_key(key);
    break;

  case STATE_AI:
    ui_manager_handle_ai_key(key);
    break;

  case STATE_SETTINGS:
    ui_manager_handle_settings_key(key);
    break;

  default:
    break;
  }
}

// =============================================================================
// Menu Key Handling
// =============================================================================

static void handle_menu_key(calx_key_t key) {
  switch (key) {
  case KEY_UP:
    if (menu_selection >= 2)
      menu_selection -= 2;
    ui_manager_set_menu_selection(menu_selection);
    break;

  case KEY_DOWN:
    if (menu_selection < 2)
      menu_selection += 2;
    ui_manager_set_menu_selection(menu_selection);
    break;

  case KEY_LEFT:
    if (menu_selection % 2 == 1)
      menu_selection--;
    ui_manager_set_menu_selection(menu_selection);
    break;

  case KEY_RIGHT:
    if (menu_selection % 2 == 0)
      menu_selection++;
    ui_manager_set_menu_selection(menu_selection);
    break;

  case KEY_OK:
  case KEY_EQUALS:
    // Select current menu item
    select_menu_item(menu_selection);
    break;

  case KEY_1:
    select_menu_item(MENU_ITEM_CHAT);
    break;
  case KEY_2:
    select_menu_item(MENU_ITEM_FILE);
    break;
  case KEY_3:
    select_menu_item(MENU_ITEM_AI);
    break;
  case KEY_4:
    select_menu_item(MENU_ITEM_SETTINGS);
    break;

  default:
    break;
  }
}

static void select_menu_item(int item) {
  switch (item) {
  case MENU_ITEM_CHAT:
    system_state_set(STATE_CHAT);
    break;
  case MENU_ITEM_FILE:
    system_state_set(STATE_FILE);
    break;
  case MENU_ITEM_AI:
    system_state_set(STATE_AI);
    break;
  case MENU_ITEM_SETTINGS:
    system_state_set(STATE_SETTINGS);
    break;
  }
}

// =============================================================================
// Network Processing
// =============================================================================

void system_state_process_network(void) {
  calx_state_t state = system_state_get();
  TickType_t now = xTaskGetTickCount();

  // Don't process network if WiFi not connected
  if (!wifi_manager_is_connected()) {
    return;
  }

  // Heartbeat (in any connected state)
  if (state == STATE_IDLE || state == STATE_MENU || state == STATE_CHAT ||
      state == STATE_FILE || state == STATE_AI) {

    TickType_t heartbeat_interval = pdMS_TO_TICKS(HEARTBEAT_NORMAL_INTERVAL_MS);

    if ((now - last_heartbeat) >= heartbeat_interval) {
      is_busy = true;
      api_client_send_heartbeat();
      last_heartbeat = now;
      is_busy = false;
    }
  }

  // State-specific network operations on entry
  static calx_state_t last_processed_state = STATE_BOOT;

  if (state != last_processed_state) {
    // State just entered - fetch data
    switch (state) {
    case STATE_CHAT: {
      // Fetch chat messages
      chat_message_t messages[10];
      int count = api_client_fetch_chat(messages, 10, NULL);
      if (count > 0) {
        // Display first message (simplified)
        ui_manager_set_file_content(messages[0].content);
      }
      LOG_INFO(TAG, "Fetched %d chat messages", count);
    } break;

    case STATE_FILE: {
      // Fetch file content
      file_content_t file;
      if (api_client_fetch_file(&file)) {
        ui_manager_set_file_content(file.content);
        LOG_INFO(TAG, "File fetched: %d chars", file.char_count);
      }
    } break;

    case STATE_AI: {
      // AI query would be triggered by calculator input
      // For now, just log that we're ready
      LOG_INFO(TAG, "AI mode ready for queries");
    } break;

    default:
      break;
    }
    last_processed_state = state;
  }
}
