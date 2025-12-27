/**
 * =============================================================================
 * CalX ESP32 Firmware - API Client
 * =============================================================================
 * HTTPS client for CalX backend communication.
 * =============================================================================
 */

#include "cJSON.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_tls.h"
#include <string.h>

#include "api_client.h"
#include "battery_manager.h"
#include "calx_config.h"
#include "logger.h"
#include "power_manager.h"
#include "security_manager.h"

static const char *TAG = "API";

// =============================================================================
// Response Buffer
// =============================================================================
#define MAX_RESPONSE_SIZE 8192
static char response_buffer[MAX_RESPONSE_SIZE];
static int response_len = 0;

// =============================================================================
// HTTP Event Handler
// =============================================================================

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
  switch (evt->event_id) {
  case HTTP_EVENT_ON_DATA:
    if (response_len + evt->data_len < MAX_RESPONSE_SIZE) {
      memcpy(response_buffer + response_len, evt->data, evt->data_len);
      response_len += evt->data_len;
      response_buffer[response_len] = '\0';
    }
    break;
  default:
    break;
  }
  return ESP_OK;
}

// =============================================================================
// HTTP Helpers
// =============================================================================

static esp_http_client_handle_t create_client(const char *endpoint) {
  char url[256];
  snprintf(url, sizeof(url), "%s%s", CALX_API_BASE_URL, endpoint);

  esp_http_client_config_t config = {
      .url = url,
      .event_handler = http_event_handler,
      .timeout_ms = CALX_API_TIMEOUT_MS,
      .crt_bundle_attach = esp_crt_bundle_attach,
  };

  return esp_http_client_init(&config);
}

static void add_auth_header(esp_http_client_handle_t client) {
  char token[128];
  if (security_manager_get_token(token, sizeof(token))) {
    char auth_header[150];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", token);
    esp_http_client_set_header(client, "Authorization", auth_header);
  }
}

static void add_json_headers(esp_http_client_handle_t client) {
  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_http_client_set_header(client, "Accept", "application/json");
}

// =============================================================================
// Initialization
// =============================================================================

void api_client_init(void) {
  LOG_INFO(TAG, "API client initialized, base URL: %s", CALX_API_BASE_URL);
}

// =============================================================================
// Binding
// =============================================================================

bool api_client_request_bind_code(char *code, int *expires_in) {
  esp_http_client_handle_t client = create_client(API_BIND_REQUEST);
  add_json_headers(client);
  esp_http_client_set_method(client, HTTP_METHOD_POST);

  // Build request body
  char device_id[32];
  security_manager_get_device_id(device_id, sizeof(device_id));

  char body[64];
  snprintf(body, sizeof(body), "{\"device_id\":\"%s\"}", device_id);
  esp_http_client_set_post_field(client, body, strlen(body));

  response_len = 0;
  esp_err_t err = esp_http_client_perform(client);
  int status = esp_http_client_get_status_code(client);

  bool success = false;
  if (err == ESP_OK && status == 200) {
    cJSON *json = cJSON_Parse(response_buffer);
    if (json) {
      cJSON *bind_code = cJSON_GetObjectItem(json, "bind_code");
      cJSON *expires = cJSON_GetObjectItem(json, "expires_in");

      if (cJSON_IsString(bind_code) && cJSON_IsNumber(expires)) {
        strncpy(code, bind_code->valuestring, 4);
        code[4] = '\0';
        *expires_in = expires->valueint;
        success = true;
        LOG_INFO(TAG, "Bind code received: %s", code);
      }
      cJSON_Delete(json);
    }
  } else {
    LOG_ERROR(TAG, "Bind request failed: %d", status);
  }

  esp_http_client_cleanup(client);
  return success;
}

bool api_client_check_bind_status(char *token) {
  char device_id[32];
  security_manager_get_device_id(device_id, sizeof(device_id));

  char endpoint[128];
  snprintf(endpoint, sizeof(endpoint), "%s?device_id=%s", API_BIND_STATUS,
           device_id);

  esp_http_client_handle_t client = create_client(endpoint);
  add_json_headers(client);

  response_len = 0;
  esp_err_t err = esp_http_client_perform(client);
  int status = esp_http_client_get_status_code(client);

  bool bound = false;
  if (err == ESP_OK && status == 200) {
    cJSON *json = cJSON_Parse(response_buffer);
    if (json) {
      cJSON *is_bound = cJSON_GetObjectItem(json, "bound");
      cJSON *dev_token = cJSON_GetObjectItem(json, "device_token");

      if (cJSON_IsTrue(is_bound)) {
        bound = true;
        if (cJSON_IsString(dev_token)) {
          strncpy(token, dev_token->valuestring, 127);
          token[127] = '\0';
        }
      }
      cJSON_Delete(json);
    }
  }

  esp_http_client_cleanup(client);
  return bound;
}

// =============================================================================
// Heartbeat
// =============================================================================

bool api_client_send_heartbeat(void) {
  esp_http_client_handle_t client = create_client(API_HEARTBEAT);
  add_auth_header(client);
  add_json_headers(client);
  esp_http_client_set_method(client, HTTP_METHOD_POST);

  // Build heartbeat body
  int battery = battery_manager_get_percent();
  calx_power_mode_t mode = power_manager_get_mode();

  char body[128];
  snprintf(body, sizeof(body),
           "{\"battery_percent\":%d,\"power_mode\":\"%s\",\"firmware_version\":"
           "\"%s\"}",
           battery, mode == POWER_MODE_NORMAL ? "NORMAL" : "LOW",
           CALX_FW_VERSION);

  esp_http_client_set_post_field(client, body, strlen(body));

  response_len = 0;
  esp_err_t err = esp_http_client_perform(client);
  int status = esp_http_client_get_status_code(client);

  bool success = (err == ESP_OK && status == 200);
  if (!success) {
    LOG_WARN(TAG, "Heartbeat failed: %d", status);
  }

  esp_http_client_cleanup(client);
  return success;
}

// =============================================================================
// Chat
// =============================================================================

int api_client_fetch_chat(chat_message_t *messages, int max_messages,
                          const char *since) {
  char endpoint[128];
  if (since) {
    snprintf(endpoint, sizeof(endpoint), "%s?since=%s", API_CHAT, since);
  } else {
    strncpy(endpoint, API_CHAT, sizeof(endpoint));
  }

  esp_http_client_handle_t client = create_client(endpoint);
  add_auth_header(client);
  add_json_headers(client);

  response_len = 0;
  esp_err_t err = esp_http_client_perform(client);
  int status = esp_http_client_get_status_code(client);

  int count = 0;
  if (err == ESP_OK && status == 200) {
    cJSON *json = cJSON_Parse(response_buffer);
    if (json) {
      cJSON *msgs = cJSON_GetObjectItem(json, "messages");
      if (cJSON_IsArray(msgs)) {
        cJSON *msg;
        cJSON_ArrayForEach(msg, msgs) {
          if (count >= max_messages)
            break;

          cJSON *content = cJSON_GetObjectItem(msg, "content");
          cJSON *sender = cJSON_GetObjectItem(msg, "sender");
          cJSON *timestamp = cJSON_GetObjectItem(msg, "created_at");

          if (cJSON_IsString(content)) {
            strncpy(messages[count].content, content->valuestring, 2500);
            messages[count].content[2500] = '\0';
          }
          if (cJSON_IsString(sender)) {
            strncpy(messages[count].sender, sender->valuestring, 7);
            messages[count].sender[7] = '\0';
          }
          if (cJSON_IsString(timestamp)) {
            strncpy(messages[count].timestamp, timestamp->valuestring, 24);
            messages[count].timestamp[24] = '\0';
          }
          count++;
        }
      }
      cJSON_Delete(json);
    }
  }

  esp_http_client_cleanup(client);
  return count;
}

bool api_client_send_chat(const char *content) {
  esp_http_client_handle_t client = create_client(API_CHAT_SEND);
  add_auth_header(client);
  add_json_headers(client);
  esp_http_client_set_method(client, HTTP_METHOD_POST);

  // Escape content for JSON
  cJSON *json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "content", content);
  char *body = cJSON_PrintUnformatted(json);

  esp_http_client_set_post_field(client, body, strlen(body));

  response_len = 0;
  esp_err_t err = esp_http_client_perform(client);
  int status = esp_http_client_get_status_code(client);

  bool success = (err == ESP_OK && status == 201);

  cJSON_Delete(json);
  free(body);
  esp_http_client_cleanup(client);
  return success;
}

// =============================================================================
// File
// =============================================================================

bool api_client_fetch_file(file_content_t *file) {
  esp_http_client_handle_t client = create_client(API_FILE);
  add_auth_header(client);
  add_json_headers(client);

  response_len = 0;
  esp_err_t err = esp_http_client_perform(client);
  int status = esp_http_client_get_status_code(client);

  bool success = false;
  if (err == ESP_OK && status == 200) {
    cJSON *json = cJSON_Parse(response_buffer);
    if (json) {
      cJSON *content = cJSON_GetObjectItem(json, "content");
      cJSON *char_count = cJSON_GetObjectItem(json, "char_count");

      if (cJSON_IsString(content)) {
        strncpy(file->content, content->valuestring, 4000);
        file->content[4000] = '\0';
        file->char_count = cJSON_IsNumber(char_count) ? char_count->valueint
                                                      : strlen(file->content);
        success = true;
      }
      cJSON_Delete(json);
    }
  }

  esp_http_client_cleanup(client);
  return success;
}

// =============================================================================
// AI
// =============================================================================

bool api_client_ai_query(const char *prompt, ai_response_t *response) {
  esp_http_client_handle_t client = create_client(API_AI_QUERY);
  add_auth_header(client);
  add_json_headers(client);
  esp_http_client_set_method(client, HTTP_METHOD_POST);

  cJSON *json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "prompt", prompt);
  char *body = cJSON_PrintUnformatted(json);

  esp_http_client_set_post_field(client, body, strlen(body));

  response_len = 0;
  esp_err_t err = esp_http_client_perform(client);
  int status = esp_http_client_get_status_code(client);

  bool success = false;
  if (err == ESP_OK && status == 200) {
    cJSON *resp_json = cJSON_Parse(response_buffer);
    if (resp_json) {
      cJSON *content = cJSON_GetObjectItem(resp_json, "content");
      cJSON *has_more = cJSON_GetObjectItem(resp_json, "has_more");
      cJSON *cursor = cJSON_GetObjectItem(resp_json, "cursor");

      if (cJSON_IsString(content)) {
        strncpy(response->content, content->valuestring, 2500);
        response->content[2500] = '\0';
        response->has_more = cJSON_IsTrue(has_more);
        if (cJSON_IsString(cursor)) {
          strncpy(response->cursor, cursor->valuestring, 63);
          response->cursor[63] = '\0';
        } else {
          response->cursor[0] = '\0';
        }
        success = true;
      }
      cJSON_Delete(resp_json);
    }
  } else {
    LOG_ERROR(TAG, "AI query failed: %d", status);
  }

  cJSON_Delete(json);
  free(body);
  esp_http_client_cleanup(client);
  return success;
}

bool api_client_ai_continue(const char *cursor, ai_response_t *response) {
  char endpoint[128];
  snprintf(endpoint, sizeof(endpoint), "%s?cursor=%s", API_AI_CONTINUE, cursor);

  esp_http_client_handle_t client = create_client(endpoint);
  add_auth_header(client);
  add_json_headers(client);

  response_len = 0;
  esp_err_t err = esp_http_client_perform(client);
  int status = esp_http_client_get_status_code(client);

  bool success = false;
  if (err == ESP_OK && status == 200) {
    cJSON *json = cJSON_Parse(response_buffer);
    if (json) {
      cJSON *content = cJSON_GetObjectItem(json, "content");
      cJSON *has_more = cJSON_GetObjectItem(json, "has_more");
      cJSON *next_cursor = cJSON_GetObjectItem(json, "cursor");

      if (cJSON_IsString(content)) {
        strncpy(response->content, content->valuestring, 2500);
        response->content[2500] = '\0';
        response->has_more = cJSON_IsTrue(has_more);
        if (cJSON_IsString(next_cursor)) {
          strncpy(response->cursor, next_cursor->valuestring, 63);
        } else {
          response->cursor[0] = '\0';
        }
        success = true;
      }
      cJSON_Delete(json);
    }
  }

  esp_http_client_cleanup(client);
  return success;
}

// =============================================================================
// Settings
// =============================================================================

bool api_client_fetch_settings(void) {
  esp_http_client_handle_t client = create_client(API_SETTINGS);
  add_auth_header(client);
  add_json_headers(client);

  response_len = 0;
  esp_err_t err = esp_http_client_perform(client);
  int status = esp_http_client_get_status_code(client);

  bool success = false;
  if (err == ESP_OK && status == 200) {
    cJSON *json = cJSON_Parse(response_buffer);
    if (json) {
      // Parse and apply settings
      // (Settings are server-controlled, device just acknowledges)
      LOG_INFO(TAG, "Settings fetched");
      success = true;
      cJSON_Delete(json);
    }
  }

  esp_http_client_cleanup(client);
  return success;
}

// =============================================================================
// OTA
// =============================================================================

bool api_client_check_update(update_info_t *info) {
  esp_http_client_handle_t client = create_client(API_UPDATE_CHECK);
  add_auth_header(client);
  add_json_headers(client);

  response_len = 0;
  esp_err_t err = esp_http_client_perform(client);
  int status = esp_http_client_get_status_code(client);

  info->available = false;

  if (err == ESP_OK && status == 200) {
    cJSON *json = cJSON_Parse(response_buffer);
    if (json) {
      cJSON *available = cJSON_GetObjectItem(json, "update_available");
      if (cJSON_IsTrue(available)) {
        info->available = true;

        cJSON *version = cJSON_GetObjectItem(json, "version");
        cJSON *url = cJSON_GetObjectItem(json, "download_url");
        cJSON *checksum = cJSON_GetObjectItem(json, "checksum");
        cJSON *size = cJSON_GetObjectItem(json, "file_size");

        if (cJSON_IsString(version)) {
          strncpy(info->version, version->valuestring, 15);
        }
        if (cJSON_IsString(url)) {
          strncpy(info->download_url, url->valuestring, 255);
        }
        if (cJSON_IsString(checksum)) {
          strncpy(info->checksum, checksum->valuestring, 64);
        }
        if (cJSON_IsNumber(size)) {
          info->file_size = size->valueint;
        }

        LOG_INFO(TAG, "Update available: %s", info->version);
      }
      cJSON_Delete(json);
    }
  }

  esp_http_client_cleanup(client);
  return info->available;
}

void api_client_report_update(const char *version, bool success) {
  esp_http_client_handle_t client = create_client(API_UPDATE_REPORT);
  add_auth_header(client);
  add_json_headers(client);
  esp_http_client_set_method(client, HTTP_METHOD_POST);

  char body[64];
  snprintf(body, sizeof(body), "{\"version\":\"%s\",\"success\":%s}", version,
           success ? "true" : "false");

  esp_http_client_set_post_field(client, body, strlen(body));

  response_len = 0;
  esp_http_client_perform(client);

  LOG_INFO(TAG, "Update result reported: %s = %s", version,
           success ? "success" : "failed");

  esp_http_client_cleanup(client);
}
