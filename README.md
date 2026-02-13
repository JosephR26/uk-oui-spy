<div align="center">

# UK-OUI-SPY PRO

### Know What's Watching You

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LEGAL.md)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-red.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Version: 3.1.0](https://img.shields.io/badge/Version-3.1.0-green.svg)](CHANGELOG.md)
[![OUI Database: 280+](https://img.shields.io/badge/OUI_Database-280%2B-orange.svg)](docs/OUI_DATABASE_EXPANSION.md)

<br>

<a href="https://ibb.co/JWz5T5w6"><img src="https://i.ibb.co/hxKDNDJQ/20260211-022216.jpg" alt="UK-OUI-SPY PRO Device" width="600"></a>

<br>

**A portable, open-source surveillance detector that identifies CCTV cameras, ANPR systems, police drones, body cameras, and facial recognition infrastructure by their wireless signatures.**

Built on a ~£15 ESP32 board. Fits in your pocket. Runs for 24+ hours on a coin-cell LiPo.

[Quick Start](QUICKSTART.md) · [User Manual](USER_MANUAL.md) · [Hardware Guide](docs/HARDWARE_SETUP.md) · [FAQ](FAQ.md)

</div>

---

## Why This Exists

The UK has one of the highest densities of surveillance cameras in the world. Most people walk past hundreds of cameras daily without knowing what's collecting their data, who operates it, or whether facial recognition is active.

UK-OUI-SPY PRO makes the invisible visible. Every Wi-Fi and Bluetooth device broadcasts a manufacturer ID (OUI) in its MAC address. This device listens for those broadcasts and cross-references them against a curated database of 280+ UK surveillance equipment manufacturers -- from Hikvision council CCTV to Axon police body cameras to NEC facial recognition systems.

No hacking. No interception. Just passive listening to publicly broadcast signals.

---

## What It Detects

| | Category | What It Finds |
|---|----------|--------------|
| **RED** | CCTV / ANPR / Drones | Government cameras, number plate readers, police drones |
| **ORANGE** | Body Cams / Cloud CCTV | Police body cameras, Verkada/Meraki enterprise systems |
| **PURPLE** | Facial Recognition | NEC NeoFace, Cognitec, BriefCam, Facewatch |
| **YELLOW** | Traffic / Parking / Smart City | ULEZ cameras, council CCTV, smart poles |
| **GREEN** | Consumer | Ring doorbells, Nest cams, dash cameras |

---

## Key Features

**Dual-Mode Scanning** -- Simultaneous Wi-Fi and Bluetooth Low Energy detection with promiscuous mode for hidden devices

**Threat Intelligence Engine** -- Every detection scored 0-100 based on device type, proximity, recurrence, and dwell time. Police/enforcement devices get a 1.5x multiplier. Close-range proximity gets 1.3x.

**Correlation Detection** -- Automatically flags coordinated surveillance. Skydio drone + controller? Flashing red `SKYDIO OPS ACTIVE` banner. 3+ government cameras in range? `SURVEILLANCE CLUSTER` alert.

**7-Screen Touchscreen UI** -- LIST, RADAR, GRAPH, HISTORY, MAP, CONFIG, INFO. Swipe navigation, colour-coded priority tiers, real-time filtering.

<!-- Add UI screenshot here: device showing LIST screen with detections -->

**Embedded Web Portal** -- Connect your phone to the device's Wi-Fi hotspot and get a full dashboard at `192.168.4.1`. Real-time detections, radar view, settings, log downloads.

<!-- Add web portal screenshot here -->

**Secure Logging** -- All detections logged to SD card as CSV. Optional AES-128 encryption. Analyse later with the included Python script or browser-based viewer.

**24+ Hour Battery** -- Deep sleep mode on a 1000mAh LiPo. USB-powered for unlimited runtime.

---

## Hardware

Total cost: **~£20**

This firmware runs on the **ESP32-2432S028** ("Cheap Yellow Display"), available on AliExpress, eBay, and Amazon UK:

| Component | Specification |
|-----------|--------------|
| MCU | ESP32-WROOM-32 (dual-core, 240MHz, Wi-Fi + BLE) |
| Display | 2.8" ILI9341 TFT (240x320, SPI) |
| Touch | XPT2046 resistive (SPI, CS on GPIO 33) |
| Storage | MicroSD slot (FAT32, up to 32GB) |
| Extras | RGB LED, LDR light sensor, TP4056 LiPo charging |

> **Important**: There are several CYD variants with different touch controllers. This firmware requires the **XPT2046 resistive touch** version. If your board has CST820 or FT6236 (capacitive, I2C), it will not work without modification. Check the chip markings before buying.

---

## Quick Start

### 1. Build & Flash

Install [VS Code](https://code.visualstudio.com/) with the [PlatformIO extension](https://platformio.org/install/ide?install=vscode), then:

```bash
git clone https://github.com/JosephR26/uk-oui-spy.git
cd uk-oui-spy
pio run --target upload
```

Or open the project folder in VS Code and click the PlatformIO Upload button.

### 2. Prepare SD Card

Format a microSD card as FAT32. Copy `examples/oui.csv` and `examples/priority.json` to the root. Insert into the board.

### 3. Boot & Scan

Power on. The Setup Wizard verifies your hardware on first boot. Tap through it, and the device starts scanning automatically. Detections appear as colour-coded cards sorted by threat level.

### 4. Web Portal (Optional)

Connect your phone to Wi-Fi network **`OUI-SPY-PRO`** (password: `spypro2026`). Open **http://192.168.4.1** for the full dashboard.

---

## Threat Scoring

Each detection receives a composite threat score (0-100):

```
Score = (Relevance + Proximity + Recurrence + Dwell Time) x Category Multiplier
```

| Factor | How It Works |
|--------|-------------|
| Relevance | Inherent risk of the device type (police CCTV > doorbell) |
| Proximity | Signal strength -- closer devices score higher |
| Recurrence | Devices seen repeatedly score higher |
| Dwell Time | Longer presence = higher score |
| Category Boost | Facial recognition, drones, ANPR get extra weight |
| Enforcement x1.5 | Police/government devices multiplied |
| Close Range x1.3 | RSSI > -50 dBm (within ~2m) multiplied |

---

## Correlation Alerts

The device automatically identifies coordinated surveillance operations:

| Alert | Trigger | Level |
|-------|---------|-------|
| `SKYDIO OPS ACTIVE` | Skydio controller + drone both detected | CRITICAL |
| `FACE RECOG ZONE` | Facial recognition infrastructure detected | CRITICAL |
| `DJI DRONE OPS` | DJI drone platform in range | HIGH |
| `SURVEILLANCE CLUSTER` | 3+ government CCTV devices | HIGH |
| `SMART CITY ZONE` | 2+ smart city infrastructure devices | HIGH |

---

## Analysis Tools

Two offline tools for analysing SD card logs after a session:

**Python Script** -- Terminal-based statistical breakdown
```bash
python analysis/analyze_detections.py detections.csv
```

**Web Viewer** -- Interactive browser dashboard with charts, filtering, and colour-coded cards. 100% local, no data uploaded.
```
Open visualization/detections_viewer.html in any browser
```

<!-- Add analysis tool screenshot here -->

---

## Documentation

| Document | Description |
|----------|-------------|
| **[Quick Start Guide](QUICKSTART.md)** | 5-minute setup, build, and first scan |
| **[User Manual](USER_MANUAL.md)** | Complete guide to all 7 screens and features |
| **[Hardware Setup](docs/HARDWARE_SETUP.md)** | Pin assignments, battery, enclosure options |
| **[FAQ](FAQ.md)** | Common questions and troubleshooting |
| **[OUI Database Guide](docs/OUI_DATABASE_EXPANSION.md)** | Database format, coverage, adding new manufacturers |
| **[Changelog](CHANGELOG.md)** | Version history |
| **[Legal](LEGAL.md)** | Licence, disclaimer, privacy notice |

---

## Contributing

Contributions welcome:

- **Report new OUIs** -- Found a surveillance manufacturer we're missing? Open an issue with the OUI, manufacturer name, and evidence of UK deployment
- **Field test** -- Share anonymised detection reports to help validate the database
- **Code** -- Bug fixes, features, and performance improvements via pull request
- **Documentation** -- Tutorials, translations, use-case write-ups

---

## Legal

This device is for **educational and professional security auditing purposes only**. It operates by passively receiving publicly broadcast radio signal metadata (MAC addresses and signal strength). It does not intercept, decrypt, or store the content of any communications.

Passive reception of broadcast metadata is not prohibited under the Wireless Telegraphy Act 2006, provided no private communications content is intercepted or disclosed (Section 48). Users are solely responsible for compliance with all applicable laws including GDPR and the UK Data Protection Act 2018.

See [LEGAL.md](LEGAL.md) for the full disclaimer, privacy notice, and MIT licence.

---

<div align="center">

**MIT Licence** · Built in Wales · [Report an Issue](https://github.com/JosephR26/uk-oui-spy/issues)

</div>
