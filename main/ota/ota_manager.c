/**
 * =============================================================================
 * CalX ESP32 Firmware - OTA Manager
 * =============================================================================
 * Firmware over-the-air update with dual partition and rollback support.
 * =============================================================================
 */

#include "esp_crt_bundle.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#include "api_client.h"
#include "battery_manager.h"
#include "calx_config.h"
#include "event_manager.h"
#include "logger.h"
#include "ota_manager.h"
#include "ui_manager.h"

static const char *TAG = "OTA";

// =============================================================================
// State
// =============================================================================
static update_info_t update_info = {0};
static bool is_updating = false;
static int progress = 0;
static TaskHandle_t ota_task_handle = NULL;

// =============================================================================
// Initialization
// =============================================================================

void ota_manager_init(void) {
  // Check if current firmware is valid
  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_img_states_t ota_state;

  if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
      LOG_INFO(TAG, "First boot after OTA, marking as valid");
      esp_ota_mark_app_valid_cancel_rollback();
    }
  }

  LOG_INFO(TAG, "OTA manager initialized, running: %s", running->label);
}

// =============================================================================
// Update Check
// =============================================================================

bool ota_manager_check_update(void) {
  bool available = api_client_check_update(&update_info);

  if (available) {
    LOG_INFO(TAG, "Update available: v%s -> v%s", CALX_FW_VERSION,
             update_info.version);
    event_manager_post_simple(EVENT_OTA_AVAILABLE);
  }

  return available;
}

const char *ota_manager_get_available_version(void) {
  return update_info.available ? update_info.version : NULL;
}

// =============================================================================
// OTA Update Task
// =============================================================================

static void ota_update_task(void *pvParameters) {
  LOG_INFO(TAG, "Starting OTA update to v%s", update_info.version);

  is_updating = true;
  progress = 0;
  ui_manager_show_ota_progress(0);

  // Configure HTTPS OTA
  esp_http_client_config_t http_config = {
      .url = update_info.download_url,
      .crt_bundle_attach = esp_crt_bundle_attach,
      .timeout_ms = OTA_RECV_TIMEOUT_MS,
  };

  esp_https_ota_config_t ota_config = {
      .http_config = &http_config,
  };

  esp_https_ota_handle_t ota_handle = NULL;
  esp_err_t err = esp_https_ota_begin(&ota_config, &ota_handle);

  if (err != ESP_OK) {
    LOG_ERROR(TAG, "OTA begin failed: %s", esp_err_to_name(err));
    goto ota_failed;
  }

  // Get image size for progress calculation
  int image_size = esp_https_ota_get_image_size(ota_handle);
  int bytes_read = 0;

  // Download and write
  while (1) {
    err = esp_https_ota_perform(ota_handle);

    if (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
      bytes_read = esp_https_ota_get_image_len_read(ota_handle);
      if (image_size > 0) {
        progress = (bytes_read * 100) / image_size;
        ui_manager_show_ota_progress(progress);
      }
      continue;
    }

    if (err == ESP_OK) {
      break; // Download complete
    }

    LOG_ERROR(TAG, "OTA perform failed: %s", esp_err_to_name(err));
    esp_https_ota_abort(ota_handle);
    goto ota_failed;
  }

  // Verify and finalize
  if (!esp_https_ota_is_complete_data_received(ota_handle)) {
    LOG_ERROR(TAG, "Incomplete data received");
    esp_https_ota_abort(ota_handle);
    goto ota_failed;
  }

  err = esp_https_ota_finish(ota_handle);
  if (err != ESP_OK) {
    LOG_ERROR(TAG, "OTA finish failed: %s", esp_err_to_name(err));
    goto ota_failed;
  }

  // Success
  LOG_INFO(TAG, "OTA update successful!");
  ui_manager_show_ota_progress(100);
  api_client_report_update(update_info.version, true);
  event_manager_post_simple(EVENT_OTA_COMPLETE);

  // Wait a moment then reboot
  vTaskDelay(pdMS_TO_TICKS(2000));
  esp_restart();

ota_failed:
  is_updating = false;
  progress = 0;
  api_client_report_update(update_info.version, false);
  event_manager_post_simple(EVENT_OTA_FAILED);
  ui_manager_show_error("Update Failed");

  ota_task_handle = NULL;
  vTaskDelete(NULL);
}

// =============================================================================
// Start Update
// =============================================================================

bool ota_manager_start_update(void) {
  // Check battery
  if (!battery_manager_allows_ota()) {
    LOG_WARN(TAG, "Battery too low for OTA (%d%% < %d%%)",
             battery_manager_get_percent(), BATTERY_OTA_MIN_PERCENT);
    ui_manager_show_error("Charge Required");
    return false;
  }

  // Check if update available
  if (!update_info.available) {
    LOG_WARN(TAG, "No update available");
    return false;
  }

  // Check not already updating
  if (is_updating) {
    LOG_WARN(TAG, "Update already in progress");
    return false;
  }

  // Start OTA task
  xTaskCreate(ota_update_task, "ota_task", 8192, NULL, 5, &ota_task_handle);

  return true;
}

// =============================================================================
// Queries
// =============================================================================

int ota_manager_get_progress(void) { return progress; }

bool ota_manager_is_updating(void) { return is_updating; }

// =============================================================================
// Rollback
// =============================================================================

bool ota_manager_rollback(void) {
  LOG_WARN(TAG, "Rolling back to previous firmware");

  esp_err_t err = esp_ota_mark_app_invalid_rollback_and_reboot();

  // If we get here, rollback failed
  LOG_ERROR(TAG, "Rollback failed: %s", esp_err_to_name(err));
  return false;
}

void ota_manager_mark_valid(void) {
  esp_ota_mark_app_valid_cancel_rollback();
  LOG_INFO(TAG, "Firmware marked as valid");
}
