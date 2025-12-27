/**
 * =============================================================================
 * CalX ESP32 Firmware - Security Manager
 * =============================================================================
 * Device identity and token management.
 * =============================================================================
 */

#include "esp_log.h"
#include "esp_mac.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>

#include "calx_config.h"
#include "logger.h"
#include "security_manager.h"

static const char *TAG = "SECURITY";

// =============================================================================
// State
// =============================================================================
static nvs_handle_t sec_nvs_handle;
static char device_id[32] = {0};
static bool initialized = false;

// =============================================================================
// Initialization
// =============================================================================

void security_manager_init(void) {
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &sec_nvs_handle);
  if (err != ESP_OK) {
    LOG_ERROR(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
    return;
  }

  // Generate device ID from MAC address
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  snprintf(device_id, sizeof(device_id), "calx_%02x%02x%02x%02x%02x%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  // Check if device ID is stored, if not, store it
  char stored_id[32] = {0};
  size_t len = sizeof(stored_id);
  err = nvs_get_str(sec_nvs_handle, NVS_KEY_DEVICE_ID, stored_id, &len);

  if (err == ESP_ERR_NVS_NOT_FOUND) {
    // First boot - store device ID
    nvs_set_str(sec_nvs_handle, NVS_KEY_DEVICE_ID, device_id);
    nvs_commit(sec_nvs_handle);
    LOG_INFO(TAG, "Device ID generated: %s", device_id);
  } else if (err == ESP_OK) {
    // Use stored device ID
    strncpy(device_id, stored_id, sizeof(device_id) - 1);
    LOG_INFO(TAG, "Device ID loaded: %s", device_id);
  }

  initialized = true;
}

// =============================================================================
// Device ID
// =============================================================================

bool security_manager_get_device_id(char *id, size_t max_len) {
  if (!initialized || max_len < strlen(device_id) + 1) {
    return false;
  }

  strncpy(id, device_id, max_len - 1);
  id[max_len - 1] = '\0';
  return true;
}

// =============================================================================
// Device Token
// =============================================================================

bool security_manager_get_token(char *token, size_t max_len) {
  if (!initialized)
    return false;

  size_t len = max_len;
  esp_err_t err =
      nvs_get_str(sec_nvs_handle, NVS_KEY_DEVICE_TOKEN, token, &len);
  return (err == ESP_OK && len > 0);
}

void security_manager_set_token(const char *token) {
  if (!initialized || token == NULL)
    return;

  nvs_set_str(sec_nvs_handle, NVS_KEY_DEVICE_TOKEN, token);
  nvs_set_u8(sec_nvs_handle, NVS_KEY_BOUND, 1);
  nvs_commit(sec_nvs_handle);

  LOG_INFO(TAG, "Device token stored");
}

void security_manager_clear_token(void) {
  if (!initialized)
    return;

  nvs_erase_key(sec_nvs_handle, NVS_KEY_DEVICE_TOKEN);
  nvs_set_u8(sec_nvs_handle, NVS_KEY_BOUND, 0);
  nvs_commit(sec_nvs_handle);

  LOG_INFO(TAG, "Device token cleared");
}

// =============================================================================
// Bind Status
// =============================================================================

bool security_manager_is_bound(void) {
  if (!initialized)
    return false;

  uint8_t bound = 0;
  nvs_get_u8(sec_nvs_handle, NVS_KEY_BOUND, &bound);
  return (bound == 1);
}

void security_manager_unbind(void) {
  security_manager_clear_token();
  LOG_WARN(TAG, "Device unbound");
}
