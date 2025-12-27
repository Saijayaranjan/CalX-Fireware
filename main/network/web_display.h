/**
 * =============================================================================
 * CalX ESP32 Firmware - Web Display Streaming
 * =============================================================================
 * Provides HTTP endpoints to stream display framebuffer to web browser.
 * =============================================================================
 */

#ifndef WEB_DISPLAY_H
#define WEB_DISPLAY_H

#include "esp_http_server.h"
#include <stdint.h>

/**
 * Initialize web display streaming module
 */
void web_display_init(void);

/**
 * HTTP handler for /display endpoint
 * Returns HTML canvas with live display
 */
esp_err_t web_display_handler(httpd_req_t *req);

/**
 * HTTP handler for /display/data endpoint
 * Returns raw framebuffer data as JSON
 */
esp_err_t web_display_data_handler(httpd_req_t *req);

#endif // WEB_DISPLAY_H
