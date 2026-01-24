# Changelog

All notable changes to UK-OUI-SPY will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned
- Enhanced touch UI with smooth scrolling
- Full settings screen implementation
- GPS integration for location tagging
- WiFi promiscuous packet capture integration
- Pattern recognition for recurring devices
- Multi-device aggregation network
- Offline map display
- Power management optimization

## [1.0.0] - 2025-01-11

### Added - Initial Release

#### Core Functionality
- BLE scanning engine using NimBLE library
- WiFi network scanning for AP detection
- Real-time OUI matching against UK surveillance database
- RSSI-based proximity detection
- Three scan modes: Quick (2s), Normal (5s), Power-Save (15s)
- Circular buffer system (50 detections max)
- 30-second detection deduplication window

#### OUI Database (80+ entries)
- **High-Risk Manufacturers** (40 OUIs):
  - Hikvision (7 OUIs) - UK Police/Council CCTV
  - Axis Communications (3 OUIs) - Body cameras, surveillance
  - DJI (2 OUIs) - Police drones
  - Motorola Solutions (3 OUIs) - ANPR, body cams
  - Avigilon (3 OUIs) - HD surveillance, ANPR
  - Bosch Security (2 OUIs) - ANPR, traffic systems
  - Digital Barriers, Edesix, Reveal Media - UK body cameras
  - FLIR Systems - Thermal imaging
  - Genetec - ANPR platforms
  - Milestone Systems - Police VMS

- **Medium-Risk Manufacturers** (25 OUIs):
  - Hanwha/Samsung - Retail CCTV
  - Dahua Technology - Council/transport
  - Vivotek, Pelco, Sony, Panasonic - Professional surveillance
  - IndigoVision - UK IP CCTV
  - Honeywell, Canon - Security systems
  - Verkada - Cloud-managed CCTV
  - Verisure, ADT - UK security providers

- **Consumer Devices** (15 OUIs):
  - Ring, Nest, Blink - Doorbell/cloud cameras
  - Arlo, Wyze, Eufy - Consumer cameras
  - Ubiquiti - UniFi Protect
  - Nextbase, BlackVue, Garmin, Thinkware, Viofo - Dash cams
  - GoPro - Action cameras

#### Categories
- CCTV - Fixed cameras
- ANPR - License plate recognition
- Drone - Aerial surveillance
- Body Cam - Wearable cameras
- Cloud CCTV - Internet-connected cameras
- Traffic - Traffic management systems
- Dash Cam - Vehicle cameras
- Doorbell - Smart doorbells

#### Display & UI
- 2.8" ILI9341 touchscreen support (240x320)
- Color-coded relevance indicators (Red/Yellow/Green)
- Real-time detection list display
- Status indicators (scanning, SD card, count)
- Touch controls for scan mode cycling
- Device detail cards with:
  - Manufacturer name
  - Category badge
  - MAC address
  - RSSI signal strength
  - Type (BLE/WiFi)
  - Deployment type
  - Notes

#### Data Logging
- Automatic CSV logging to microSD card
- Log fields: Timestamp, MAC, OUI, Manufacturer, Category, Relevance, Deployment, RSSI, Type, Notes
- Circular buffer to prevent memory overflow
- Detection deduplication
- Header auto-creation on first run

#### Proximity Alerts
- RSSI-based alert patterns
- Three proximity levels:
  - Very Close (>-50 dBm): 3 rapid beeps
  - Medium (>-70 dBm): 2 beeps
  - Far (>-90 dBm): 1 beep
- Relevance-based frequency (HIGH = 2000Hz, MEDIUM = 1500Hz)
- Silent mode support
- Only alerts on MEDIUM/HIGH relevance

#### Hardware Support
- ESP32-2432S028 development board
- ILI9341 TFT display driver
- Resistive touch controller
- MicroSD card (SPI, GPIO 5)
- Passive buzzer (GPIO 25)
- LED indicator (GPIO 4)
- Battery charging support

#### Analysis Tools
- **Python Script** (`analyze_detections.py`):
  - Statistical summary
  - Category breakdown
  - Relevance analysis
  - Top manufacturers ranking
  - RSSI proximity statistics
  - Deployment type analysis
  - High-risk device identification
  - CSV export of summary data

- **Web Visualization Tool** (`detections_viewer.html`):
  - Interactive dashboard
  - Real-time statistics cards
  - Category distribution charts
  - Relevance level visualization
  - Top 10 manufacturers chart
  - Advanced filtering (relevance, category, manufacturer)
  - Color-coded detection cards
  - 100% local processing (no server upload)
  - Responsive design for mobile/desktop
  - RSSI proximity indicators

#### Documentation
- Comprehensive README (500+ lines)
- Quick start guide
- Hardware setup guide with wiring diagrams
- Visualization tool documentation
- Legal and ethical use guidelines
- Troubleshooting guide
- Development roadmap
- Sample detection data

#### Example Data
- 50+ sample detections covering:
  - All device categories
  - All relevance levels
  - Multiple manufacturers
  - Range of RSSI values
  - Both BLE and WiFi types

#### Build System
- PlatformIO configuration
- Library dependencies:
  - TFT_eSPI v2.5.43
  - NimBLE-Arduino v1.4.1
  - ArduinoJson v6.21.3
- Optimized build flags for ESP32-2432S028
- Display driver configuration
- SPIFFS/LittleFS support

## [0.9.0] - Pre-release Development

### Added
- Initial WiFi promiscuous mode framework
  - WiFi packet sniffer callbacks
  - Management frame parsing
  - MAC address extraction
  - Channel scanning (1, 6, 11)
  - Deduplication cache
  - RSSI capture

### Notes
WiFi promiscuous mode is implemented but not yet integrated into main.cpp. This is a foundation for future packet-level WiFi detection beyond just AP scanning.

## Version History

### Version Numbering
- **1.0.0** - Initial stable release
- **1.x.x** - Feature additions, bug fixes
- **2.x.x** - Major architectural changes
- **x.1.x** - Minor features, improvements
- **x.x.1** - Bug fixes, patches

### Release Philosophy
- **Stable releases**: Thoroughly tested, production-ready
- **Beta releases**: Feature-complete, testing phase
- **Alpha releases**: Experimental, early testing

## Upgrade Notes

### From Development to v1.0.0
- No breaking changes
- New OUI entries may detect more devices
- Improved documentation
- Added visualization tools

### Future Breaking Changes
None planned for v1.x series. Major changes will be v2.0.0.

## Database Updates

### OUI Database v1.0
- 80+ UK-relevant surveillance device OUIs
- Covers major manufacturers in UK market
- Categorized by deployment type and relevance
- Manually curated and verified

### How to Update OUIs
1. Edit `include/oui_database.h`
2. Add new entry to `UK_OUI_DATABASE` array:
   ```cpp
   {"XX:XX:XX", "Manufacturer", CAT_TYPE, REL_LEVEL, DEPLOY_TYPE, "Notes"},
   ```
3. Rebuild and upload firmware
4. Database updates without reflashing coming in v1.1

## Known Issues

### v1.0.0
- WiFi promiscuous mode not integrated (framework only)
- Touch scrolling not yet implemented (fixed list)
- Settings screen shows placeholder
- No GPS support yet
- MAC randomization may prevent detection of some devices
- RSSI distance estimation varies by environment

### Workarounds
- Use relevance filtering for focused detection
- Tap screen top to change scan modes
- Review logs on SD card for complete history
- Use Python/web tools for advanced analysis

## Security Notes

### v1.0.0 Security Considerations
- Device performs passive monitoring only
- No active attacks or interference
- MAC addresses stored locally on SD card
- No network transmission of collected data
- Complies with UK passive radio monitoring laws

### Privacy
- User responsible for data handling
- GDPR compliance required for certain uses
- Recommend anonymizing MACs for public sharing
- Delete logs regularly if not needed

## Performance

### v1.0.0 Benchmarks
- **BLE Scan**: ~3s for full scan (Normal mode)
- **WiFi Scan**: ~2s for network scan
- **OUI Lookup**: <1ms per MAC
- **Display Update**: ~50ms full refresh
- **SD Write**: ~10ms per detection
- **Memory Usage**: ~200KB RAM (50 detections)

### Battery Life
- **Quick Mode**: 2-3 hours (1000mAh)
- **Normal Mode**: 3-5 hours (1000mAh)
- **Power-Save Mode**: 6-8 hours (1000mAh)

## Credits

### Contributors
- Initial concept and implementation
- OUI database research and curation
- Documentation and guides

### Libraries
- **Bodmer** - TFT_eSPI library
- **h2zero** - NimBLE-Arduino library
- **Espressif** - ESP32 Arduino framework

### Data Sources
- IEEE OUI Database
- UK surveillance equipment research
- Community feedback and testing

## License

MIT License with security research disclaimer.
See LICENSE file for full terms.

---

**Stay Updated**: Watch repository for new releases and OUI database updates.
