# CalX ESP32 Firmware

<div align="center">

[![Website](https://img.shields.io/badge/Website-calxio.vercel.app-blue?style=for-the-badge)](https://calxio.vercel.app/)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.2.2-blue?logo=espressif)](https://github.com/espressif/esp-idf)
[![ESP32](https://img.shields.io/badge/ESP32-WROOM--32-green)](https://www.espressif.com/en/products/socs/esp32)
[![License](https://img.shields.io/badge/License-MIT-yellow)](LICENSE)

**A smart calculator companion device with cloud connectivity, AI integration, and OTA updates.**

🌐 **[CalX Website](https://calxio.vercel.app/)** • [Features](#features) • [Hardware](#hardware-bom) • [Setup](#setup) • [Build](#build--flash)

![CalX Dashboard](https://github.com/Saijayaranjan/CalX-Frontend/blob/main/public/images/Website.png?raw=true)

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

---

## Hardware BOM

Complete Bill of Materials to build your own CalX device.

### 1. Core Compute (Mandatory)

| Component | Specification |
|-----------|---------------|
| **Microcontroller** | ESP32-WROOM-32 Dev Board (38-pin) |
| | Dual-core 240 MHz, WiFi 2.4 GHz + Bluetooth |
| | USB-to-UART onboard, 4 MB flash |

### 2. Display (Mandatory)

| Component | Specification |
|-----------|---------------|
| **OLED Display** | 0.91" 128×32 SSD1306 (I2C) |
| | White or Blue, 3.3–5V |

### 3. Input System (Mandatory)

| Component | Specification |
|-----------|---------------|
| **Tactile Buttons** | 10× SMD or THT tactile switches |
| | Navigation (↑ ↓ ← →), OK, Back, Menu, Shift, Power |

### 4. Power System (Mandatory)

| Component | Specification |
|-----------|---------------|
| **Battery** | 3.7V Li-Po, 450–500 mAh (502030/WLY52535) |
| **Charger** | TP4056 Li-ion Charger (USB-C preferred) |
| **Regulator** | AMS1117-3.3V or ESP32 onboard |
| **Power Switch** | Slide switch (SPDT/SPST) |

### 5. Battery Safety (Mandatory)

| Component | Specification |
|-----------|---------------|
| **Voltage Divider** | 100kΩ + 100kΩ resistors |
| **Polyfuse** | 1206L050 (500 mA) |

### 6. Passive Components (Mandatory)

| Component | Specification |
|-----------|---------------|
| **Resistors** | 10kΩ (pull-ups), 100kΩ (battery sensing) |
| **Capacitors** | 0.1µF ceramic, 10µF electrolytic |

### 7. Wiring & Connectors

| Component | Specification |
|-----------|---------------|
| **Wires** | 24–28 AWG, PVC or silicone |
| **Connectors** | DuPont 2.54mm, JST-PH 2-pin (battery) |
| **Jumper Wires** | M-M, M-F, F-F for prototyping |

### 8. Optional Components

| Component | Purpose |
|-----------|---------|
| LEDs (3mm R/G/Y) | Status indicators |
| Micro-SD module | Extended storage |
| RTC module | Offline timekeeping |

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

---

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

```
build/
├── calx_firmware.bin          # Main firmware (~1 MB)
├── bootloader/bootloader.bin  # Bootloader (~27 KB)
├── partition_table/           # Flash partition layout
└── ota_data_initial.bin       # OTA tracking data
```

---

## Project Structure

```
CalX-Fireware/
├── main/
│   ├── app_main.c              # Entry point
│   ├── config/calx_config.h    # Configuration
│   ├── core/                   # State machine, events, logging
│   ├── drivers/                # OLED, keypad, battery, power
│   ├── storage/                # NVS, security
│   ├── network/                # WiFi, API client
│   ├── ui/                     # Display rendering
│   └── ota/                    # Firmware updates
├── CMakeLists.txt
├── partitions.csv
└── sdkconfig.defaults
```

## First Boot Flow

1. **Power On** → Boot screen displays "CalX v1.0.0"
2. **Not Bound** → Screen shows "Not Bound - Press any key"
3. **WiFi Setup** → AP mode starts: `CalX-Setup`
4. **Connect Phone** → Join the AP, captive portal opens
5. **Select Network** → Choose WiFi and enter password
6. **Binding** → 6-digit code appears on screen
7. **Dashboard** → Enter code at [calxio.vercel.app](https://calxio.vercel.app/)
8. **Ready** → Device shows idle screen with time

## License

MIT License - see [LICENSE](LICENSE) for details.

---

<div align="center">

**Part of the CalX Ecosystem**

🌐 [CalX Website](https://calxio.vercel.app/) • [Backend](https://github.com/Saijayaranjan/calx-backend) • [Frontend](https://github.com/Saijayaranjan/CalX-Frontend)

</div>
