# Changelog

All notable changes to UK-OUI-SPY will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [3.1.0] - 2026-02-13

### Added
- **XPT2046 resistive touch support** -- switched from CST820 (I2C) to XPT2046 (SPI) for ESP32-2432S028 compatibility
- **Tiered priority display** -- detections grouped into 5 tiers for instant threat assessment
- **Correlation detection engine** -- identifies coordinated surveillance ops (drone + controller, surveillance clusters)
- **Field-validated database entries** -- 7 new OUIs discovered during Cardiff field testing (smart city infrastructure)
- **Deep sleep power management** -- 24+ hour battery on 1000mAh LiPo with configurable sleep cycles
- **Threat scoring algorithm** -- composite score (0-100) with boosts for enforcement devices and close proximity
- **Swipe & gesture touch UI** -- vertical scroll, horizontal swipe for settings/filters, long-press actions
- **Police/enforcement filter** -- quick toggle for body cams, ANPR, drones, facial recognition only
- **Wi-Fi promiscuous mode** -- full packet capture (probe requests, beacons, management frames), channel hopping (1, 6, 11)
- **Behavioural analysis** -- classifies devices as FIXED or MOBILE based on RSSI variance
- **Embedded web portal** -- full dashboard over local Wi-Fi hotspot (OUI-SPY-PRO / spypro2026)
- **AES-128 encrypted logging** -- optional encryption for SD card logs
- **RGB LED colour coding** -- red = alert, blue = BLE scan, green = Wi-Fi scan
- **Auto-brightness** -- LDR-based display brightness control
- **9 configuration toggles** -- BLE, Wi-Fi, promiscuous, SD logging, deep sleep, police filter, encryption, auto-brightness, web portal

### Changed
- Touch controller support: XPT2046 (SPI, CS on GPIO 33) replaces CST820 (I2C)
- SPI frequency reduced to 20MHz for stability
- LED alerts simplified to red-only proximity alerts (removed buzzer dependency)
- Detection struct includes threatScore field, sorted by score after each scan
- Documentation cleaned up for public release -- removed internal dev files, consolidated guides

### Removed
- Buzzer support (device relies on screen and LED alerts)
- CST820/FT6236 capacitive touch support (XPT2046 only)
- Arduino IDE .ino wrapper file (use PlatformIO or build from src/)

## [1.0.2] - 2026-01-26

### Added
- **Facial recognition category** (CAT_FACIAL_RECOG) -- dedicated category with purple display colour
- **Parking enforcement category** (CAT_PARKING_ENFORCEMENT) -- council enforcement cameras
- **48 new OUIs** -- NEC NeoFace (South Wales Police), Cognitec, BriefCam, Clearview AI, Idemia, Thales, Facewatch, Conduent, NSL Services, APCOA, ParkingEye, Telent, Vicon, Oncam, and more
- Cardiff & Welsh council infrastructure OUIs

### Changed
- Total OUI count: 230+ to 280+ entries
- Updated category colour mapping for facial recognition (purple) and parking (yellow)

## [1.0.1] - 2026-01-24

### Added
- **150+ new surveillance device OUIs** (+187% increase from v1.0.0)
- UK police equipment: Axon, WatchGuard, Sepura, Zepcam
- London infrastructure: Kapsch (ULEZ/congestion charging), March Networks (TfL)
- Enterprise cloud: Cisco Meraki, Verkada, Eagle Eye Networks
- Consumer brands: TP-Link, Xiaomi, additional Ring/Nest/Eufy variants
- Transport: Siemens, 360 Vision Technology
- Corrected backlight pin for ESP32-2432S028

## [1.0.0] - 2026-01-11

### Added
- Initial release
- BLE scanning (NimBLE) and Wi-Fi AP scanning
- 80+ UK surveillance OUI database
- 8 device categories (CCTV, ANPR, Drone, Body Cam, Cloud CCTV, Traffic, Dash Cam, Doorbell)
- 2.8" ILI9341 touchscreen UI with colour-coded relevance
- MicroSD card CSV logging with deduplication
- RSSI-based proximity detection
- Three scan modes: Quick (2s), Normal (5s), Power-Save (15s)
- Python analysis script and web visualisation tool
- Comprehensive documentation and sample data

## Credits

### Libraries
- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) by Bodmer
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) by h2zero
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) by Benoit Blanchon
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32) by Espressif

### Data Sources
- IEEE OUI Database
- UK surveillance equipment research
- Community field testing and feedback

## Licence

MIT Licence. See [LEGAL.md](LEGAL.md) for full terms.
