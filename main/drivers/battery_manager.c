/**
 * =============================================================================
 * CalX ESP32 Firmware - Battery Manager
 * =============================================================================
 * ADC-based battery voltage monitoring with smoothing and percentage mapping.
 * =============================================================================
 */

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "battery_manager.h"
#include "calx_config.h"
#include "event_manager.h"
#include "logger.h"

static const char *TAG = "BATTERY";

// =============================================================================
// State
// =============================================================================
static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t cali_handle = NULL;
static int voltage_samples[BATTERY_SAMPLE_COUNT] = {0};
static int sample_index = 0;
static int current_voltage_mv = BATTERY_FULL_MV;
static int current_percent = 100;
static bool is_low = false;
static bool was_low = false;
static bool cali_enabled = false;

// =============================================================================
// Initialization
// =============================================================================

void battery_manager_init(void) {
  // ADC1 init
  adc_oneshot_unit_init_cfg_t init_cfg = {
      .unit_id = ADC_UNIT_1,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));

  // Channel config
  adc_oneshot_chan_cfg_t chan_cfg = {
      .atten = BATTERY_ADC_ATTEN,
      .bitwidth = ADC_BITWIDTH_12,
  };
  ESP_ERROR_CHECK(
      adc_oneshot_config_channel(adc_handle, BATTERY_ADC_CHANNEL, &chan_cfg));

  // Try calibration
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
  adc_cali_curve_fitting_config_t cali_cfg = {
      .unit_id = ADC_UNIT_1,
      .chan = BATTERY_ADC_CHANNEL,
      .atten = BATTERY_ADC_ATTEN,
      .bitwidth = ADC_BITWIDTH_12,
  };
  if (adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle) == ESP_OK) {
    cali_enabled = true;
  }
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
  adc_cali_line_fitting_config_t cali_cfg = {
      .unit_id = ADC_UNIT_1,
      .atten = BATTERY_ADC_ATTEN,
      .bitwidth = ADC_BITWIDTH_12,
  };
  if (adc_cali_create_scheme_line_fitting(&cali_cfg, &cali_handle) == ESP_OK) {
    cali_enabled = true;
  }
#endif

  // Initialize samples with a reasonable default
  for (int i = 0; i < BATTERY_SAMPLE_COUNT; i++) {
    voltage_samples[i] = BATTERY_FULL_MV / BATTERY_DIVIDER_RATIO;
  }

  // Take initial reading
  battery_manager_update();

  LOG_INFO(TAG, "Battery manager initialized, voltage: %dmV (%d%%)",
           current_voltage_mv, current_percent);
}

// =============================================================================
// Voltage to Percentage Mapping
// =============================================================================

static int voltage_to_percent(int voltage_mv) {
  // Clamp to valid range
  if (voltage_mv >= BATTERY_FULL_MV) {
    return 100;
  }
  if (voltage_mv <= BATTERY_EMPTY_MV) {
    return 0;
  }

  // Linear interpolation
  int range = BATTERY_FULL_MV - BATTERY_EMPTY_MV;
  int offset = voltage_mv - BATTERY_EMPTY_MV;
  return (offset * 100) / range;
}

// =============================================================================
// Update
// =============================================================================

void battery_manager_update(void) {
  // Read ADC raw value
  int raw = 0;
  adc_oneshot_read(adc_handle, BATTERY_ADC_CHANNEL, &raw);

  // Convert to voltage in mV
  int measured_mv = 0;
  if (cali_enabled) {
    adc_cali_raw_to_voltage(cali_handle, raw, &measured_mv);
  } else {
    // Rough estimate without calibration
    measured_mv = (raw * 3300) / 4095;
  }

  // Account for voltage divider
  uint32_t actual_mv = (uint32_t)(measured_mv * BATTERY_DIVIDER_RATIO);

  // Add to moving average buffer
  voltage_samples[sample_index] = actual_mv;
  sample_index = (sample_index + 1) % BATTERY_SAMPLE_COUNT;

  // Calculate moving average
  int sum = 0;
  for (int i = 0; i < BATTERY_SAMPLE_COUNT; i++) {
    sum += voltage_samples[i];
  }
  current_voltage_mv = sum / BATTERY_SAMPLE_COUNT;

  // Calculate percentage
  current_percent = voltage_to_percent(current_voltage_mv);

  // Check for low battery transition
  was_low = is_low;
  is_low = (current_voltage_mv < BATTERY_CRITICAL_MV);

  // Post events on state change
  if (is_low && !was_low) {
    LOG_WARN(TAG, "Low battery! %dmV (%d%%)", current_voltage_mv,
             current_percent);
    event_manager_post_simple(EVENT_LOW_BATTERY);
  } else if (!is_low && was_low) {
    LOG_INFO(TAG, "Battery OK: %dmV (%d%%)", current_voltage_mv,
             current_percent);
    event_manager_post_simple(EVENT_BATTERY_OK);
  }
}

// =============================================================================
// Queries
// =============================================================================

int battery_manager_get_percent(void) { return current_percent; }

int battery_manager_get_voltage_mv(void) { return current_voltage_mv; }

bool battery_manager_is_low(void) { return is_low; }

bool battery_manager_allows_ota(void) {
  return current_percent >= BATTERY_OTA_MIN_PERCENT;
}

bool battery_manager_is_charging(void) {
  // Could be detected via a charging indicator GPIO
  // For now, return false
  return false;
}
