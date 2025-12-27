/**
 * =============================================================================
 * CalX ESP32 Firmware - OTA Manager Header
 * =============================================================================
 */

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <stdbool.h>

/**
 * Initialize OTA manager
 */
void ota_manager_init(void);

/**
 * Check if firmware update is available
 * @return true if update available
 */
bool ota_manager_check_update(void);

/**
 * Start OTA update
 * @return true if update started
 */
bool ota_manager_start_update(void);

/**
 * Get current update progress (0-100)
 */
int ota_manager_get_progress(void);

/**
 * Check if update is in progress
 */
bool ota_manager_is_updating(void);

/**
 * Rollback to previous firmware
 * @return true if rollback succeeded
 */
bool ota_manager_rollback(void);

/**
 * Get available update version (if any)
 */
const char *ota_manager_get_available_version(void);

/**
 * Mark current firmware as valid (after successful boot)
 */
void ota_manager_mark_valid(void);

#endif // OTA_MANAGER_H
