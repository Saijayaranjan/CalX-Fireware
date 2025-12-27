/**
 * =============================================================================
 * CalX ESP32 Firmware - Power Manager Header
 * =============================================================================
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "calx_config.h"
#include <stdbool.h>

/**
 * Initialize power manager
 */
void power_manager_init(void);

/**
 * Set power mode
 */
void power_manager_set_mode(calx_power_mode_t mode);

/**
 * Get current power mode
 */
calx_power_mode_t power_manager_get_mode(void);

/**
 * Set screen timeout value in seconds
 */
void power_manager_set_screen_timeout(int seconds);

/**
 * Get screen timeout value
 */
int power_manager_get_screen_timeout(void);

/**
 * Reset screen timeout (call on user activity)
 */
void power_manager_reset_timeout(void);

/**
 * Check if screen should be off due to timeout
 */
bool power_manager_is_screen_timeout(void);

/**
 * Force enter low power mode (for low battery)
 */
void power_manager_force_low_power(void);

/**
 * Update power management (called periodically)
 */
void power_manager_update(void);

#endif // POWER_MANAGER_H
