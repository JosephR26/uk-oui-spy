# UK-OUI-SPY PRO - User Manual

**Version 7.1.0**

Thank you for purchasing the UK-OUI-SPY PRO, a professional-grade tool for detecting and analyzing surveillance devices. This manual will guide you through every feature of your new device.

## 1. Introduction

The UK-OUI-SPY PRO is designed for security professionals, privacy advocates, and concerned citizens. It identifies nearby surveillance devices by scanning for their Wi-Fi and Bluetooth MAC addresses and matching them against a curated database of manufacturers.

### 1.1. Box Contents

*   1 x UK-OUI-SPY PRO Device
*   1 x USB-C Charging Cable
*   1 x Quick Start Guide

### 1.2. Device Overview

*   **2.8" Capacitive Touchscreen**: The primary interface for all on-device operations.
*   **USB-C Port**: For charging the internal LiPo battery.
*   **MicroSD Card Slot**: For loading the OUI database and logging detections.
*   **Status LEDs**: Provides at-a-glance status for power, scanning, and alerts.

## 2. Getting Started

### 2.1. Charging the Device

Before first use, fully charge the device by connecting the included USB-C cable to the port on the bottom of the device and a standard USB power adapter. The red status LED will turn off when charging is complete.

### 2.2. Preparing the MicroSD Card

1.  Obtain a microSD card (up to 32GB) and format it as **FAT32**.
2.  Download the latest `oui.csv` database file from our official GitHub repository.
3.  Copy the `oui.csv` file to the root directory of the microSD card.
4.  Insert the card into the device's microSD card slot.

### 2.3. First-Time Setup Wizard

On its first boot, the device will launch a mandatory Setup Wizard. This wizard will:

1.  **Welcome you** to the device.
2.  **Perform a Hardware Check**: It verifies that the touchscreen, SD card, OUI database, and battery are all functioning correctly.
3.  **Confirm Readiness**: Once all checks pass, you can proceed to the main interface.

Follow the on-screen prompts by tapping "NEXT" and then "GO" to complete the setup. The device will not run the wizard again unless the configuration is reset.

## 3. On-Device Touchscreen UI

The primary interface is the 2.8" touchscreen. A persistent navigation bar at the bottom allows you to switch between 7 different screens.

### 3.1. The 7 Screens

| Icon | Name      | Description                                                                 |
| :--- | :-------- | :-------------------------------------------------------------------------- |
| LIST | Detections| The main screen. A scrollable, real-time list of all detected devices.      |
| RADAR| Radar     | A proximity radar visualizing the relative distance of detected devices.    |
| GRAPH| Signal    | A line graph showing the signal strength (RSSI) history of a selected device. |
| HIST | History   | A view of past detections loaded from the SD card log file.                 |
| MAP  | Map       | A grid-based proximity map showing device positions.                        |
| CFG  | Config    | The settings page for configuring all device options.                       |
| INFO | Status    | A system information page showing firmware, hardware, and memory status.    |

### 3.2. LIST - The Detection Screen

This is the default screen. It shows a live, color-coded list of all detected devices, sorted by the most recent.

*   **Filtering**: Tap the filter bar at the top (**ALL / HIGH / MED / LOW**) to show only devices of a certain relevance level.
*   **Scrolling**: Swipe up and down on the list to scroll through detections.
*   **Selection**: Tap on any device in the list to select it. The selected device will be highlighted and its data will be used in the **GRAPH** screen.

### 3.3. RADAR - Proximity Visualization

This screen provides a classic radar-style view of the surrounding environment. Each dot represents a detected device.

*   **Center**: The center of the radar represents your position.
*   **Distance**: Devices closer to the center have a stronger signal (are likely nearer).
*   **Color**: The color of the dot corresponds to its **Threat Score**.

### 3.4. GRAPH - Signal Strength History

This screen plots the RSSI (Received Signal Strength Indicator) of the device you selected on the **LIST** screen. This is useful for determining if a device is moving closer or further away over time.

### 3.5. HIST - Detection History

This screen loads and displays the last 50 entries from the `detections.csv` log file on the SD card. It provides a quick way to review past activity without removing the SD card.

### 3.6. MAP - Grid Proximity Map

Similar to the radar, this screen shows detected devices on a grid. If a GPS module is connected and has a fix, your current coordinates will be displayed.

### 3.7. CFG - Configuration

This screen allows you to configure the device in real-time.

*   **Toggles**: Enable or disable key features like BLE/WiFi scanning, promiscuous mode, SD logging, encryption, auto-brightness, and the web portal.
*   **Brightness Slider**: If auto-brightness is disabled, a slider appears, allowing you to manually set the screen brightness.

All settings are saved automatically to the device's non-volatile storage.

### 3.8. INFO - System Status

This page provides a detailed overview of the device's status, including:

*   Firmware Version
*   Battery Voltage
*   OUI Database Entry Count
*   Free Memory
*   Hardware Status (Touch, SD Card, GPS)
*   Total Packets Captured
*   Recurring Device Count
*   System Uptime
*   Connected Web Clients

## 4. The Embedded Web Portal

For a richer experience, the UK-OUI-SPY PRO hosts its own Wi-Fi hotspot and web server, allowing you to interact with it from any phone, tablet, or laptop.

### 4.1. Connecting to the Web Portal

1.  On your phone or computer, open your Wi-Fi settings.
2.  Connect to the network named **"OUI-SPY-PRO"**.
3.  Enter the password: **`spypro2026`**
4.  A captive portal should automatically open your browser to the dashboard. If not, manually navigate to `http://192.168.4.1`.

### 4.2. Web Portal Features

The web portal mirrors and extends the functionality of the on-device UI.

*   **Dashboard**: A real-time overview of system status and the most recent detections.
*   **Detections Page**: A full, searchable, and filterable list of all detections.
*   **Radar Page**: A large, interactive canvas-based radar visualization.
*   **Settings Page**: Remotely configure all device settings.
*   **Logs Page**: View recent log entries and **download the full `detections.csv` file** directly to your computer.

## 5. Advanced Features

### 5.1. Threat Intelligence Engine

The device doesn't just detect; it analyzes. A composite **Threat Score (0-100)** is calculated for each device based on:

*   **Relevance**: The inherent risk of the device type (e.g., a police bodycam is higher than a consumer doorbell).
*   **Proximity**: How strong the signal is.
*   **Recurrence**: How many times the device has been seen.
*   **Dwell Time**: How long the device has been in the vicinity.
*   **Category**: Certain categories (e.g., Facial Recognition) receive a score boost.

### 5.2. Behavioral Analysis

The device analyzes RSSI fluctuations to classify devices as **FIXED** (likely stationary, like a CCTV camera) or **MOBILE** (likely moving, like a bodycam or drone).

### 5.3. Secure Logging (AES-128)

When enabled in the Settings, all logs written to the SD card are encrypted using AES-128. This protects your sensitive detection data if the device is lost or confiscated. A companion decryption tool will be available from our GitHub repository.

## 6. Maintenance

### 6.1. Updating the Firmware

Periodically check the official GitHub repository for new firmware releases. Follow the instructions in the `README.md` file to update your device to the latest version.

### 6.2. Updating the OUI Database

The `oui.csv` file is the heart of the detection engine. Download the latest version from the GitHub repository and replace the file on your SD card to ensure you can detect the newest devices.

## 7. Legal Disclaimer

This device is intended for educational and professional security auditing purposes only. The use of this device for any illegal or unauthorized activities is strictly prohibited. The developers are not responsible for any misuse of this product. Always ensure you are compliant with local laws and regulations regarding radio scanning and privacy in your jurisdiction.
