/**
 * =============================================================================
 * CalX ESP32 Firmware - UI Manager
 * =============================================================================
 * Screen rendering and UI state management for 128x32 OLED.
 * =============================================================================
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <string.h>

#include "api_client.h"
#include "battery_manager.h"
#include "display_driver.h"
#include "logger.h"
#include "power_manager.h"
#include "system_state.h"
#include "text_renderer.h"
#include "ui_manager.h"
#include "wifi_manager.h"

static const char *TAG = "UI";

// =============================================================================
// State
// =============================================================================
static SemaphoreHandle_t ui_mutex = NULL;
static bool needs_redraw = true;
static calx_state_t current_screen = STATE_BOOT;

// Screen-specific state
static int menu_selection = 0;
static int settings_selection = 0;
static bool in_settings_submenu = false; // Are we in a submenu?
static int submenu_selection = 0;        // Which option in submenu
static bool has_notification = false;

// Content buffers
static char busy_message[32] = "Fetching...";
static char error_message[32] = "Error";
static char bind_code[5] = "----";
static int ota_progress = 0;

// Chat state
static int chat_scroll = 0;
static int chat_page = 0;

// File state
static int file_scroll = 0;

// AI state
static bool ai_has_more = false;

// =============================================================================
// Initialization
// =============================================================================

void ui_manager_init(void) {
  ui_mutex = xSemaphoreCreateMutex();
  text_renderer_init();
  LOG_INFO(TAG, "UI manager initialized");
}

// =============================================================================
// Screen Rendering Functions
// =============================================================================

static void render_boot_screen(void) {
  display_driver_clear();
  display_driver_draw_text_centered(8, "CalX", TEXT_SIZE_LARGE);
  display_driver_draw_text_centered(24, "Starting...", TEXT_SIZE_SMALL);
  display_driver_update();
}

static void render_not_bound_screen(void) {
  display_driver_clear();
  display_driver_draw_text_centered(8, "CalX", TEXT_SIZE_LARGE);
  display_driver_draw_text_centered(24, "Not Bound", TEXT_SIZE_SMALL);
  display_driver_update();
}

static void render_idle_screen(void) {
  display_driver_clear();

  // Line 1: CalX logo (centered)
  display_driver_draw_text_centered(4, "CalX", TEXT_SIZE_LARGE);

  // Line 2-3: Status line (ONLINE/OFFLINE + Battery + Notification)
  char status[32];
  int battery = battery_manager_get_percent();
  bool online = wifi_manager_is_connected();

  snprintf(status, sizeof(status), "%s %d%%", online ? "ONLINE" : "OFFLINE",
           battery);

  display_driver_draw_text_centered(22, status, TEXT_SIZE_SMALL);

  // Notification dot (right side)
  if (has_notification) {
    display_driver_draw_text(120, 22, "*", TEXT_SIZE_SMALL);
  }

  display_driver_update();
}

static void render_menu_screen(void) {
  display_driver_clear();

  // 2x2 grid menu
  const char *items[] = {"1.Chat", "3.AI", "2.File", "4.Set"};

  // Row 1
  display_driver_draw_text(0, 4, items[0], TEXT_SIZE_SMALL);
  display_driver_draw_text(64, 4, items[1], TEXT_SIZE_SMALL);

  // Row 2
  display_driver_draw_text(0, 16, items[2], TEXT_SIZE_SMALL);
  display_driver_draw_text(64, 16, items[3], TEXT_SIZE_SMALL);

  // Selection indicator
  int x = (menu_selection % 2) * 64;
  int y = (menu_selection / 2) * 12 + 4;

  // Draw arrow before selected item
  if (menu_selection == 0 || menu_selection == 2) {
    display_driver_draw_text(56, y, ">", TEXT_SIZE_SMALL);
  } else {
    display_driver_draw_text(120, y, "<", TEXT_SIZE_SMALL);
  }

  // Invert selected item
  display_driver_invert_rect(x, y - 2, 60, 12);

  display_driver_update();
}

static void render_busy_screen(void) {
  display_driver_clear();
  display_driver_draw_text_centered(12, busy_message, TEXT_SIZE_NORMAL);
  display_driver_update();
}

static void render_chat_screen(void) {
  display_driver_clear();

  // Render current chat message using text renderer
  text_renderer_render_content(chat_scroll);

  display_driver_update();
}

static void render_file_screen(void) {
  display_driver_clear();

  // Render file content with small font (4 lines)
  text_renderer_render_content(file_scroll);

  display_driver_update();
}

static void render_ai_screen(void) {
  display_driver_clear();

  text_renderer_render_content(0);

  // Show [More...] indicator if more content available
  if (ai_has_more) {
    display_driver_draw_text_centered(24, "[More...]", TEXT_SIZE_SMALL);
  }

  display_driver_update();
}

static void render_display_settings(void) {
  const char *items[] = {"Text Size", "Theme", "Contrast", "Timeout"};
  const char *values[] = {"Normal", "Dark", "Med", "30s"}; // Placeholders

  int start = (submenu_selection / 4) * 4;
  for (int i = 0; i < 4 && (start + i) < 4; i++) {
    int y = i * 8;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%s: %s", items[start + i],
             values[start + i]);
    display_driver_draw_text(0, y, buffer, TEXT_SIZE_SMALL);

    if ((start + i) == submenu_selection) {
      display_driver_invert_rect(0, y, 128, 8);
    }
  }
}

static void render_power_settings(void) {
  const char *items[] = {"Power Mode", "Battery", "Charging", "Sleep"};
  const char *values[] = {"Normal", "85%", "No", "Auto"}; // Placeholders

  int start = (submenu_selection / 4) * 4;
  for (int i = 0; i < 4 && (start + i) < 4; i++) {
    int y = i * 8;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%s: %s", items[start + i],
             values[start + i]);
    display_driver_draw_text(0, y, buffer, TEXT_SIZE_SMALL);

    if ((start + i) == submenu_selection) {
      display_driver_invert_rect(0, y, 128, 8);
    }
  }
}

static void render_device_settings(void) {
  const char *items[] = {"Name", "ID", "Bind Status", "Unbind"};
  const char *values[] = {"CalX", "8857...", "Bound", "Select"}; // Placeholders

  int start = (submenu_selection / 4) * 4;
  for (int i = 0; i < 4 && (start + i) < 4; i++) {
    int y = i * 8;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%s: %s", items[start + i],
             values[start + i]);
    display_driver_draw_text(0, y, buffer, TEXT_SIZE_SMALL);

    if ((start + i) == submenu_selection) {
      display_driver_invert_rect(0, y, 128, 8);
    }
  }
}

static void render_internet_settings(void) {
  const char *items[] = {"Status", "WiFi Setup", "Saved Network",
                         "BLE Fallback"};
  const char *values[] = {"Offline", "Scan...", "None", "Off"}; // Placeholders

  int start = (submenu_selection / 4) * 4;
  for (int i = 0; i < 4 && (start + i) < 4; i++) {
    int y = i * 8;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%s: %s", items[start + i],
             values[start + i]);
    display_driver_draw_text(0, y, buffer, TEXT_SIZE_SMALL);

    if ((start + i) == submenu_selection) {
      display_driver_invert_rect(0, y, 128, 8);
    }
  }
}

static void render_ai_config_settings(void) {
  const char *items[] = {"Enabled", "Provider", "Model", "Length"};
  const char *values[] = {"Yes", "OpenAI", "GPT-4o", "Normal"}; // Placeholders

  int start = (submenu_selection / 4) * 4;
  for (int i = 0; i < 4 && (start + i) < 4; i++) {
    int y = i * 8;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%s: %s", items[start + i],
             values[start + i]);
    display_driver_draw_text(0, y, buffer, TEXT_SIZE_SMALL);

    if ((start + i) == submenu_selection) {
      display_driver_invert_rect(0, y, 128, 8);
    }
  }
}

static void render_keyboard_settings(void) {
  const char *items[] = {"Mode", "Key Repeat", "Long Press", "Shift"};
  const char *values[] = {"T9", "Fast", "Med", "Toggle"}; // Placeholders

  int start = (submenu_selection / 4) * 4;
  for (int i = 0; i < 4 && (start + i) < 4; i++) {
    int y = i * 8;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%s: %s", items[start + i],
             values[start + i]);
    display_driver_draw_text(0, y, buffer, TEXT_SIZE_SMALL);

    if ((start + i) == submenu_selection) {
      display_driver_invert_rect(0, y, 128, 8);
    }
  }
}

static void render_update_settings(void) {
  const char *items[] = {"Version", "Check Now", "Auto Update", "Channel"};
  const char *values[] = {"v1.0.0", "Select", "On", "Stable"}; // Placeholders

  int start = (submenu_selection / 4) * 4;
  for (int i = 0; i < 4 && (start + i) < 4; i++) {
    int y = i * 8;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%s: %s", items[start + i],
             values[start + i]);
    display_driver_draw_text(0, y, buffer, TEXT_SIZE_SMALL);

    if ((start + i) == submenu_selection) {
      display_driver_invert_rect(0, y, 128, 8);
    }
  }
}

static void render_advanced_settings(void) {
  const char *items[] = {"Factory Reset", "Clear Cache", "Debug Info",
                         "Reboot"};
  const char *values[] = {"Select", "Select", "Select",
                          "Select"}; // Placeholders

  int start = (submenu_selection / 4) * 4;
  for (int i = 0; i < 4 && (start + i) < 4; i++) {
    int y = i * 8;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%s: %s", items[start + i],
             values[start + i]);
    display_driver_draw_text(0, y, buffer, TEXT_SIZE_SMALL);

    if ((start + i) == submenu_selection) {
      display_driver_invert_rect(0, y, 128, 8);
    }
  }
}

static void render_settings_screen(void) {
  display_driver_clear();

  if (in_settings_submenu) {
    switch (settings_selection) {
    case 0:
      render_internet_settings();
      break;
    case 1:
      render_ai_config_settings();
      break;
    case 2:
      render_keyboard_settings();
      break;
    case 3:
      render_display_settings();
      break;
    case 4:
      render_power_settings();
      break;
    case 5:
      render_device_settings();
      break;
    case 6:
      render_update_settings();
      break;
    case 7:
      render_advanced_settings();
      break;
    default:
      display_driver_draw_text_centered(4, "Error", TEXT_SIZE_SMALL);
      break;
    }
  } else {
    const char *items[] = {"1.Internet", "2.AI Config", "3.Keyboard",
                           "4.Display",  "5.Power",     "6.Device",
                           "7.Update",   "8.Advanced"};

    // Show 4 items at a time
    int start = (settings_selection / 4) * 4;
    for (int i = 0; i < 4 && (start + i) < 8; i++) {
      int y = i * 8;
      display_driver_draw_text(0, y, items[start + i], TEXT_SIZE_SMALL);

      if ((start + i) == settings_selection) {
        display_driver_invert_rect(0, y, 128, 8);
      }
    }
  }

  display_driver_update();
}

static void render_error_screen(void) {
  display_driver_clear();
  display_driver_draw_text_centered(4, "Error", TEXT_SIZE_NORMAL);
  display_driver_draw_text_centered(18, error_message, TEXT_SIZE_SMALL);
  display_driver_update();
}

static void render_low_battery_screen(void) {
  display_driver_clear();
  display_driver_draw_text_centered(4, "Low Battery", TEXT_SIZE_NORMAL);
  display_driver_draw_text_centered(18, "Please Charge", TEXT_SIZE_SMALL);
  display_driver_update();
}

static void render_ota_screen(void) {
  display_driver_clear();

  char progress_str[16];
  snprintf(progress_str, sizeof(progress_str), "Updating... %d%%",
           ota_progress);

  display_driver_draw_text_centered(8, progress_str, TEXT_SIZE_NORMAL);

  // Progress bar
  int bar_width = (100 * ota_progress) / 100;
  display_driver_draw_rect(10, 22, 108, 6);
  display_driver_fill_rect(12, 24, bar_width, 2, true);

  display_driver_update();
}

static void render_bind_screen(void) {
  display_driver_clear();
  display_driver_draw_text_centered(4, "Bind Code", TEXT_SIZE_SMALL);
  display_driver_draw_text_centered(14, bind_code, TEXT_SIZE_LARGE);
  display_driver_update();
}

static void render_wifi_setup_screen(void) {
  display_driver_clear();
  display_driver_draw_text_centered(4, "WiFi Setup", TEXT_SIZE_NORMAL);
  display_driver_draw_text_centered(18, "Connect to CalX-Setup",
                                    TEXT_SIZE_SMALL);
  display_driver_update();
}

// =============================================================================
// Update (called from task)
// =============================================================================

void ui_manager_update(void) {
  if (!needs_redraw) {
    return;
  }

  if (xSemaphoreTake(ui_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    return;
  }

  switch (current_screen) {
  case STATE_BOOT:
    render_boot_screen();
    break;
  case STATE_NOT_BOUND:
    render_not_bound_screen();
    break;
  case STATE_BIND:
    render_bind_screen();
    break;
  case STATE_IDLE:
    render_idle_screen();
    break;
  case STATE_MENU:
    render_menu_screen();
    break;
  case STATE_BUSY:
    render_busy_screen();
    break;
  case STATE_CHAT:
    render_chat_screen();
    break;
  case STATE_FILE:
    render_file_screen();
    break;
  case STATE_AI:
    render_ai_screen();
    break;
  case STATE_SETTINGS:
    render_settings_screen();
    break;
  case STATE_ERROR:
    render_error_screen();
    break;
  case STATE_LOW_BATTERY:
    render_low_battery_screen();
    break;
  case STATE_OTA_UPDATE:
    render_ota_screen();
    break;
  case STATE_WIFI_SETUP:
    render_wifi_setup_screen();
    break;
  default:
    break;
  }

  needs_redraw = false;
  xSemaphoreGive(ui_mutex);
}

// =============================================================================
// State Change Handler
// =============================================================================

void ui_manager_on_state_change(calx_state_t new_state) {
  if (xSemaphoreTake(ui_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    current_screen = new_state;
    needs_redraw = true;

    // Reset screen-specific state
    if (new_state == STATE_MENU) {
      menu_selection = 0;
    } else if (new_state == STATE_SETTINGS) {
      settings_selection = 0;
    } else if (new_state == STATE_CHAT) {
      chat_scroll = 0;
      chat_page = 0;
      has_notification = false; // Clear notification when entering chat
    }

    xSemaphoreGive(ui_mutex);
  }

  // Wake screen on state change
  power_manager_reset_timeout();
}

// =============================================================================
// Public Interface
// =============================================================================

void ui_manager_show_boot_screen(void) {
  current_screen = STATE_BOOT;
  needs_redraw = true;
}

void ui_manager_set_menu_selection(int selection) {
  if (selection >= 0 && selection <= 3) {
    menu_selection = selection;
    needs_redraw = true;
  }
}

void ui_manager_set_notification(bool notification) {
  has_notification = notification;
  if (current_screen == STATE_IDLE) {
    needs_redraw = true;
  }
}

void ui_manager_show_busy(const char *message) {
  strncpy(busy_message, message, sizeof(busy_message) - 1);
  current_screen = STATE_BUSY;
  needs_redraw = true;
}

void ui_manager_show_error(const char *message) {
  strncpy(error_message, message, sizeof(error_message) - 1);
  current_screen = STATE_ERROR;
  needs_redraw = true;
}

void ui_manager_show_bind_code(const char *code) {
  strncpy(bind_code, code, 4);
  bind_code[4] = '\0';
  needs_redraw = true;
}

void ui_manager_show_ota_progress(int percent) {
  ota_progress = percent;
  current_screen = STATE_OTA_UPDATE;
  needs_redraw = true;
}

void ui_manager_set_ai_response(const char *response, bool has_more) {
  text_renderer_set_content(response, TEXT_SIZE_NORMAL);
  ai_has_more = has_more;
  needs_redraw = true;
}

void ui_manager_set_file_content(const char *content) {
  text_renderer_set_content(content, TEXT_SIZE_SMALL);
  file_scroll = 0;
  needs_redraw = true;
}

// =============================================================================
// Key Handlers
// =============================================================================

void ui_manager_handle_chat_key(calx_key_t key) {
  switch (key) {
  case KEY_UP:
    if (chat_scroll > 0)
      chat_scroll--;
    needs_redraw = true;
    break;
  case KEY_DOWN:
    chat_scroll++;
    needs_redraw = true;
    break;
  case KEY_OK:
    // Send a message - simplified for now
    // In production, would show input UI first
    api_client_send_chat("Hello from device!");
    LOG_INFO("UI", "Chat message sent");
    break;
  case KEY_EQUALS:
    chat_page++;
    chat_scroll = 0;
    needs_redraw = true;
    break;
  case KEY_DEL:
    if (chat_page > 0) {
      chat_page--;
      chat_scroll = 0;
      needs_redraw = true;
    }
    break;
  default:
    break;
  }
}

void ui_manager_handle_file_key(calx_key_t key) {
  switch (key) {
  case KEY_UP:
    if (file_scroll > 0)
      file_scroll--;
    needs_redraw = true;
    break;
  case KEY_DOWN:
    file_scroll++;
    needs_redraw = true;
    break;
  case KEY_EQUALS:
    file_scroll += 4;
    needs_redraw = true;
    break;
  case KEY_DEL:
    file_scroll = (file_scroll >= 4) ? file_scroll - 4 : 0;
    needs_redraw = true;
    break;
  default:
    break;
  }
}

void ui_manager_handle_ai_key(calx_key_t key) {
  if (key == KEY_OK && ai_has_more) {
    // Trigger fetch of next chunk (via event)
    ui_manager_show_busy("Fetching...");
  }
}

void ui_manager_handle_settings_key(calx_key_t key) {
  if (in_settings_submenu) {
    if (key == KEY_AC) {
      in_settings_submenu = false;
      needs_redraw = true;
      return;
    }

    // Submenu navigation
    switch (key) {
    case KEY_UP:
      if (submenu_selection > 0)
        submenu_selection--;
      needs_redraw = true;
      break;
    case KEY_DOWN:
      if (submenu_selection < 3) // Default limit, will specialize per menu
        submenu_selection++;
      needs_redraw = true;
      break;
    case KEY_OK:
    case KEY_EQUALS:
      // TODO: Edit value
      LOG_INFO("UI", "Edit setting: %d in menu %d", submenu_selection,
               settings_selection);
      break;
    default:
      break;
    }
    return;
  }

  // Main settings menu navigation
  switch (key) {
  case KEY_UP:
    if (settings_selection > 0)
      settings_selection--;
    needs_redraw = true;
    break;
  case KEY_DOWN:
    if (settings_selection < 7) // 8 items (0-7)
      settings_selection++;
    needs_redraw = true;
    break;
  case KEY_OK:
  case KEY_EQUALS:
    in_settings_submenu = true;
    submenu_selection = 0;
    needs_redraw = true;
    LOG_INFO("UI", "Entering settings submenu: %d", settings_selection);
    break;
  case KEY_1:
  case KEY_2:
  case KEY_3:
  case KEY_4:
  case KEY_5:
  case KEY_6:
  case KEY_7:
  case KEY_8:
    settings_selection = key - KEY_1;
    needs_redraw = true;
    break;
  default:
    break;
  }
}
