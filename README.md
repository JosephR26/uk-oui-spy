# UK-OUI-SPY PRO - v3.1.0

**Professional UK Surveillance Device Detector**

![UK-OUI-SPY PRO](https://i.imgur.com/your-image-url.png)  <!-- Replace with a real image of the device -->

UK-OUI-SPY PRO is a professional-grade, portable device designed to detect and analyze potential surveillance devices in the United Kingdom. It leverages a comprehensive database of Organizationally Unique Identifiers (OUIs) known to be used by CCTV, ANPR, drones, body cameras, and other surveillance equipment. The device is built on the ESP32 platform and features a rich touchscreen interface, an embedded web portal, and advanced intelligence capabilities.

This project is intended for security professionals, privacy advocates, and individuals concerned about the proliferation of surveillance in public and private spaces.

## Key Features (v3.1.0)

*   **Tiered Priority Display**: Detections are now categorized into tiers (High Value, Surveillance Infra, Vehicle CCTV, etc.) for instant threat assessment.
*   **Correlation Detection Engine**: Automatically identifies and alerts on coordinated surveillance operations, such as a drone and its controller operating in the same area.
*   **Field-Validated Database**: The OUI database now includes 7 new entries discovered and validated during field tests in Cardiff, including smart city infrastructure.
*   **Dual-Mode Scanning**: Simultaneously scans for both **Wi-Fi** and **Bluetooth Low Energy (BLE)** devices.
*   **Promiscuous Mode**: Captures raw Wi-Fi management frames to detect hidden or non-broadcasting devices.
*   **OUI Database**: Utilizes a dynamic OUI database loaded from the SD card, with a robust static fallback.
*   **Threat Intelligence**: Assigns a composite threat score (0-100) to each detection based on relevance, proximity, recurrence, and behavior.
*   **Behavioral Analysis**: Classifies devices as **FIXED** or **MOBILE** based on signal strength variance.
*   **Touchscreen UI**: A 2.8" capacitive touchscreen with 7 distinct pages for live analysis and configuration.
*   **Embedded Web Portal**: A full-featured web dashboard accessible from any phone or laptop over a local Wi-Fi hotspot.
*   **Secure Logging**: Optional **AES-128 encryption** for all detection logs stored on the SD card.
*   **GPS Integration**: Optional GPS module support for location tagging of all detections.
*   **Power Management**: Deep sleep mode, battery monitoring, and auto-brightness for extended field use.

## Hardware

The firmware is specifically designed for the **ESP32-2432S028 (CYD)**, a low-cost development board that includes:

*   **MCU**: ESP32-WROOM-32
*   **Display**: 2.8" ILI9341 TFT (240x320)
*   **Touch**: Capacitive CST820
*   **Peripherals**: RGB LED, LDR, Buzzer, SD Card Slot

## Firmware Setup (Arduino IDE)

1.  **Install Arduino IDE**: Download and install the latest version from the [Arduino website](https://www.arduino.cc/en/software).
2.  **Install ESP32 Boards**: Follow the instructions [here](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html) to add ESP32 board support to your IDE.
3.  **Select Board**: In the Arduino IDE, go to `Tools > Board` and select **"ESP32 Dev Module"**.
4.  **Install Libraries**: Open the Library Manager (`Tools > Manage Libraries...`) and install the following:
    *   `TFT_eSPI` by Bodmer (v2.5.43+)
    *   `NimBLE-Arduino` by h2zero (v1.4.1+)
    *   `ArduinoJson` by Benoit Blanchon (v6.21+)
    *   `ESPAsyncWebServer` by me-no-dev (v1.2.3+)
    *   `AsyncTCP` by me-no-dev (v1.1.1+)
    *   `TinyGPSPlus` by Mikal Hart (v1.0.3+) *(optional, for GPS)*
5.  **Configure TFT_eSPI**: Locate the `User_Setup.h` file within the `TFT_eSPI` library folder and configure it for the ESP32-2432S028. The required settings are documented in the header of the `.ino` file.
6.  **Upload Firmware**: Open the `UK_OUI_SPY_PRO.ino` file in the Arduino IDE, connect your device, select the correct COM port, and click the "Upload" button.

## Getting Started

1.  **Prepare SD Card**: Format a microSD card as FAT32. Create a file named `oui.csv` and `priority.json` in the root directory. You can use the provided `examples/` files as a template.
2.  **First Boot**: On the first boot, the device will enter a **Setup Wizard** to check the hardware (Touch, SD Card, Battery) and guide you through initial setup.
3.  **Navigate**: Use the navigation bar at the bottom of the screen to switch between pages.
4.  **Web Portal**: Connect your phone or laptop to the **"OUI-SPY-PRO"** Wi-Fi network (password: `spypro2026`). Open a browser and navigate to `http://192.168.4.1` to access the web dashboard.

## Documentation

For a complete guide to all features, please refer to the full **[User Manual](USER_MANUAL.md)**.

## Disclaimer

This device is intended for educational and professional security auditing purposes only. The use of this device for any illegal or unauthorized activities is strictly prohibited. The developers are not responsible for any misuse of this product. Always ensure you are compliant with local laws and regulations regarding radio scanning and privacy.

## License

This project is licensed under the MIT License - see the [LEGAL.md](LEGAL.md) file for details.
