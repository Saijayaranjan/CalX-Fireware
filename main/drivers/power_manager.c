/**
 * =============================================================================
 * CalX ESP32 Firmware - Power Manager
 * =============================================================================
 * Power mode management and screen timeout handling.
 * =============================================================================
 */

#include "esp_log.h"
#include "esp_pm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "display_driver.h"
#include "logger.h"
#include "power_manager.h"
#include "storage_manager.h"

static const char *TAG = "POWER";

// =============================================================================
// State
// =============================================================================
static calx_power_mode_t current_mode = POWER_MODE_NORMAL;
static int screen_timeout_s = SCREEN_TIMEOUT_DEFAULT_S;
static uint32_t last_activity_time = 0;
static bool screen_off = false;
static bool forced_low_power = false;

// =============================================================================
// Initialization
// =============================================================================

void power_manager_init(void) {
  // Load settings from NVS
  current_mode = storage_manager_get_power_mode();
  screen_timeout_s = storage_manager_get_screen_timeout();

  if (screen_timeout_s < SCREEN_TIMEOUT_MIN_S) {
    screen_timeout_s = SCREEN_TIMEOUT_DEFAULT_S;
  }

  last_activity_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

// Configure ESP32 power management
#if CONFIG_PM_ENABLE
  esp_pm_config_esp32_t pm_config = {
      .max_freq_mhz = 240,
      .min_freq_mhz = (current_mode == POWER_MODE_LOW) ? 80 : 160,
      .light_sleep_enable = (current_mode == POWER_MODE_LOW),
  };
  esp_pm_configure(&pm_config);
#endif

  LOG_INFO(TAG, "Power manager initialized (mode: %s, timeout: %ds)",
           current_mode == POWER_MODE_NORMAL ? "NORMAL" : "LOW",
           screen_timeout_s);
}

// =============================================================================
// Mode Management
// =============================================================================

void power_manager_set_mode(calx_power_mode_t mode) {
  if (mode == current_mode) {
    return;
  }

  current_mode = mode;
  storage_manager_set_power_mode(mode);

#if CONFIG_PM_ENABLE
  esp_pm_config_esp32_t pm_config = {
      .max_freq_mhz = 240,
      .min_freq_mhz = (mode == POWER_MODE_LOW) ? 80 : 160,
      .light_sleep_enable = (mode == POWER_MODE_LOW),
  };
  esp_pm_configure(&pm_config);
#endif

  LOG_INFO(TAG, "Power mode changed to: %s",
           mode == POWER_MODE_NORMAL ? "NORMAL" : "LOW");
}

calx_power_mode_t power_manager_get_mode(void) {
  return forced_low_power ? POWER_MODE_LOW : current_mode;
}

// =============================================================================
// Screen Timeout
// =============================================================================

void power_manager_set_screen_timeout(int seconds) {
  if (seconds < SCREEN_TIMEOUT_MIN_S) {
    seconds = SCREEN_TIMEOUT_MIN_S;
  } else if (seconds > SCREEN_TIMEOUT_MAX_S) {
    seconds = SCREEN_TIMEOUT_MAX_S;
  }

  screen_timeout_s = seconds;
  storage_manager_set_screen_timeout(seconds);

  LOG_INFO(TAG, "Screen timeout set to: %ds", seconds);
}

int power_manager_get_screen_timeout(void) { return screen_timeout_s; }

void power_manager_reset_timeout(void) {
  last_activity_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

  // Turn screen back on if it was off
  if (screen_off) {
    screen_off = false;
    display_driver_power(true);
    LOG_DEBUG(TAG, "Screen on (activity)");
  }
}

bool power_manager_is_screen_timeout(void) { return screen_off; }

// =============================================================================
// Low Power Mode
// =============================================================================

void power_manager_force_low_power(void) {
  if (!forced_low_power) {
    forced_low_power = true;
    LOG_WARN(TAG, "Forced low power mode enabled");

// Apply low power PM settings immediately
#if CONFIG_PM_ENABLE
    esp_pm_config_esp32_t pm_config = {
        .max_freq_mhz = 160,
        .min_freq_mhz = 80,
        .light_sleep_enable = true,
    };
    esp_pm_configure(&pm_config);
#endif
  }
}

// =============================================================================
// Update
// =============================================================================

void power_manager_update(void) {
  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
  uint32_t elapsed_s = (now - last_activity_time) / 1000;

  // Check for screen timeout
  if (!screen_off && elapsed_s >= screen_timeout_s) {
    screen_off = true;
    display_driver_power(false);
    LOG_DEBUG(TAG, "Screen off (timeout)");
  }
}
