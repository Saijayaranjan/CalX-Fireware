/**
 * =============================================================================
 * CalX ESP32 Firmware - Logger
 * =============================================================================
 * Logging system with levels, uptime tracking, and buffer for debug screen.
 * =============================================================================
 */

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "logger.h"

// =============================================================================
// Configuration
// =============================================================================
#define LOG_BUFFER_SIZE 512
#define LOG_LINE_SIZE 80
#define LOG_MAX_LINES 8

// =============================================================================
// State
// =============================================================================
static log_level_t min_level = LOG_LEVEL_INFO;
static char log_buffer[LOG_BUFFER_SIZE];
static int buffer_pos = 0;
static int64_t start_time_us = 0;

// Level strings
static const char *level_strings[] = {"DEBUG", "INFO", "WARN", "ERROR"};

// ESP-IDF log level mapping
static const esp_log_level_t esp_levels[] = {ESP_LOG_DEBUG, ESP_LOG_INFO,
                                             ESP_LOG_WARN, ESP_LOG_ERROR};

// =============================================================================
// Initialization
// =============================================================================

void logger_init(void) {
  start_time_us = esp_timer_get_time();
  memset(log_buffer, 0, sizeof(log_buffer));
  buffer_pos = 0;

  // Set ESP-IDF log level
  esp_log_level_set("*", ESP_LOG_INFO);
}

void logger_set_level(log_level_t level) { min_level = level; }

// =============================================================================
// Logging
// =============================================================================

void logger_log(log_level_t level, const char *tag, const char *format, ...) {
  if (level < min_level) {
    return;
  }

  // Format the message
  char message[LOG_LINE_SIZE];
  va_list args;
  va_start(args, format);
  vsnprintf(message, sizeof(message), format, args);
  va_end(args);

  // Log to ESP-IDF
  esp_log_write(esp_levels[level], tag, "%s: %s\n", level_strings[level],
                message);

  // Add to circular buffer for debug screen
  int len = snprintf(log_buffer + buffer_pos, LOG_LINE_SIZE, "[%s] %s\n",
                     level_strings[level], message);
  buffer_pos += len;

  // Wrap buffer
  if (buffer_pos >= LOG_BUFFER_SIZE - LOG_LINE_SIZE) {
    buffer_pos = 0;
  }
}

// =============================================================================
// Uptime
// =============================================================================

const char *logger_get_uptime(void) {
  static char uptime_str[32];

  int64_t elapsed_us = esp_timer_get_time() - start_time_us;
  int64_t elapsed_s = elapsed_us / 1000000;

  int hours = elapsed_s / 3600;
  int minutes = (elapsed_s % 3600) / 60;
  int seconds = elapsed_s % 60;

  if (hours > 0) {
    snprintf(uptime_str, sizeof(uptime_str), "%dh %dm", hours, minutes);
  } else if (minutes > 0) {
    snprintf(uptime_str, sizeof(uptime_str), "%dm %ds", minutes, seconds);
  } else {
    snprintf(uptime_str, sizeof(uptime_str), "%ds", seconds);
  }

  return uptime_str;
}

const char *logger_get_buffer(void) { return log_buffer; }
