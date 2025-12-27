/**
 * =============================================================================
 * CalX ESP32 Firmware - Battery Manager Header
 * =============================================================================
 */

#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Initialize battery monitoring
 */
void battery_manager_init(void);

/**
 * Update battery reading (called from battery task)
 */
void battery_manager_update(void);

/**
 * Get current battery percentage (0-100)
 */
int battery_manager_get_percent(void);

/**
 * Get current battery voltage in mV
 */
int battery_manager_get_voltage_mv(void);

/**
 * Check if battery is low (below critical threshold)
 */
bool battery_manager_is_low(void);

/**
 * Check if battery level allows OTA (>= 30%)
 */
bool battery_manager_allows_ota(void);

/**
 * Check if device is currently charging
 */
bool battery_manager_is_charging(void);

#endif // BATTERY_MANAGER_H
