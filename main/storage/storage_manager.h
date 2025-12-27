/**
 * =============================================================================
 * CalX ESP32 Firmware - Storage Manager Header
 * =============================================================================
 */

#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include "calx_config.h"
#include <stdbool.h>

/**
 * Initialize storage manager (NVS)
 */
void storage_manager_init(void);

// === WiFi Credentials ===
bool storage_manager_get_wifi_ssid(char *ssid, size_t max_len);
bool storage_manager_get_wifi_pass(char *pass, size_t max_len);
void storage_manager_set_wifi_credentials(const char *ssid, const char *pass);
bool storage_manager_has_wifi_credentials(void);
void storage_manager_clear_wifi_credentials(void);

// === Device Settings ===
calx_power_mode_t storage_manager_get_power_mode(void);
void storage_manager_set_power_mode(calx_power_mode_t mode);

calx_text_size_t storage_manager_get_text_size(void);
void storage_manager_set_text_size(calx_text_size_t size);

calx_keyboard_t storage_manager_get_keyboard(void);
void storage_manager_set_keyboard(calx_keyboard_t keyboard);

int storage_manager_get_screen_timeout(void);
void storage_manager_set_screen_timeout(int seconds);

// === Factory Reset ===
void storage_manager_factory_reset(void);

// === Cache ===
void storage_manager_clear_cache(void);

#endif // STORAGE_MANAGER_H
