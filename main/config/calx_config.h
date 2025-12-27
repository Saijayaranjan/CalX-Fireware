/**
 * =============================================================================
 * CalX ESP32 Firmware - Central Configuration
 * =============================================================================
 * All constants, limits, and hardware definitions in one place.
 * Matches backend constraints from CalX-BD.
 * =============================================================================
 */

#ifndef CALX_CONFIG_H
#define CALX_CONFIG_H

// =============================================================================
// Firmware Version
// =============================================================================
#define CALX_FW_VERSION "1.0.0"
#define CALX_FW_VERSION_MAJOR 1
#define CALX_FW_VERSION_MINOR 0
#define CALX_FW_VERSION_PATCH 0

// =============================================================================
// Backend API Configuration
// =============================================================================
#define CALX_API_BASE_URL "https://calx-backend.vercel.app"
#define CALX_API_TIMEOUT_MS 15000
#define CALX_API_RETRY_COUNT 3
#define CALX_API_RETRY_DELAY_MS 1000

// API Endpoints (relative to base URL)
#define API_BIND_REQUEST "/device/bind/request"
#define API_BIND_STATUS "/device/bind/status"
#define API_HEARTBEAT "/device/heartbeat"
#define API_SETTINGS "/device/settings"
#define API_CHAT "/device/chat"
#define API_CHAT_SEND "/device/chat/send"
#define API_FILE "/device/file"
#define API_AI_QUERY "/device/ai/query"
#define API_AI_CONTINUE "/device/ai/continue"
#define API_UPDATE_CHECK "/device/update/check"
#define API_UPDATE_DOWNLOAD "/device/update/download"
#define API_UPDATE_REPORT "/device/update/report"

// =============================================================================
// Character Limits (Must Match Backend)
// =============================================================================
#define CHAT_MAX_CHARS 2500
#define CHAT_HARD_LIMIT 4000
#define AI_INPUT_MAX_CHARS 2500
#define AI_OUTPUT_CHUNK_SIZE 2500
#define FILE_MAX_CHARS 4000

// =============================================================================
// Display Configuration (SSD1306 OLED)
// =============================================================================
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 32
#define DISPLAY_I2C_ADDR 0x3C
#define DISPLAY_I2C_SDA_PIN 21
#define DISPLAY_I2C_SCL_PIN 22
#define DISPLAY_I2C_FREQ_HZ 400000

// Text rendering (characters per line / lines per screen)
#define TEXT_SMALL_CHARS_LINE 21 // 4 lines visible
#define TEXT_SMALL_LINES 4
#define TEXT_NORMAL_CHARS_LINE 16 // 3 lines visible
#define TEXT_NORMAL_LINES 3
#define TEXT_LARGE_CHARS_LINE 10 // 2 lines visible
#define TEXT_LARGE_LINES 2

// =============================================================================
// Keypad Configuration (Matrix)
// =============================================================================
#define KEYPAD_ROWS 6
#define KEYPAD_COLS 5

// Row GPIO pins (directly specify your wiring)
#define KEYPAD_ROW_PINS {4, 5, 18, 19, 23, 25}
// Column GPIO pins
#define KEYPAD_COL_PINS {26, 27, 32, 33, 14}

// Debounce timing
#define KEYPAD_DEBOUNCE_MS 50
#define KEYPAD_LONG_PRESS_MS 1000 // AC long press for idle
#define KEYPAD_SCAN_INTERVAL_MS 20

// =============================================================================
// Battery Configuration
// =============================================================================
#define BATTERY_ADC_CHANNEL ADC_CHANNEL_6 // GPIO34
#define BATTERY_ADC_ATTEN ADC_ATTEN_DB_12
#define BATTERY_DIVIDER_RATIO 2.0 // Voltage divider: 2x

// Voltage thresholds (in mV)
#define BATTERY_FULL_MV 4200
#define BATTERY_EMPTY_MV 3300
#define BATTERY_CRITICAL_MV 3300   // Low battery cutoff
#define BATTERY_OTA_MIN_PERCENT 30 // Minimum for OTA updates

// Measurement
#define BATTERY_SAMPLE_COUNT 10 // Moving average samples
#define BATTERY_UPDATE_MS 10000 // 10 second update interval

// =============================================================================
// Power Management
// =============================================================================
#define SCREEN_TIMEOUT_DEFAULT_S 30
#define SCREEN_TIMEOUT_MIN_S 10
#define SCREEN_TIMEOUT_MAX_S 300

// Power modes
typedef enum { POWER_MODE_NORMAL = 0, POWER_MODE_LOW = 1 } calx_power_mode_t;

// =============================================================================
// WiFi Configuration
// =============================================================================
#define WIFI_AP_SSID "CalX-Setup"
#define WIFI_AP_PASS "" // Open network for setup
#define WIFI_AP_CHANNEL 1
#define WIFI_AP_MAX_CONN 4

#define WIFI_STA_RETRY_MAX 5
#define WIFI_STA_RETRY_DELAY_MS 2000
#define WIFI_SCAN_MAX_NETWORKS 20

// =============================================================================
// Heartbeat Configuration
// =============================================================================
#define HEARTBEAT_NORMAL_INTERVAL_MS 60000    // 60 seconds
#define HEARTBEAT_LOWPOWER_INTERVAL_MS 600000 // 10 minutes

// =============================================================================
// OTA Configuration
// =============================================================================
#define OTA_RECV_TIMEOUT_MS 10000
#define OTA_BUF_SIZE 1024

// =============================================================================
// NVS Keys
// =============================================================================
#define NVS_NAMESPACE "calx"
#define NVS_KEY_DEVICE_ID "device_id"
#define NVS_KEY_DEVICE_TOKEN "dev_token"
#define NVS_KEY_WIFI_SSID "wifi_ssid"
#define NVS_KEY_WIFI_PASS "wifi_pass"
#define NVS_KEY_POWER_MODE "power_mode"
#define NVS_KEY_TEXT_SIZE "text_size"
#define NVS_KEY_KEYBOARD "keyboard"
#define NVS_KEY_SCREEN_TIMEOUT "screen_to"
#define NVS_KEY_BOUND "is_bound"

// =============================================================================
// Text Size Enum
// =============================================================================
typedef enum {
  TEXT_SIZE_SMALL = 0,
  TEXT_SIZE_NORMAL = 1,
  TEXT_SIZE_LARGE = 2
} calx_text_size_t;

// =============================================================================
// Keyboard Type Enum
// =============================================================================
typedef enum { KEYBOARD_QWERTY = 0, KEYBOARD_T9 = 1 } calx_keyboard_t;

// =============================================================================
// System States
// =============================================================================
typedef enum {
  STATE_BOOT = 0,
  STATE_NOT_BOUND,
  STATE_WIFI_SETUP,
  STATE_IDLE,
  STATE_MENU,
  STATE_CHAT,
  STATE_FILE,
  STATE_AI,
  STATE_SETTINGS,
  STATE_BUSY,
  STATE_LOW_BATTERY,
  STATE_ERROR,
  STATE_OTA_UPDATE
} calx_state_t;

// =============================================================================
// Event Types
// =============================================================================
typedef enum {
  EVENT_NONE = 0,
  EVENT_KEY_PRESS,
  EVENT_KEY_LONG_PRESS,
  EVENT_WIFI_CONNECTED,
  EVENT_WIFI_DISCONNECTED,
  EVENT_WIFI_SCAN_DONE,
  EVENT_BIND_SUCCESS,
  EVENT_BIND_FAILED,
  EVENT_NEW_CHAT_MESSAGE,
  EVENT_AI_RESPONSE_READY,
  EVENT_FILE_UPDATED,
  EVENT_LOW_BATTERY,
  EVENT_BATTERY_OK,
  EVENT_OTA_AVAILABLE,
  EVENT_OTA_COMPLETE,
  EVENT_OTA_FAILED,
  EVENT_TIMEOUT,
  EVENT_API_ERROR,
  EVENT_API_SUCCESS
} calx_event_type_t;

// =============================================================================
// Key Codes
// =============================================================================
typedef enum {
  KEY_NONE = 0,
  KEY_0,
  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,
  KEY_PLUS,
  KEY_MINUS,
  KEY_MULTIPLY,
  KEY_DIVIDE,
  KEY_EQUALS, // = key (next page)
  KEY_DEL,    // DEL key (previous page / backspace)
  KEY_AC,     // AC key (back / idle)
  KEY_DOT,    // Decimal point
  KEY_UP,
  KEY_DOWN,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_OK // Enter / Select
} calx_key_t;

// =============================================================================
// Menu Items
// =============================================================================
#define MENU_ITEM_CHAT 0
#define MENU_ITEM_FILE 1
#define MENU_ITEM_AI 2
#define MENU_ITEM_SETTINGS 3

// Settings menu items
#define SETTINGS_INTERNET 0
#define SETTINGS_AI_CONFIG 1
#define SETTINGS_ADVANCED 2
#define SETTINGS_UPDATE 3
#define SETTINGS_BIND 4
#define SETTINGS_KEYBOARD 5

// Advanced settings items
#define ADVANCED_FACTORY_RESET 0
#define ADVANCED_CLEAR_CACHE 1
#define ADVANCED_DEBUG_INFO 2
#define ADVANCED_POWER_MODE 3
#define ADVANCED_SCREEN_TIME 4
#define ADVANCED_TEXT_SIZE 5

#endif // CALX_CONFIG_H
