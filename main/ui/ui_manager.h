/**
 * =============================================================================
 * CalX ESP32 Firmware - UI Manager Header
 * =============================================================================
 */

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "calx_config.h"

/**
 * Initialize UI manager
 */
void ui_manager_init(void);

/**
 * Update display (called from UI task)
 */
void ui_manager_update(void);

/**
 * Show boot screen
 */
void ui_manager_show_boot_screen(void);

/**
 * Called when system state changes
 */
void ui_manager_on_state_change(calx_state_t new_state);

/**
 * Set menu selection highlight
 */
void ui_manager_set_menu_selection(int selection);

/**
 * Handle key in chat screen
 */
void ui_manager_handle_chat_key(calx_key_t key);

/**
 * Handle key in file viewer
 */
void ui_manager_handle_file_key(calx_key_t key);

/**
 * Handle key in AI response view
 */
void ui_manager_handle_ai_key(calx_key_t key);

/**
 * Handle key in settings menu
 */
void ui_manager_handle_settings_key(calx_key_t key);

/**
 * Set notification dot (new chat message)
 */
void ui_manager_set_notification(bool has_notification);

/**
 * Set chat messages for display
 */
void ui_manager_set_chat_messages(const char **messages, int count);

/**
 * Set file content for display
 */
void ui_manager_set_file_content(const char *content);

/**
 * Set AI response for display
 */
void ui_manager_set_ai_response(const char *response, bool has_more);

/**
 * Show busy/fetching screen
 */
void ui_manager_show_busy(const char *message);

/**
 * Show error screen
 */
void ui_manager_show_error(const char *message);

/**
 * Show bind code screen
 */
void ui_manager_show_bind_code(const char *code);

/**
 * Show OTA progress
 */
void ui_manager_show_ota_progress(int percent);

#endif // UI_MANAGER_H
