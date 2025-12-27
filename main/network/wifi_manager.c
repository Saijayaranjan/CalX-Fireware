/**
 * =============================================================================
 * CalX ESP32 Firmware - WiFi Manager
 * =============================================================================
 * WiFi station/AP mode management with captive portal support.
 * =============================================================================
 */

#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include <string.h>

#include "calx_config.h"
#include "event_manager.h"
#include "input_manager.h"
#include "logger.h"
#include "portal_html.h"
#include "storage_manager.h"
#include "web_display.h"
#include "wifi_manager.h"

static const char *TAG = "WIFI";

// =============================================================================
// Event Bits
// =============================================================================
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

// =============================================================================
// State
// =============================================================================
static EventGroupHandle_t wifi_event_group;
static esp_netif_t *sta_netif = NULL;
static esp_netif_t *ap_netif = NULL;
static httpd_handle_t http_server = NULL;
static bool is_connected = false;
static bool is_ap_mode = false;
static int retry_count = 0;
static char current_ip[16] = "0.0.0.0";
static char current_ssid[33] = "";
static int8_t current_rssi = 0;

// Scan results
static wifi_network_t scan_results[WIFI_SCAN_MAX_NETWORKS];
static int scan_count = 0;

// Forward declarations
static void wifi_connect_internal(const char *ssid, const char *password);

// =============================================================================
// Event Handler
// =============================================================================

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT) {
    switch (event_id) {
    case WIFI_EVENT_STA_START:
      esp_wifi_connect();
      break;

    case WIFI_EVENT_STA_DISCONNECTED:
      is_connected = false;
      if (retry_count < WIFI_STA_RETRY_MAX) {
        esp_wifi_connect();
        retry_count++;
        LOG_WARN(TAG, "Retry %d/%d", retry_count, WIFI_STA_RETRY_MAX);
      } else {
        xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        event_manager_post_simple(EVENT_WIFI_DISCONNECTED);
      }
      break;

    case WIFI_EVENT_AP_STACONNECTED: {
      LOG_INFO(TAG, "Station connected to AP");
      break;
    }

    case WIFI_EVENT_AP_STADISCONNECTED: {
      LOG_INFO(TAG, "Station disconnected from AP");
      break;
    }

    case WIFI_EVENT_SCAN_DONE:
      event_manager_post_simple(EVENT_WIFI_SCAN_DONE);
      break;
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    snprintf(current_ip, sizeof(current_ip), IPSTR, IP2STR(&event->ip_info.ip));
    LOG_INFO(TAG, "Connected, IP: %s", current_ip);

    is_connected = true;
    retry_count = 0;
    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    event_manager_post_simple(EVENT_WIFI_CONNECTED);
  }
}

// =============================================================================
// Initialization
// =============================================================================

void wifi_manager_init(void) {
  wifi_event_group = xEventGroupCreate();

  // Create default network interfaces
  sta_netif = esp_netif_create_default_wifi_sta();
  ap_netif = esp_netif_create_default_wifi_ap();

  // Initialize WiFi
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // Register event handlers
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &wifi_event_handler, NULL));

  LOG_INFO(TAG, "WiFi manager initialized");
}

// =============================================================================
// Station Mode
// =============================================================================

void wifi_manager_connect(void) {
  char ssid[33] = {0};
  char pass[64] = {0};

  if (!storage_manager_get_wifi_ssid(ssid, sizeof(ssid))) {
    LOG_WARN(TAG, "No stored SSID");
    return;
  }
  storage_manager_get_wifi_pass(pass, sizeof(pass));

  wifi_connect_internal(ssid, pass);
}

static void wifi_connect_internal(const char *ssid, const char *password) {
  // Stop AP if running
  if (is_ap_mode) {
    wifi_manager_stop_ap();
  }

  wifi_config_t wifi_config = {0};
  strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
  if (password && strlen(password) > 0) {
    strncpy((char *)wifi_config.sta.password, password,
            sizeof(wifi_config.sta.password) - 1);
  }
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  strncpy(current_ssid, ssid, sizeof(current_ssid) - 1);

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  retry_count = 0;
  LOG_INFO(TAG, "Connecting to: %s", ssid);
}

void wifi_manager_disconnect(void) {
  esp_wifi_disconnect();
  is_connected = false;
}

bool wifi_manager_connect_to(const char *ssid, const char *password) {
  if (!ssid)
    return false;

  // Save credentials
  storage_manager_set_wifi_credentials(ssid, password ? password : "");

  // Connect
  wifi_connect_internal(ssid, password);

  return true;
}

bool wifi_manager_is_connected(void) { return is_connected; }

bool wifi_manager_has_credentials(void) {
  return storage_manager_has_wifi_credentials();
}

bool wifi_manager_wait_connected(TickType_t timeout_ticks) {
  EventBits_t bits =
      xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                          pdFALSE, pdFALSE, timeout_ticks);
  return (bits & WIFI_CONNECTED_BIT) != 0;
}

// =============================================================================
// AP Mode (Captive Portal)
// =============================================================================

// HTTP handlers
static esp_err_t root_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, PORTAL_HTML, strlen(PORTAL_HTML));
  return ESP_OK;
}

static esp_err_t scan_handler(httpd_req_t *req) {
  // Use static buffers to avoid heap fragmentation issues
  static wifi_ap_record_t ap_list[10];
  static char json[2048];

  // Perform scan
  wifi_scan_config_t scan_config = {
      .show_hidden = true,
      .scan_type = WIFI_SCAN_TYPE_ACTIVE,
  };
  esp_wifi_scan_start(&scan_config, true);

  uint16_t ap_count = 10; // Max 10 networks
  esp_wifi_scan_get_ap_records(&ap_count, ap_list);

  // Build JSON response
  int pos = sprintf(json, "[");
  for (int i = 0; i < ap_count && i < 10; i++) {
    if (i > 0)
      pos += sprintf(json + pos, ",");
    pos += sprintf(json + pos, "{\"ssid\":\"%s\",\"rssi\":%d,\"secure\":%s}",
                   ap_list[i].ssid, ap_list[i].rssi,
                   ap_list[i].authmode != WIFI_AUTH_OPEN ? "true" : "false");
  }
  sprintf(json + pos, "]");

  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, json, strlen(json));

  return ESP_OK;
}

static esp_err_t connect_handler(httpd_req_t *req) {
  char buf[256];
  int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
  if (ret <= 0) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data");
    return ESP_FAIL;
  }
  buf[ret] = '\0';

  // Parse JSON (simple parsing)
  char ssid[33] = {0};
  char pass[64] = {0};

  char *ssid_start = strstr(buf, "\"ssid\":\"");
  if (ssid_start) {
    ssid_start += 8;
    char *ssid_end = strchr(ssid_start, '"');
    if (ssid_end) {
      int len = ssid_end - ssid_start;
      if (len > 32)
        len = 32;
      strncpy(ssid, ssid_start, len);
    }
  }

  char *pass_start = strstr(buf, "\"password\":\"");
  if (pass_start) {
    pass_start += 12;
    char *pass_end = strchr(pass_start, '"');
    if (pass_end) {
      int len = pass_end - pass_start;
      if (len > 63)
        len = 63;
      strncpy(pass, pass_start, len);
    }
  }

  if (strlen(ssid) == 0) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID required");
    return ESP_FAIL;
  }

  LOG_INFO(TAG, "Portal connect request: %s", ssid);

  // Send response before connecting
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, "{\"status\":\"connecting\"}");

  // Delay to allow response, then connect
  vTaskDelay(pdMS_TO_TICKS(500));
  wifi_manager_connect_to(ssid, pass);

  return ESP_OK;
}

static esp_err_t keypress_handler(httpd_req_t *req) {
  char buf[128];
  int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
  if (ret <= 0) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data");
    return ESP_FAIL;
  }
  buf[ret] = '\0';

  // Parse key from JSON: {"key":7} or {"key":"KEY_7"}
  int key_code = -1;
  char *key_start = strstr(buf, "\"key\":");
  if (key_start) {
    key_start += 6;
    while (*key_start == ' ' || *key_start == '\"')
      key_start++;
    key_code = atoi(key_start);
  }

  if (key_code < 0 || key_code > KEY_OK) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid key");
    return ESP_FAIL;
  }

  // Inject the key press
  input_manager_inject_key((calx_key_t)key_code);

  // Send success response
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
  return ESP_OK;
}

static esp_err_t status_handler(httpd_req_t *req) {
  char json[256];
  snprintf(json, sizeof(json),
           "{\"wifi_connected\":%s,\"ssid\":\"%s\",\"ip\":\"%s\"}",
           is_connected ? "true" : "false", current_ssid, current_ip);

  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, json, strlen(json));
}

void wifi_manager_start_ap(void) {
  is_ap_mode = true;

  // Configure AP
  wifi_config_t wifi_config = {
      .ap =
          {
              .ssid = WIFI_AP_SSID,
              .ssid_len = strlen(WIFI_AP_SSID),
              .channel = WIFI_AP_CHANNEL,
              .password = WIFI_AP_PASS,
              .max_connection = WIFI_AP_MAX_CONN,
              .authmode = WIFI_AUTH_OPEN,
          },
  };

  // Use APSTA mode to allow scanning while AP is active
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  // Start HTTP server for captive portal
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.uri_match_fn = httpd_uri_match_wildcard;

  if (httpd_start(&http_server, &config) == ESP_OK) {
    // Root page
    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
    };
    httpd_register_uri_handler(http_server, &root);

    // Captive portal detection
    httpd_uri_t captive = {
        .uri = "/generate_204",
        .method = HTTP_GET,
        .handler = root_handler,
    };
    httpd_register_uri_handler(http_server, &captive);

    // Scan endpoint
    httpd_uri_t scan = {
        .uri = "/scan",
        .method = HTTP_GET,
        .handler = scan_handler,
    };
    httpd_register_uri_handler(http_server, &scan);

    // Connect endpoint
    httpd_uri_t connect = {
        .uri = "/connect",
        .method = HTTP_POST,
        .handler = connect_handler,
    };
    httpd_register_uri_handler(http_server, &connect);

    // Keypress endpoint (virtual keypad)
    httpd_uri_t keypress = {
        .uri = "/keypress",
        .method = HTTP_POST,
        .handler = keypress_handler,
    };
    httpd_register_uri_handler(http_server, &keypress);

    // Display endpoints
    httpd_uri_t display = {
        .uri = "/display",
        .method = HTTP_GET,
        .handler = web_display_handler,
    };
    httpd_register_uri_handler(http_server, &display);

    httpd_uri_t display_data = {
        .uri = "/display/data",
        .method = HTTP_GET,
        .handler = web_display_data_handler,
    };
    httpd_register_uri_handler(http_server, &display_data);

    // Status endpoint
    httpd_uri_t status = {
        .uri = "/status",
        .method = HTTP_GET,
        .handler = status_handler,
    };
    httpd_register_uri_handler(http_server, &status);
  }

  LOG_INFO(TAG, "AP started: %s", WIFI_AP_SSID);
  web_display_init();
}

void wifi_manager_stop_ap(void) {
  if (http_server) {
    httpd_stop(http_server);
    http_server = NULL;
  }

  esp_wifi_stop();
  is_ap_mode = false;

  LOG_INFO(TAG, "AP stopped");
}

// =============================================================================
// Network Scanning
// =============================================================================

int wifi_manager_scan(wifi_network_t *networks, int max_networks) {
  wifi_scan_config_t scan_config = {
      .show_hidden = true,
      .scan_type = WIFI_SCAN_TYPE_ACTIVE,
  };

  esp_wifi_scan_start(&scan_config, true);

  uint16_t ap_count = 0;
  esp_wifi_scan_get_ap_num(&ap_count);

  wifi_ap_record_t *ap_list = malloc(ap_count * sizeof(wifi_ap_record_t));
  if (!ap_list)
    return 0;

  esp_wifi_scan_get_ap_records(&ap_count, ap_list);

  int count = (ap_count < max_networks) ? ap_count : max_networks;
  for (int i = 0; i < count; i++) {
    strncpy(networks[i].ssid, (char *)ap_list[i].ssid, 32);
    networks[i].ssid[32] = '\0';
    networks[i].rssi = ap_list[i].rssi;
    networks[i].secure = (ap_list[i].authmode != WIFI_AUTH_OPEN);
  }

  free(ap_list);
  return count;
}

// =============================================================================
// Getters
// =============================================================================

const char *wifi_manager_get_ip(void) { return current_ip; }

const char *wifi_manager_get_ssid(void) { return current_ssid; }

int8_t wifi_manager_get_rssi(void) {
  if (is_connected) {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
      current_rssi = ap_info.rssi;
    }
  }
  return current_rssi;
}

void wifi_manager_start_webserver(void) {
  if (http_server != NULL) {
    LOG_INFO(TAG, "HTTP server already running");
    return;
  }

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.uri_match_fn = httpd_uri_match_wildcard;

  if (httpd_start(&http_server, &config) == ESP_OK) {
    // Keypress endpoint for web display
    httpd_uri_t keypress = {
        .uri = "/keypress",
        .method = HTTP_POST,
        .handler = keypress_handler,
    };
    httpd_register_uri_handler(http_server, &keypress);

    // Web display endpoint
    httpd_uri_t display = {
        .uri = "/display",
        .method = HTTP_GET,
        .handler = web_display_handler,
    };
    httpd_register_uri_handler(http_server, &display);

    // Web display data endpoint
    httpd_uri_t display_data = {
        .uri = "/display/data",
        .method = HTTP_GET,
        .handler = web_display_data_handler,
    };
    httpd_register_uri_handler(http_server, &display_data);

    LOG_INFO(TAG, "Web server started on port 80");
    web_display_init();
  } else {
    LOG_ERROR(TAG, "Failed to start web server");
  }
}
