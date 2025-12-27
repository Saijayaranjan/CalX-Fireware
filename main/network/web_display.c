/**
 * =============================================================================
 * CalX ESP32 Firmware - Web Display Streaming
 * =============================================================================
 * Provides HTTP endpoints to stream display framebuffer to web browser.
 * =============================================================================
 */

#include "web_display.h"
#include "display_driver.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "WEB_DISPLAY";

// =============================================================================
// Initialization
// =============================================================================

void web_display_init(void) {
  ESP_LOGI(TAG, "Web display streaming initialized");
}

// =============================================================================
// HTTP Handlers
// =============================================================================

static const char WEB_DISPLAY_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>CalX Display & Keypad</title>
    <style>
        body { 
            margin: 0; 
            padding: 20px; 
            background: #0a0e14; 
            color: #fff; 
            font-family: -apple-system, BlinkMacSystemFont, sans-serif;
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 20px;
        }
        h1 { color: #5cefe5; margin: 0; }
        #display { 
            border: 2px solid #5cefe5; 
            background: #000;
            box-shadow: 0 0 20px rgba(92, 239, 229, 0.3);
            image-rendering: pixelated;
            image-rendering: crisp-edges;
        }
        .keypad {
            display: grid;
            grid-template-columns: repeat(5, 1fr);
            gap: 8px;
            max-width: 400px;
            width: 100%;
        }
        .key {
            aspect-ratio: 1;
            background: linear-gradient(135deg, #1a2332 0%, #0f1621 100%);
            border: 1px solid #2a3f5f;
            border-radius: 8px;
            font-size: 18px;
            font-weight: 600;
            color: #5cefe5;
            cursor: pointer;
            transition: all 0.1s;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .key:active {
            background: #5cefe5;
            color: #000;
            transform: scale(0.95);
        }
        .key:hover {
            border-color: #5cefe5;
            box-shadow: 0 0 10px rgba(92, 239, 229, 0.3);
        }
        #fps { color: #888; font-size: 12px; }
    </style>
</head>
<body>
    <h1>CalX Virtual Display & Keypad</h1>
    <canvas id="display" width="512" height="128"></canvas>
    <div id="fps">FPS: 0</div>
    
    <div class="keypad">
        <button class="key" onclick="press(17)">↑</button>
        <button class="key" onclick="press(18)">↓</button>
        <button class="key" onclick="press(19)">←</button>
        <button class="key" onclick="press(20)">→</button>
        <button class="key" onclick="press(21)">OK</button>
        
        <button class="key" onclick="press(1)">1</button>
        <button class="key" onclick="press(2)">2</button>
        <button class="key" onclick="press(3)">3</button>
        <button class="key" onclick="press(11)">+</button>
        <button class="key" onclick="press(12)">−</button>
        
        <button class="key" onclick="press(4)">4</button>
        <button class="key" onclick="press(5)">5</button>
        <button class="key" onclick="press(6)">6</button>
        <button class="key" onclick="press(13)">×</button>
        <button class="key" onclick="press(14)">÷</button>
        
        <button class="key" onclick="press(7)">7</button>
        <button class="key" onclick="press(8)">8</button>
        <button class="key" onclick="press(9)">9</button>
        <button class="key" onclick="press(15)">=</button>
        <button class="key" onclick="press(16)">DEL</button>
        
        <button class="key" onclick="press(10)">0</button>
        <button class="key" onclick="press(22)">.</button>
        <button class="key" onclick="press(23)">(</button>
        <button class="key" onclick="press(24)">)</button>
        <button class="key" onclick="press(25)">AC</button>
    </div>
    
    <script>
        const canvas = document.getElementById('display');
        const ctx = canvas.getContext('2d');
        let lastUpdate = Date.now();
        let frames = 0;
        
        // Note: Keypad input is disabled in view-only mode
        // function press(key) {
        //     fetch('/keypress', {
        //         method: 'POST',
        //         headers: {'Content-Type': 'application/json'},
        //         body: JSON.stringify({key: key})
        //     });
        // }
        
        function updateDisplay() {
            fetch('/display/data')
                .then(r => r.json())
                .then(data => {
                    ctx.fillStyle = '#000';
                    ctx.fillRect(0, 0, 512, 128);
                    
                    for (let y = 0; y < 32; y++) {
                        for (let x = 0; x < 128; x++) {
                            const byteIndex = Math.floor(y / 8) * 128 + x;
                            const bitIndex = y % 8;
                            if (data.buffer[byteIndex] & (1 << bitIndex)) {
                                ctx.fillStyle = '#5cefe5';
                                ctx.shadowColor = '#5cefe5';
                                ctx.shadowBlur = 2;
                                ctx.fillRect(x * 4, y * 4, 4, 4);
                                ctx.shadowBlur = 0;
                            }
                        }
                    }
                    
                    frames++;
                    const now = Date.now();
                    if (now - lastUpdate >= 1000) {
                        document.getElementById('fps').textContent = 'FPS: ' + frames;
                        frames = 0;
                        lastUpdate = now;
                    }
                });
        }
        
        setInterval(updateDisplay, 100);
        updateDisplay();
    </script>
</body>
</html>
)rawliteral";

esp_err_t web_display_handler(httpd_req_t *req) {
  // Serve a simple HTML page with canvas that auto-refreshes
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, WEB_DISPLAY_HTML, strlen(WEB_DISPLAY_HTML));
}

esp_err_t web_display_data_handler(httpd_req_t *req) {
  // Get display buffer
  const uint8_t *buffer = display_driver_get_buffer();
  const int buffer_size = DISPLAY_BUFFER_SIZE;

  // Build JSON response with buffer data
  // Format: {"buffer":[byte0,byte1,...]}
  char *json = malloc(buffer_size * 5 + 50); // Overhead for JSON structure
  if (!json) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory error");
    return ESP_FAIL;
  }

  int pos = sprintf(json, "{\"buffer\":[");
  for (int i = 0; i < buffer_size; i++) {
    if (i > 0)
      pos += sprintf(json + pos, ",");
    pos += sprintf(json + pos, "%d", buffer[i]);
  }
  sprintf(json + pos, "]}");

  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  esp_err_t ret = httpd_resp_send(req, json, strlen(json));

  free(json);
  return ret;
}
