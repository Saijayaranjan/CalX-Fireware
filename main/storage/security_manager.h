/**
 * =============================================================================
 * CalX ESP32 Firmware - Security Manager Header
 * =============================================================================
 */

#ifndef SECURITY_MANAGER_H
#define SECURITY_MANAGER_H

#include <stdbool.h>
#include <stddef.h>

/**
 * Initialize security manager
 */
void security_manager_init(void);

/**
 * Get device ID (derived from MAC address)
 * @param device_id Buffer to store device ID
 * @param max_len Buffer size
 * @return true if successful
 */
bool security_manager_get_device_id(char *device_id, size_t max_len);

/**
 * Get device token (for API authentication)
 * @param token Buffer to store token
 * @param max_len Buffer size
 * @return true if token exists
 */
bool security_manager_get_token(char *token, size_t max_len);

/**
 * Set device token (after successful binding)
 * @param token Token to store
 */
void security_manager_set_token(const char *token);

/**
 * Clear device token (unbind)
 */
void security_manager_clear_token(void);

/**
 * Check if device is bound (has token)
 * @return true if bound
 */
bool security_manager_is_bound(void);

/**
 * Perform full unbind (clear token, mark as unbound)
 */
void security_manager_unbind(void);

#endif // SECURITY_MANAGER_H
