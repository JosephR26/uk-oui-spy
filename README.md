# UK-OUI-SPY PRO v3.1.0

**Portable UK Surveillance Device Detector**

<a href="https://ibb.co/JWz5T5w6"><img src="https://i.ibb.co/hxKDNDJQ/20260211-022216.jpg" alt="UK-OUI-SPY PRO Device" border="0"></a>

UK-OUI-SPY PRO is an open-source, portable surveillance device detector built on the ESP32 platform. It identifies nearby CCTV cameras, ANPR systems, drones, body cameras, and other surveillance equipment by matching their Wi-Fi and Bluetooth MAC addresses against a curated database of UK-relevant manufacturers (OUIs).

Built for security professionals, privacy researchers, and anyone interested in understanding the surveillance landscape around them.

## Features

- **Dual-Mode Scanning** -- Simultaneous Wi-Fi and Bluetooth Low Energy (BLE) detection
- **Promiscuous Mode** -- Captures raw Wi-Fi management frames to find hidden/non-broadcasting devices
- **280+ OUI Database** -- Curated database of UK surveillance manufacturers, loadable from SD card
- **Threat Intelligence** -- Composite threat scoring (0-100) based on relevance, proximity, recurrence, and behaviour
- **Tiered Priority Display** -- Detections grouped by priority tier for instant threat assessment
- **Correlation Engine** -- Automatically identifies coordinated surveillance operations (e.g. drone + controller)
- **Behavioural Analysis** -- Classifies devices as FIXED or MOBILE based on RSSI variance
- **7-Screen Touchscreen UI** -- LIST, RADAR, GRAPH, HISTORY, MAP, CONFIG, INFO
- **Embedded Web Portal** -- Full dashboard accessible from any phone/laptop over local Wi-Fi hotspot
- **Secure Logging** -- CSV logging to SD card with optional AES-128 encryption
- **GPS Support** -- Optional GPS module for location tagging
- **Deep Sleep Mode** -- 24+ hour battery life on 1000mAh LiPo

## Hardware

This firmware targets the **ESP32-2432S028** ("Cheap Yellow Display" / CYD), a low-cost all-in-one development board:

| Component | Specification |
|-----------|--------------|
| **MCU** | ESP32-WROOM-32 (dual-core, 240MHz) |
| **Display** | 2.8" ILI9341 TFT (240x320, SPI) |
| **Touch** | XPT2046 resistive touch controller (SPI) |
| **Storage** | MicroSD card slot (FAT32, up to 32GB) |
| **Connectivity** | Wi-Fi 802.11 b/g/n, Bluetooth 4.2 BLE |
| **Peripherals** | RGB LED, LDR (ambient light sensor) |

> **Note**: There are several CYD variants. This firmware is configured for the version with **XPT2046 resistive touch** (SPI, active-low CS on GPIO 33). Check your board's touch controller IC before flashing.

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (recommended) or Arduino IDE
- USB cable for your board (Micro-USB or USB-C depending on variant)
- MicroSD card (4-32GB, formatted FAT32)

### Build & Flash (PlatformIO)

```bash
git clone https://github.com/JosephR26/uk-oui-spy.git
cd uk-oui-spy
pio run --target upload
```

### Build & Flash (Arduino IDE)

1. Install [ESP32 board support](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)
2. Select board: **ESP32 Dev Module**
3. Install libraries via Library Manager:
   - `TFT_eSPI` by Bodmer (v2.5.43+)
   - `NimBLE-Arduino` by h2zero (v1.4.1+)
   - `ArduinoJson` by Benoit Blanchon (v6.21+)
4. Open `src/main.cpp` and upload

### First Boot

1. **Prepare SD Card** -- Format as FAT32, copy `oui.csv` and `priority.json` from the `examples/` folder to the root
2. **Power On** -- The Setup Wizard runs on first boot to verify hardware (touch, SD, battery)
3. **Start Scanning** -- Navigate using the bottom nav bar. Detections appear automatically
4. **Web Portal** -- Connect to Wi-Fi network `OUI-SPY-PRO` (password: `spypro2026`), then open `http://192.168.4.1`

## Detection Categories

| Category | Description | Colour |
|----------|-------------|--------|
| CCTV | Fixed surveillance cameras | Red |
| ANPR | Automatic number plate recognition | Red |
| Drone | Quadcopters and autonomous drones | Red |
| Body Cam | Police/security body cameras | Orange |
| Cloud CCTV | Cloud-connected camera systems | Orange |
| Facial Recog | Facial recognition systems | Purple |
| Traffic | Traffic monitoring infrastructure | Yellow |
| Parking | Council parking enforcement cameras | Yellow |
| Smart City | IoT/smart city infrastructure | Yellow |
| Dash Cam | Vehicle dash cameras | Green |
| Doorbell | Smart doorbell cameras | Green |

## Priority Tiers

| Tier | Level | Examples |
|------|-------|----------|
| 5 (Red) | CRITICAL | Government CCTV, drones, facial recognition |
| 4 (Orange) | HIGH | Surveillance infrastructure, body cameras |
| 3 (Yellow) | MODERATE | Vehicle CCTV, traffic systems |
| 2 (Green) | LOW | Consumer cameras, doorbells |
| 1 (Grey) | BASELINE | ISP routers, consumer devices (filtered) |

## Analysis Tools

Two offline tools are included for analysing detection logs from the SD card:

- **Python Script** -- `python analysis/analyze_detections.py detections.csv` -- statistical breakdown, category analysis, top manufacturers
- **Web Viewer** -- Open `visualization/detections_viewer.html` in a browser, load your CSV -- interactive charts, filtering, colour-coded cards. 100% local, no data uploaded.

## Documentation

| Document | Description |
|----------|-------------|
| [Quick Start Guide](QUICKSTART.md) | 5-minute setup guide |
| [User Manual](USER_MANUAL.md) | Complete feature guide for all 7 screens |
| [Hardware Setup](docs/HARDWARE_SETUP.md) | Assembly, wiring, enclosure options |
| [FAQ](FAQ.md) | Common questions, troubleshooting |
| [OUI Database Guide](docs/OUI_DATABASE_EXPANSION.md) | Database format, coverage, and how to add new manufacturers |
| [Changelog](CHANGELOG.md) | Version history |
| [Legal](LEGAL.md) | Licence, disclaimer, privacy notice |

## Contributing

Contributions are welcome. You can help by:

- **Reporting new OUIs** -- Found a surveillance manufacturer we're missing? Open an issue with the OUI, manufacturer name, and evidence of UK deployment
- **Field testing** -- Share detection reports (anonymised) to help validate the database
- **Code** -- Bug fixes, new features, performance improvements via pull request
- **Documentation** -- Tutorials, translations, use-case write-ups

## Legal

This device is for **educational and professional security auditing purposes only**. It operates by passively receiving publicly broadcast radio signals (MAC addresses and signal metadata only). It does not intercept, decrypt, or store the content of any communications. Passive reception of broadcast radio metadata is not prohibited under the Wireless Telegraphy Act 2006, provided no encrypted or private communications content is intercepted or disclosed (Section 48).

Users are solely responsible for compliance with all applicable laws including GDPR and the UK Data Protection Act 2018. See [LEGAL.md](LEGAL.md) for the full disclaimer and MIT licence.

## Licence

MIT Licence -- see [LEGAL.md](LEGAL.md) for details.
