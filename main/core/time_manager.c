/**
 * =============================================================================
 * CalX ESP32 Firmware - Time Manager
 * =============================================================================
 * NTP time synchronization and time formatting.
 * =============================================================================
 */

#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "logger.h"
#include "time_manager.h"

static const char *TAG = "TIME_MGR";

// =============================================================================
// Configuration
// =============================================================================
#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.google.com"
#define TIMEZONE "IST-5:30" // Indian Standard Time

// =============================================================================
// State
// =============================================================================
static bool time_synced = false;

// =============================================================================
// Callbacks
// =============================================================================

static void time_sync_notification_cb(struct timeval *tv) {
  time_synced = true;
  LOG_INFO(TAG, "Time synchronized via NTP");
}

// =============================================================================
// Initialization
// =============================================================================

void time_manager_init(void) {
  // Set timezone
  setenv("TZ", TIMEZONE, 1);
  tzset();

  // Configure SNTP
  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, NTP_SERVER_1);
  esp_sntp_setservername(1, NTP_SERVER_2);
  sntp_set_time_sync_notification_cb(time_sync_notification_cb);

  // Start SNTP - will sync when WiFi connects
  esp_sntp_init();

  LOG_INFO(TAG, "Time manager initialized");
}

// =============================================================================
// Time Queries
// =============================================================================

bool time_manager_is_synced(void) { return time_synced; }

time_t time_manager_get_timestamp(void) {
  if (!time_synced) {
    return 0;
  }

  time_t now;
  time(&now);
  return now;
}

const char *time_manager_get_time_str(void) {
  static char time_str[8];

  if (!time_synced) {
    strcpy(time_str, "--:--");
    return time_str;
  }

  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  snprintf(time_str, sizeof(time_str), "%02d:%02d", timeinfo.tm_hour,
           timeinfo.tm_min);

  return time_str;
}

const char *time_manager_get_date_str(void) {
  static char date_str[32];

  if (!time_synced) {
    strcpy(date_str, "--/--/--");
    return date_str;
  }

  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  snprintf(date_str, sizeof(date_str), "%02d/%02d/%02d", timeinfo.tm_mday,
           timeinfo.tm_mon + 1, timeinfo.tm_year % 100);

  return date_str;
}

void time_manager_sync(void) {
  // Force resync
  esp_sntp_restart();
}
