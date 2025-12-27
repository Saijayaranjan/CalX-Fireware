# CalX ESP32 Firmware

<div align="center">

![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.2.2-blue?logo=espressif)
![ESP32](https://img.shields.io/badge/ESP32-WROOM--32-green)
![License](https://img.shields.io/badge/License-MIT-yellow)

**A smart calculator companion device with cloud connectivity, AI integration, and OTA updates.**

[Features](#features) • [Hardware](#hardware-requirements) • [Setup](#setup) • [Build](#build--flash) • [Architecture](#project-structure)

![CalX Dashboard](https://raw.githubusercontent.com/Saijayaranjan/CalX-Frontend/main/public/images/Website.png)

</div>

---

## Features

- 🌐 **WiFi Connectivity** - STA/AP modes with captive portal for easy setup
- 📱 **Device Binding** - Secure pairing with CalX Dashboard via bind codes
- 💬 **Real-time Chat** - Send/receive messages from the dashboard
- 📝 **File Storage** - Access synced notes and files on the device
- 🤖 **AI Queries** - On-device AI with multi-provider support (ChatGPT, Gemini)
- 🔄 **OTA Updates** - Over-the-air firmware updates with rollback support
- 🔋 **Battery Management** - ADC-based monitoring with low power modes
- 📺 **OLED Display** - 128x32 SSD1306 with multiple text sizes

## Hardware Requirements

### Components

| Component | Specification |
|-----------|---------------|
| Microcontroller | ESP32-WROOM-32 DevKit |
| Display | SSD1306 OLED 128x32 (I2C) |
| Input | 6x5 Matrix Keypad |
| Power | 3.7V LiPo Battery + Voltage Divider |
| Storage | 4MB Flash (built-in) |

### GPIO Pinout

```
┌─────────────────────────────────────┐
│           ESP32 Pinout              │
├─────────────────────────────────────┤
│  I2C Display                        │
│    SDA ──────── GPIO 21             │
│    SCL ──────── GPIO 22             │
├─────────────────────────────────────┤
│  Battery ADC                        │
│    VBAT ─────── GPIO 34 (ADC1_CH6)  │
├─────────────────────────────────────┤
│  Keypad Matrix (6 Rows x 5 Cols)    │
│    Rows ─────── GPIO 4,5,18,19,23,25│
│    Cols ─────── GPIO 26,27,32,33,14 │
└─────────────────────────────────────┘
```

## Setup

### Prerequisites

- macOS, Linux, or Windows with WSL
- Python 3.8+
- Git

### Install ESP-IDF

```bash
# Clone ESP-IDF
mkdir -p ~/esp
cd ~/esp
git clone -b v5.2.2 --recursive https://github.com/espressif/esp-idf.git

# Install toolchain
cd esp-idf
./install.sh esp32

# Add to shell (add to ~/.zshrc or ~/.bashrc)
alias get_idf='. ~/esp/esp-idf/export.sh'
```

### Clone This Repository

```bash
git clone https://github.com/Saijayaranjan/CalX-Fireware.git
cd CalX-Fireware
```

## Build & Flash

### Build Firmware

```bash
# Activate ESP-IDF environment
get_idf  # or: . ~/esp/esp-idf/export.sh

# Set target chip
idf.py set-target esp32

# Build
idf.py build
```

### Flash to Device

```bash
# Find your serial port
ls /dev/cu.usb*   # macOS
ls /dev/ttyUSB*   # Linux

# Flash and monitor
idf.py -p /dev/cu.usbserial-XXXX flash monitor

# Exit monitor: Ctrl+]
```

### Build Output

After successful build:
```
build/
├── calx_firmware.bin          # Main firmware (~1 MB)
├── bootloader/bootloader.bin  # Bootloader (~27 KB)
├── partition_table/           # Flash partition layout
└── ota_data_initial.bin       # OTA tracking data
```

## Project Structure

```
CalX-Fireware/
├── main/
│   ├── app_main.c              # Entry point, task creation
│   ├── config/
│   │   └── calx_config.h       # All configuration constants
│   ├── core/
│   │   ├── system_state.c/h    # State machine
│   │   ├── event_manager.c/h   # Event queue system
│   │   ├── logger.c/h          # Logging with levels
│   │   └── time_manager.c/h    # NTP time sync
│   ├── drivers/
│   │   ├── display_driver.c/h  # SSD1306 OLED driver
│   │   ├── input_manager.c/h   # Keypad scanner
│   │   ├── battery_manager.c/h # ADC battery monitoring
│   │   └── power_manager.c/h   # Power modes
│   ├── storage/
│   │   ├── storage_manager.c/h # NVS persistence
│   │   └── security_manager.c/h# Device ID & tokens
│   ├── network/
│   │   ├── wifi_manager.c/h    # WiFi STA/AP modes
│   │   └── api_client.c/h      # HTTPS API client
│   ├── ui/
│   │   ├── ui_manager.c/h      # Screen rendering
│   │   └── text_renderer.c/h   # Text wrapping/pagination
│   ├── ota/
│   │   └── ota_manager.c/h     # OTA update handling
│   └── captive_portal/
│       └── portal_html.h       # WiFi setup webpage
├── CMakeLists.txt              # Project CMake config
├── partitions.csv              # Flash partition table
└── sdkconfig.defaults          # Default SDK configuration
```

## Configuration

All settings are in `main/config/calx_config.h`:

### Backend API
```c
#define CALX_API_BASE_URL    "https://calx-backend.vercel.app"
#define CALX_API_TIMEOUT_MS  15000
```

### Character Limits (matches backend)
```c
#define CHAT_MAX_CHARS       2500
#define AI_INPUT_MAX_CHARS   2500
#define FILE_MAX_CHARS       4000
```

### Hardware Pins
```c
#define DISPLAY_I2C_SDA_PIN  21
#define DISPLAY_I2C_SCL_PIN  22
#define BATTERY_ADC_CHANNEL  ADC_CHANNEL_6  // GPIO34
```

## Device States

```
┌───────────┐    ┌────────────┐    ┌──────────────┐
│   BOOT    │───▶│ NOT_BOUND  │───▶│ WIFI_SETUP   │
└───────────┘    └────────────┘    └──────────────┘
                                          │
                 ┌────────────────────────┘
                 ▼
            ┌─────────┐
            │  IDLE   │◀──────────────────┐
            └────┬────┘                   │
                 │                        │
            ┌────▼────┐                   │
            │  MENU   │───┬───┬───┬───────┤
            └─────────┘   │   │   │       │
                          ▼   ▼   ▼       │
                       CHAT FILE AI  SETTINGS
```

## API Endpoints

The firmware communicates with these backend endpoints:

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/device/bind/request` | POST | Get bind code |
| `/device/bind/status` | GET | Check binding status |
| `/device/heartbeat` | POST | Send device status |
| `/device/chat` | GET | Fetch messages |
| `/device/chat/send` | POST | Send message |
| `/device/file` | GET | Get synced file |
| `/device/ai/query` | POST | Send AI prompt |
| `/device/update/check` | GET | Check for OTA |

## First Boot Flow

1. **Power On** → Boot screen displays "CalX v1.0.0"
2. **Not Bound** → Screen shows "Not Bound - Press any key"
3. **WiFi Setup** → AP mode starts: `CalX-Setup`
4. **Connect Phone** → Join the AP, captive portal opens
5. **Select Network** → Choose WiFi and enter password
6. **Binding** → 6-digit code appears on screen
7. **Dashboard** → Enter code in CalX Dashboard to bind
8. **Ready** → Device shows idle screen with time

## Troubleshooting

### Build Errors
```bash
# Clean and rebuild
idf.py fullclean
idf.py build
```

### Flash Issues
```bash
# Hold BOOT button while flashing
idf.py -p PORT flash

# Erase flash completely
idf.py -p PORT erase-flash
```

### Monitor Issues
```bash
# Change baud rate if garbled output
idf.py -p PORT -b 115200 monitor
```

## License

MIT License - see [LICENSE](LICENSE) for details.

---

<div align="center">

**Part of the CalX Ecosystem**

[CalX Backend](https://github.com/Saijayaranjan/calx-backend) • [CalX Frontend](https://github.com/Saijayaranjan/CalX-Frontend)

</div>
