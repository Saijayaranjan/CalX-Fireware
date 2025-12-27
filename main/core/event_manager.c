/**
 * =============================================================================
 * CalX ESP32 Firmware - Event Manager
 * =============================================================================
 * Decoupled event system for communication between modules.
 * =============================================================================
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <string.h>

#include "event_manager.h"
#include "logger.h"
#include "system_state.h"

static const char *TAG = "EVENT_MGR";

// =============================================================================
// Configuration
// =============================================================================
#define EVENT_QUEUE_SIZE 32
#define MAX_CALLBACKS 8

// =============================================================================
// Callback Registration
// =============================================================================
typedef struct {
  calx_event_type_t type;
  event_callback_t callback;
} event_listener_t;

// =============================================================================
// State
// =============================================================================
static QueueHandle_t event_queue = NULL;
static event_listener_t listeners[MAX_CALLBACKS];
static int listener_count = 0;

// =============================================================================
// Initialization
// =============================================================================

void event_manager_init(void) {
  event_queue = xQueueCreate(EVENT_QUEUE_SIZE, sizeof(calx_event_t));
  if (event_queue == NULL) {
    LOG_ERROR(TAG, "Failed to create event queue");
    return;
  }

  memset(listeners, 0, sizeof(listeners));
  listener_count = 0;

  LOG_INFO(TAG, "Event manager initialized");
}

// =============================================================================
// Event Posting
// =============================================================================

bool event_manager_post(calx_event_t *event) {
  if (event_queue == NULL || event == NULL) {
    return false;
  }

  BaseType_t result = xQueueSend(event_queue, event, pdMS_TO_TICKS(10));
  if (result != pdTRUE) {
    LOG_WARN(TAG, "Event queue full, dropping event type %d", event->type);
    return false;
  }

  return true;
}

bool event_manager_post_simple(calx_event_type_t type) {
  calx_event_t event = {.type = type, .value = 0};
  return event_manager_post(&event);
}

bool event_manager_post_key(calx_key_t key, bool long_press) {
  calx_event_t event = {
      .type = long_press ? EVENT_KEY_LONG_PRESS : EVENT_KEY_PRESS, .key = key};
  return event_manager_post(&event);
}

// =============================================================================
// Event Processing
// =============================================================================

void event_manager_process(void) {
  if (event_queue == NULL) {
    return;
  }

  calx_event_t event;

  // Process all pending events
  while (xQueueReceive(event_queue, &event, 0) == pdTRUE) {
    // Handle key events through state machine
    if (event.type == EVENT_KEY_PRESS) {
      system_state_handle_key(event.key, false);
    } else if (event.type == EVENT_KEY_LONG_PRESS) {
      system_state_handle_key(event.key, true);
    }

    // Notify registered listeners
    for (int i = 0; i < listener_count; i++) {
      if (listeners[i].type == event.type && listeners[i].callback != NULL) {
        listeners[i].callback(&event);
      }
    }
  }
}

// =============================================================================
// Callback Registration
// =============================================================================

void event_manager_register(calx_event_type_t type, event_callback_t callback) {
  if (listener_count >= MAX_CALLBACKS) {
    LOG_WARN(TAG, "Max callbacks reached");
    return;
  }

  listeners[listener_count].type = type;
  listeners[listener_count].callback = callback;
  listener_count++;

  LOG_DEBUG(TAG, "Registered callback for event type %d", type);
}

// =============================================================================
// Utility
// =============================================================================

void event_manager_clear(void) {
  if (event_queue != NULL) {
    xQueueReset(event_queue);
  }
}
