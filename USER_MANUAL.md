# UK-OUI-SPY PRO - User Manual

**Version 3.1.0**

## 1. Introduction

UK-OUI-SPY PRO is an open-source surveillance device detector built on the ESP32 platform. It identifies nearby surveillance equipment by scanning for their Wi-Fi and Bluetooth MAC addresses and matching them against a curated database of UK-relevant manufacturers.

This manual covers all device features, screens, and configuration options.

### 1.1. Hardware Overview

- **2.8" Resistive Touchscreen** (XPT2046, SPI) -- the primary interface for all on-device operations
- **USB Port** -- for power and firmware flashing
- **MicroSD Card Slot** -- for loading the OUI database and logging detections
- **RGB LED** -- provides at-a-glance status (red = alert, blue = BLE scanning, green = Wi-Fi scanning)

## 2. Getting Started

### 2.1. Preparing the MicroSD Card

1. Format a microSD card (up to 32GB) as **FAT32**.
2. Download the latest `oui.csv` and `priority.json` files from the `examples/` folder in the repository.
3. Copy both files to the root directory of the microSD card.
4. Insert the card into the device's microSD card slot.

### 2.2. First-Time Setup Wizard

On its first boot, the device launches a Setup Wizard that:

1. **Welcomes you** to the device.
2. **Performs a Hardware Check** -- verifies that the touchscreen, SD card, OUI database, and battery are functioning correctly.
3. **Confirms Readiness** -- once all checks pass, tap NEXT then GO to proceed.

The wizard only runs once unless the configuration is reset.

## 3. Touchscreen UI

A persistent navigation bar at the bottom of the screen lets you switch between 4 screens.

### 3.1. Screen Reference

| Button | Screen | Description |
|--------|--------|-------------|
| LIST | Detections | Main screen. Real-time, scrollable list of detected devices |
| RADAR | Radar | Proximity visualisation -- dots represent detected devices |
| CONFIG | Config | Toggle settings: BLE, Wi-Fi, SD logging, auto-brightness, web portal |
| INFO | Status | Firmware version, battery, memory, OUI count, uptime |

### 3.2. LIST -- Detection Screen

The default screen. Shows a live, colour-coded list of all detected devices, sorted by priority tier.

- **Tiered Display** -- Devices grouped by priority (e.g. HIGH VALUE TARGET, SURVEILLANCE INFRA)
- **Filtering** -- Tap the filter bar (ALL / HIGH / MED / LOW) to show specific relevance levels
- **Scrolling** -- Swipe up/down to scroll through detections
- **Selection** -- Tap a device card to highlight it

### 3.3. RADAR -- Proximity Visualisation

A radar-style view of detected devices.

- **Centre** = your position
- **Distance from centre** = relative signal strength (closer = stronger signal)
- **Dot colour** = threat score

### 3.4. CONFIG -- Configuration

Real-time configuration toggles:

- BLE scanning on/off
- Wi-Fi scanning on/off
- SD card logging on/off
- Auto-brightness on/off
- Show baseline devices on/off
- Web portal on/off

All settings are saved to non-volatile storage automatically.

### 3.5. INFO -- System Status

Displays firmware version, battery voltage, OUI database count, free memory, hardware status, total packets captured, recurring device count, uptime, and connected web clients.

## 4. Embedded Web Portal

The device hosts its own Wi-Fi hotspot and web server for access from any phone, tablet, or laptop.

### 4.1. Connecting

1. On your phone/computer, connect to Wi-Fi network **OUI-SPY-PRO**
2. Password: **spypro2026**
3. A captive portal should open automatically. If not, navigate to **http://192.168.4.1**

### 4.2. Web Portal Features

- **Dashboard** -- real-time system status and recent detections
- **Detections** -- full, searchable, filterable detection list
- **Radar** -- large interactive radar visualisation
- **Settings** -- remotely configure all device settings
- **Logs** -- view log entries and download `detections.csv` directly

## 5. Advanced Features

### 5.1. Threat Intelligence Engine

A composite **Threat Score (0-100)** calculated for each device based on:

- **Relevance** -- inherent risk of the device type
- **Proximity** -- signal strength (RSSI)
- **Recurrence** -- how many times the device has been seen
- **Dwell Time** -- how long the device has been nearby
- **Category** -- certain categories (facial recognition, drones) receive a score boost
- **1.5x multiplier** for police/enforcement devices
- **1.3x multiplier** for close-range proximity (RSSI > -50)

### 5.2. Correlation Detection Engine

Automatically identifies coordinated surveillance operations. Triggers a flashing red banner.

| Rule | Description | Alert Level |
|------|-------------|-------------|
| SKYDIO OPS ACTIVE | Skydio controller + drone both detected | CRITICAL |
| DJI DRONE OPS | DJI drone platform detected | HIGH |
| SURVEILLANCE CLUSTER | 3+ government CCTV devices detected | HIGH |
| FACE RECOG ZONE | Genetec facial recognition infrastructure | CRITICAL |
| SMART CITY ZONE | 2+ smart city infrastructure devices | HIGH |

### 5.3. Behavioural Analysis

The device analyses RSSI fluctuations to classify devices as **FIXED** (stationary, like a CCTV camera) or **MOBILE** (moving, like a body camera or drone).

### 5.4. Session Logging

Each boot creates a new CSV log at `/sessions/<SESSION-ID>.csv` on the SD card. The session ID is randomly generated at boot, so each walk produces a uniquely named file. Pull the card after a session and open the CSV directly in any spreadsheet or analysis tool.

## 6. Maintenance

### 6.1. Updating the Firmware

Check the [GitHub repository](https://github.com/JosephR26/uk-oui-spy) for new releases. Pull the latest code and re-flash:

```bash
git pull
pio run --target upload
```

### 6.2. Updating the OUI Database

Download the latest `oui.csv` and `priority.json` files from the repository and replace the files on your SD card.

## 7. Legal Disclaimer

This device is for educational and professional security auditing purposes only. Use responsibly and in compliance with all local laws. See [LEGAL.md](LEGAL.md) for the full legal disclaimer and licence.
