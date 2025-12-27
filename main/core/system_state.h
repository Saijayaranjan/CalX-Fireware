/**
 * =============================================================================
 * CalX ESP32 Firmware - System State Header
 * =============================================================================
 */

#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include "calx_config.h"

/**
 * Initialize the system state machine
 */
void system_state_init(void);

/**
 * Set the current system state
 * @param state New state to transition to
 */
void system_state_set(calx_state_t state);

/**
 * Get the current system state
 * @return Current system state
 */
calx_state_t system_state_get(void);

/**
 * Get the previous system state (for back navigation)
 * @return Previous system state
 */
calx_state_t system_state_get_previous(void);

/**
 * Go back to previous state
 */
void system_state_go_back(void);

/**
 * Go to idle state (AC long press)
 */
void system_state_go_idle(void);

/**
 * Process network operations based on current state
 * Called from network task
 */
void system_state_process_network(void);

/**
 * Handle key press in current state
 * @param key The key that was pressed
 * @param long_press True if this was a long press
 */
void system_state_handle_key(calx_key_t key, bool long_press);

/**
 * Check if system is in a busy state (network operation)
 * @return True if busy
 */
bool system_state_is_busy(void);

/**
 * Set error state with message
 * @param error_msg Error message to display
 */
void system_state_set_error(const char *error_msg);

#endif // SYSTEM_STATE_H
