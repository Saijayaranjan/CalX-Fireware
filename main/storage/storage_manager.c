/**
 * =============================================================================
 * CalX ESP32 Firmware - Storage Manager
 * =============================================================================
 * NVS-based persistent storage for settings and credentials.
 * =============================================================================
 */

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

#include "logger.h"
#include "storage_manager.h"

static const char *TAG = "STORAGE";

// =============================================================================
// State
// =============================================================================
static nvs_handle_t stor_nvs_handle;
static bool initialized = false;

// =============================================================================
// Initialization
// =============================================================================

void storage_manager_init(void) {
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &stor_nvs_handle);
  if (err != ESP_OK) {
    LOG_ERROR(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
    return;
  }

  initialized = true;
  LOG_INFO(TAG, "Storage manager initialized");
}

// =============================================================================
// WiFi Credentials
// =============================================================================

bool storage_manager_get_wifi_ssid(char *ssid, size_t max_len) {
  if (!initialized)
    return false;

  size_t len = max_len;
  esp_err_t err = nvs_get_str(stor_nvs_handle, NVS_KEY_WIFI_SSID, ssid, &len);
  return (err == ESP_OK);
}

bool storage_manager_get_wifi_pass(char *pass, size_t max_len) {
  if (!initialized)
    return false;

  size_t len = max_len;
  esp_err_t err = nvs_get_str(stor_nvs_handle, NVS_KEY_WIFI_PASS, pass, &len);
  return (err == ESP_OK);
}

void storage_manager_set_wifi_credentials(const char *ssid, const char *pass) {
  if (!initialized)
    return;

  nvs_set_str(stor_nvs_handle, NVS_KEY_WIFI_SSID, ssid);
  nvs_set_str(stor_nvs_handle, NVS_KEY_WIFI_PASS, pass);
  nvs_commit(stor_nvs_handle);

  LOG_INFO(TAG, "WiFi credentials saved");
}

bool storage_manager_has_wifi_credentials(void) {
  char ssid[33];
  return storage_manager_get_wifi_ssid(ssid, sizeof(ssid));
}

void storage_manager_clear_wifi_credentials(void) {
  if (!initialized)
    return;

  nvs_erase_key(stor_nvs_handle, NVS_KEY_WIFI_SSID);
  nvs_erase_key(stor_nvs_handle, NVS_KEY_WIFI_PASS);
  nvs_commit(stor_nvs_handle);

  LOG_INFO(TAG, "WiFi credentials cleared");
}

// =============================================================================
// Power Mode
// =============================================================================

calx_power_mode_t storage_manager_get_power_mode(void) {
  if (!initialized)
    return POWER_MODE_NORMAL;

  uint8_t mode = POWER_MODE_NORMAL;
  nvs_get_u8(stor_nvs_handle, NVS_KEY_POWER_MODE, &mode);
  return (calx_power_mode_t)mode;
}

void storage_manager_set_power_mode(calx_power_mode_t mode) {
  if (!initialized)
    return;

  nvs_set_u8(stor_nvs_handle, NVS_KEY_POWER_MODE, (uint8_t)mode);
  nvs_commit(stor_nvs_handle);
}

// =============================================================================
// Text Size
// =============================================================================

calx_text_size_t storage_manager_get_text_size(void) {
  if (!initialized)
    return TEXT_SIZE_NORMAL;

  uint8_t size = TEXT_SIZE_NORMAL;
  nvs_get_u8(stor_nvs_handle, NVS_KEY_TEXT_SIZE, &size);
  return (calx_text_size_t)size;
}

void storage_manager_set_text_size(calx_text_size_t size) {
  if (!initialized)
    return;

  nvs_set_u8(stor_nvs_handle, NVS_KEY_TEXT_SIZE, (uint8_t)size);
  nvs_commit(stor_nvs_handle);
}

// =============================================================================
// Keyboard
// =============================================================================

calx_keyboard_t storage_manager_get_keyboard(void) {
  if (!initialized)
    return KEYBOARD_QWERTY;

  uint8_t kb = KEYBOARD_QWERTY;
  nvs_get_u8(stor_nvs_handle, NVS_KEY_KEYBOARD, &kb);
  return (calx_keyboard_t)kb;
}

void storage_manager_set_keyboard(calx_keyboard_t keyboard) {
  if (!initialized)
    return;

  nvs_set_u8(stor_nvs_handle, NVS_KEY_KEYBOARD, (uint8_t)keyboard);
  nvs_commit(stor_nvs_handle);
}

// =============================================================================
// Screen Timeout
// =============================================================================

int storage_manager_get_screen_timeout(void) {
  if (!initialized)
    return SCREEN_TIMEOUT_DEFAULT_S;

  int32_t timeout = SCREEN_TIMEOUT_DEFAULT_S;
  nvs_get_i32(stor_nvs_handle, NVS_KEY_SCREEN_TIMEOUT, &timeout);
  return (int)timeout;
}

void storage_manager_set_screen_timeout(int seconds) {
  if (!initialized)
    return;

  nvs_set_i32(stor_nvs_handle, NVS_KEY_SCREEN_TIMEOUT, seconds);
  nvs_commit(stor_nvs_handle);
}

// =============================================================================
// Factory Reset
// =============================================================================

void storage_manager_factory_reset(void) {
  if (!initialized)
    return;

  LOG_WARN(TAG, "Factory reset initiated");

  // Clear all keys except device ID
  nvs_erase_key(stor_nvs_handle, NVS_KEY_WIFI_SSID);
  nvs_erase_key(stor_nvs_handle, NVS_KEY_WIFI_PASS);
  nvs_erase_key(stor_nvs_handle, NVS_KEY_DEVICE_TOKEN);
  nvs_erase_key(stor_nvs_handle, NVS_KEY_POWER_MODE);
  nvs_erase_key(stor_nvs_handle, NVS_KEY_TEXT_SIZE);
  nvs_erase_key(stor_nvs_handle, NVS_KEY_KEYBOARD);
  nvs_erase_key(stor_nvs_handle, NVS_KEY_SCREEN_TIMEOUT);
  nvs_erase_key(stor_nvs_handle, NVS_KEY_BOUND);

  nvs_commit(stor_nvs_handle);

  LOG_INFO(TAG, "Factory reset complete");
}

void storage_manager_clear_cache(void) {
  // No persistent cache to clear in current implementation
  LOG_INFO(TAG, "Cache cleared");
}
