/**
 * =============================================================================
 * CalX ESP32 Firmware - WiFi Manager Header
 * =============================================================================
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "freertos/FreeRTOS.h"
#include <stdbool.h>

// WiFi network info for scan results
typedef struct {
  char ssid[33];
  int8_t rssi;
  bool secure;
} wifi_network_t;

/**
 * Initialize WiFi manager
 */
void wifi_manager_init(void);

/**
 * Start WiFi station mode and connect to saved network
 */
void wifi_manager_connect(void);

/**
 * Disconnect from current network
 */
void wifi_manager_disconnect(void);

/**
 * Start AP mode for captive portal
 */
void wifi_manager_start_ap(void);

/**
 * Stop AP mode
 */
void wifi_manager_stop_ap(void);

/**
 * Scan for available networks
 * @param networks Array to store results
 * @param max_networks Maximum networks to return
 * @return Number of networks found
 */
int wifi_manager_scan(wifi_network_t *networks, int max_networks);

/**
 * Connect to a specific network
 * @param ssid Network SSID
 * @param password Network password (NULL for open)
 * @return true if connection started
 */
bool wifi_manager_connect_to(const char *ssid, const char *password);

/**
 * Check if WiFi is connected
 */
bool wifi_manager_is_connected(void);

/**
 * Check if stored credentials exist
 */
bool wifi_manager_has_credentials(void);

/**
 * Wait for WiFi connection
 * @param timeout_ticks Maximum time to wait
 * @return true if connected
 */
bool wifi_manager_wait_connected(TickType_t timeout_ticks);

/**
 * Get current IP address as string
 */
const char *wifi_manager_get_ip(void);

/**
 * Get current SSID
 */
const char *wifi_manager_get_ssid(void);

/**
 * Get signal strength (RSSI)
 */
int8_t wifi_manager_get_rssi(void);

/**
 * Start HTTP server for web display (station mode)
 */
void wifi_manager_start_webserver(void);

#endif // WIFI_MANAGER_H
