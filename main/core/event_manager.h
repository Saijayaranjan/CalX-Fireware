/**
 * =============================================================================
 * CalX ESP32 Firmware - Event Manager Header
 * =============================================================================
 */

#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

#include "calx_config.h"
#include <stdbool.h>

/**
 * Event structure
 */
typedef struct {
  calx_event_type_t type;
  union {
    calx_key_t key; // For KEY_PRESS events
    int value;      // Generic integer value
    void *data;     // Generic data pointer
  };
} calx_event_t;

/**
 * Event callback function type
 */
typedef void (*event_callback_t)(calx_event_t *event);

/**
 * Initialize the event manager
 */
void event_manager_init(void);

/**
 * Post an event to the queue
 * @param event Event to post
 * @return true if successful
 */
bool event_manager_post(calx_event_t *event);

/**
 * Post a simple event by type
 * @param type Event type
 * @return true if successful
 */
bool event_manager_post_simple(calx_event_type_t type);

/**
 * Post a key event
 * @param key Key code
 * @param long_press True if long press
 * @return true if successful
 */
bool event_manager_post_key(calx_key_t key, bool long_press);

/**
 * Process pending events (called from main loop)
 */
void event_manager_process(void);

/**
 * Register a callback for a specific event type
 * @param type Event type to listen for
 * @param callback Function to call when event occurs
 */
void event_manager_register(calx_event_type_t type, event_callback_t callback);

/**
 * Clear all pending events
 */
void event_manager_clear(void);

#endif // EVENT_MANAGER_H
