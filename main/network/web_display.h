/**
 * =============================================================================
 * CalX ESP32 Firmware - Web Display Streaming
 * =============================================================================
 * Provides HTTP endpoints to stream display framebuffer to web browser.
 * =============================================================================
 */

#pragma once

#include "esp_http_server.h"

void web_display_init(void);
esp_err_t web_display_handler(httpd_req_t *req);
esp_err_t web_display_data_handler(httpd_req_t *req);
