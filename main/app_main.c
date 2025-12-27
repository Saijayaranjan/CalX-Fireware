/**
 * =============================================================================
 * CalX ESP32 Firmware - Application Entry Point
 * =============================================================================
 * Main entry point. Initializes all subsystems and starts the main loop.
 * =============================================================================
 */

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>

#include "api_client.h"
#include "battery_manager.h"
#include "calx_config.h"
#include "display_driver.h"
#include "event_manager.h"
#include "input_manager.h"
#include "logger.h"
#include "power_manager.h"
#include "security_manager.h"
#include "storage_manager.h"
#include "system_state.h"
#include "time_manager.h"
#include "ui_manager.h"
#include "wifi_manager.h"

static const char *TAG = "CALX_MAIN";

// Task handles
static TaskHandle_t ui_task_handle = NULL;
static TaskHandle_t input_task_handle = NULL;
static TaskHandle_t network_task_handle = NULL;
static TaskHandle_t battery_task_handle = NULL;

// =============================================================================
// Task Priorities
// =============================================================================
#define TASK_PRIORITY_UI 5
#define TASK_PRIORITY_INPUT 6
#define TASK_PRIORITY_NETWORK 4
#define TASK_PRIORITY_BATTERY 3

// =============================================================================
// Task Stack Sizes
// =============================================================================
#define TASK_STACK_UI 4096
#define TASK_STACK_INPUT 2048
#define TASK_STACK_NETWORK 8192
#define TASK_STACK_BATTERY 2048

// =============================================================================
// Initialize NVS
// =============================================================================
static esp_err_t init_nvs(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGW(TAG, "NVS partition was truncated, erasing...");
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  return ret;
}

// =============================================================================
// UI Task - Handles display rendering
// =============================================================================
static void ui_task(void *pvParameters) {
  LOG_INFO(TAG, "UI task started");

  while (1) {
    ui_manager_update();
    vTaskDelay(pdMS_TO_TICKS(33)); // ~30 FPS
  }
}

// =============================================================================
// Input Task - Handles keypad scanning
// =============================================================================
static void input_task(void *pvParameters) {
  LOG_INFO(TAG, "Input task started");

  while (1) {
    input_manager_scan();
    vTaskDelay(pdMS_TO_TICKS(KEYPAD_SCAN_INTERVAL_MS));
  }
}

// =============================================================================
// Network Task - Handles API communication
// =============================================================================
static void network_task(void *pvParameters) {
  TickType_t last_heartbeat = xTaskGetTickCount();
  TickType_t last_bind_check = 0;
  TickType_t last_settings_fetch = 0;
  TickType_t last_ota_check = 0;
  bool bind_code_requested = false;
  char bind_code[5] = {0};

  while (1) {
    calx_state_t state = system_state_get();

    // Handle not bound state - request bind code once
    if (state == STATE_NOT_BOUND && !bind_code_requested &&
        wifi_manager_is_connected()) {
      int expires_in;
      if (api_client_request_bind_code(bind_code, &expires_in)) {
        ui_manager_show_bind_code(bind_code);
        system_state_set(STATE_BIND);
        bind_code_requested = true;
        last_bind_check = xTaskGetTickCount();
        LOG_INFO(TAG, "Bind code displayed: %s", bind_code);
      }
    }

    // Poll bind status every 5 seconds when in bind state
    if (state == STATE_BIND) {
      TickType_t now = xTaskGetTickCount();
      if ((now - last_bind_check) >= pdMS_TO_TICKS(5000)) {
        char token[128];
        if (api_client_check_bind_status(token)) {
          // Device is now bound!
          security_manager_set_token(token);
          LOG_INFO(TAG, "Device bound successfully!");
          system_state_set(STATE_IDLE);
          bind_code_requested = false; // Reset for next time
        }
        last_bind_check = now;
      }
    }

    // Send heartbeat every 60 seconds (if bound)
    if (security_manager_is_bound() && wifi_manager_is_connected()) {
      TickType_t now = xTaskGetTickCount();
      if ((now - last_heartbeat) >= pdMS_TO_TICKS(60000)) {
        api_client_send_heartbeat();
        last_heartbeat = now;
      }
    }

    // Fetch settings every 5 minutes (if bound)
    if (security_manager_is_bound() && wifi_manager_is_connected()) {
      TickType_t now = xTaskGetTickCount();
      if ((now - last_settings_fetch) >= pdMS_TO_TICKS(300000)) {
        api_client_fetch_settings();
        last_settings_fetch = now;
      }
    }

    // Check for OTA updates daily (if bound)
    if (security_manager_is_bound() && wifi_manager_is_connected()) {
      TickType_t now = xTaskGetTickCount();
      if ((now - last_ota_check) >= pdMS_TO_TICKS(86400000)) {
        update_info_t info;
        if (api_client_check_update(&info)) {
          LOG_INFO(TAG, "OTA update available: %s", info.version);
        }
        last_ota_check = now;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// =============================================================================
// Battery Task - Monitors battery level
// =============================================================================
static void battery_task(void *pvParameters) {
  LOG_INFO(TAG, "Battery task started");

  while (1) {
    battery_manager_update();
    vTaskDelay(pdMS_TO_TICKS(BATTERY_UPDATE_MS));
  }
}

// =============================================================================
// Main Application Entry
// =============================================================================
void app_main(void) {
  // =========================================================================
  // Phase 1: Core Initialization
  // =========================================================================

  // Initialize logging first
  logger_init();
  LOG_INFO(TAG, "=================================");
  LOG_INFO(TAG, "CalX Firmware v%s", CALX_FW_VERSION);
  LOG_INFO(TAG, "=================================");

  // Initialize NVS (required for WiFi and storage)
  ESP_ERROR_CHECK(init_nvs());
  LOG_INFO(TAG, "NVS initialized");

  // Initialize TCP/IP and event loop
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  LOG_INFO(TAG, "Network stack initialized");

  // =========================================================================
  // Phase 2: Storage & Security
  // =========================================================================

  storage_manager_init();
  LOG_INFO(TAG, "Storage manager initialized");

  security_manager_init();
  LOG_INFO(TAG, "Security manager initialized");

  // =========================================================================
  // Phase 3: Hardware Initialization
  // =========================================================================

  // Initialize display
  display_driver_init();
  LOG_INFO(TAG, "Display initialized");

  // Show boot screen immediately
  ui_manager_init();
  ui_manager_show_boot_screen();

  // Initialize input (keypad)
  input_manager_init();
  LOG_INFO(TAG, "Input manager initialized");

  // Initialize battery monitoring
  battery_manager_init();
  LOG_INFO(TAG, "Battery manager initialized");

  // Initialize power manager
  power_manager_init();
  LOG_INFO(TAG, "Power manager initialized");

  // =========================================================================
  // Phase 4: Event System
  // =========================================================================

  event_manager_init();
  LOG_INFO(TAG, "Event manager initialized");

  // =========================================================================
  // Phase 5: System State Machine
  // =========================================================================

  system_state_init();
  LOG_INFO(TAG, "System state initialized");

  // =========================================================================
  // Phase 6: WiFi Initialization
  // =========================================================================

  wifi_manager_init();
  LOG_INFO(TAG, "WiFi manager initialized");

  // Initialize time manager (for NTP sync)
  time_manager_init();
  LOG_INFO(TAG, "Time manager initialized");

  // =========================================================================
  // Phase 7: Start Tasks
  // =========================================================================

  LOG_INFO(TAG, "Starting tasks...");

  xTaskCreate(ui_task, "ui_task", TASK_STACK_UI, NULL, TASK_PRIORITY_UI,
              &ui_task_handle);
  xTaskCreate(input_task, "input_task", TASK_STACK_INPUT, NULL,
              TASK_PRIORITY_INPUT, &input_task_handle);
  xTaskCreate(network_task, "network_task", TASK_STACK_NETWORK, NULL,
              TASK_PRIORITY_NETWORK, &network_task_handle);
  xTaskCreate(battery_task, "battery_task", TASK_STACK_BATTERY, NULL,
              TASK_PRIORITY_BATTERY, &battery_task_handle);

  LOG_INFO(TAG, "All tasks started");

  // =========================================================================
  // Phase 8: Transition from Boot
  // =========================================================================

  // Small delay for boot screen visibility
  vTaskDelay(pdMS_TO_TICKS(1500));

  // Check if device is bound
  if (security_manager_is_bound()) {
    // Try to connect to stored WiFi
    if (wifi_manager_has_credentials()) {
      LOG_INFO(TAG, "Attempting WiFi connection...");
      wifi_manager_connect();
      // Start web server for remote access
      wifi_manager_start_webserver();
      system_state_set(STATE_IDLE);
    } else {
      LOG_INFO(TAG, "No WiFi credentials, starting AP mode");
      wifi_manager_start_ap();
      system_state_set(STATE_WIFI_SETUP);
    }
  } else {
    LOG_INFO(TAG, "Device not bound, starting AP mode");
    wifi_manager_start_ap();
    system_state_set(STATE_NOT_BOUND);
  }

  LOG_INFO(TAG, "CalX initialization complete");

  // Main task can now idle - other tasks handle the work
  while (1) {
    // Process events in main loop
    event_manager_process();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
