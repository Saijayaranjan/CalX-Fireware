/**
 * =============================================================================
 * CalX ESP32 Firmware - Input Manager Header
 * =============================================================================
 */

#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "calx_config.h"
#include <stdbool.h>

/**
 * Initialize the keypad input manager
 */
void input_manager_init(void);

/**
 * Scan keypad for pressed keys (called from input task)
 */
void input_manager_scan(void);

/**
 * Get the currently pressed key
 * @return Key code or KEY_NONE
 */
calx_key_t input_manager_get_key(void);

/**
 * Check if a specific key is pressed
 */
bool input_manager_is_key_pressed(calx_key_t key);

/**
 * Check if any key is pressed
 */
bool input_manager_any_key_pressed(void);

/**
 * Get last key press time (for timeout handling)
 */
uint32_t input_manager_get_last_key_time(void);

#endif // INPUT_MANAGER_H
