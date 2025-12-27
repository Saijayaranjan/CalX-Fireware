/**
 * =============================================================================
 * CalX ESP32 Firmware - API Client Header
 * =============================================================================
 */

#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <stdbool.h>

// Chat message structure
typedef struct {
  char content[2501];
  char sender[8]; // "DEVICE" or "WEB"
  char timestamp[25];
} chat_message_t;

// AI response structure
typedef struct {
  char content[2501];
  bool has_more;
  char cursor[64];
} ai_response_t;

// File content structure
typedef struct {
  char content[4001];
  int char_count;
} file_content_t;

// Update info structure
typedef struct {
  bool available;
  char version[16];
  char download_url[256];
  char checksum[65];
  int file_size;
} update_info_t;

/**
 * Initialize API client
 */
void api_client_init(void);

// === Binding ===
/**
 * Request a bind code from server
 * @param code Output buffer for 4-char code
 * @param expires_in Output for expiration time in seconds
 * @return true if successful
 */
bool api_client_request_bind_code(char *code, int *expires_in);

/**
 * Poll bind status
 * @param token Output buffer for device token if bound
 * @return true if bound
 */
bool api_client_check_bind_status(char *token);

// === Heartbeat ===
/**
 * Send heartbeat to server
 * @return true if successful
 */
bool api_client_send_heartbeat(void);

// === Chat ===
/**
 * Fetch chat messages
 * @param messages Output array
 * @param max_messages Maximum to fetch
 * @param since Fetch messages after this timestamp (NULL for all)
 * @return Number of messages fetched
 */
int api_client_fetch_chat(chat_message_t *messages, int max_messages,
                          const char *since);

/**
 * Send chat message
 * @param content Message content
 * @return true if successful
 */
bool api_client_send_chat(const char *content);

// === File ===
/**
 * Fetch file content
 * @param file Output structure
 * @return true if successful
 */
bool api_client_fetch_file(file_content_t *file);

// === AI ===
/**
 * Send AI query
 * @param prompt Query prompt
 * @param response Output structure
 * @return true if successful
 */
bool api_client_ai_query(const char *prompt, ai_response_t *response);

/**
 * Continue AI response (get next chunk)
 * @param cursor Cursor from previous response
 * @param response Output structure
 * @return true if successful
 */
bool api_client_ai_continue(const char *cursor, ai_response_t *response);

// === Settings ===
/**
 * Fetch device settings from server
 * @return true if successful
 */
bool api_client_fetch_settings(void);

// === OTA ===
/**
 * Check for firmware updates
 * @param info Output structure
 * @return true if update available
 */
bool api_client_check_update(update_info_t *info);

/**
 * Report OTA update result
 * @param version Version that was updated to
 * @param success Whether update succeeded
 */
void api_client_report_update(const char *version, bool success);

#endif // API_CLIENT_H
