/**
 * =============================================================================
 * CalX ESP32 Firmware - Logger Header
 * =============================================================================
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>

// Log levels
typedef enum {
  LOG_LEVEL_DEBUG = 0,
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARN,
  LOG_LEVEL_ERROR
} log_level_t;

/**
 * Initialize logging system
 */
void logger_init(void);

/**
 * Set minimum log level
 */
void logger_set_level(log_level_t level);

/**
 * Log a message
 */
void logger_log(log_level_t level, const char *tag, const char *format, ...);

/**
 * Get uptime string (for debug info screen)
 */
const char *logger_get_uptime(void);

/**
 * Get log buffer (last N lines for debug screen)
 */
const char *logger_get_buffer(void);

// Convenience macros
#define LOG_DEBUG(tag, fmt, ...)                                               \
  logger_log(LOG_LEVEL_DEBUG, tag, fmt, ##__VA_ARGS__)
#define LOG_INFO(tag, fmt, ...)                                                \
  logger_log(LOG_LEVEL_INFO, tag, fmt, ##__VA_ARGS__)
#define LOG_WARN(tag, fmt, ...)                                                \
  logger_log(LOG_LEVEL_WARN, tag, fmt, ##__VA_ARGS__)
#define LOG_ERROR(tag, fmt, ...)                                               \
  logger_log(LOG_LEVEL_ERROR, tag, fmt, ##__VA_ARGS__)

#endif // LOGGER_H
