/**
 * =============================================================================
 * CalX ESP32 Firmware - Time Manager Header
 * =============================================================================
 */

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <stdbool.h>
#include <time.h>

/**
 * Initialize time manager and start NTP sync
 */
void time_manager_init(void);

/**
 * Check if time has been synced via NTP
 * @return true if synced
 */
bool time_manager_is_synced(void);

/**
 * Get current Unix timestamp
 * @return Unix timestamp or 0 if not synced
 */
time_t time_manager_get_timestamp(void);

/**
 * Get formatted time string (HH:MM)
 * @return Static buffer with time string
 */
const char *time_manager_get_time_str(void);

/**
 * Get formatted date string (DD/MM/YY)
 * @return Static buffer with date string
 */
const char *time_manager_get_date_str(void);

/**
 * Force NTP sync
 */
void time_manager_sync(void);

#endif // TIME_MANAGER_H
