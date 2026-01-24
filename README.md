# UK-OUI-SPY ESP32 v6

A portable, discreet device that detects, logs, and alerts on UK-relevant surveillance and law-enforcement devices via their Bluetooth/Wi-Fi MAC addresses (OUIs), optimized for field use, analysis, and security awareness.

![Status](https://img.shields.io/badge/status-active-success)
![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![License](https://img.shields.io/badge/license-MIT-green)

## Overview

UK-OUI-SPY is a specialized surveillance detection tool designed for security researchers, privacy advocates, and those interested in understanding the surveillance landscape in their environment. By scanning for Bluetooth Low Energy (BLE) and Wi-Fi MAC addresses and matching them against a curated database of UK surveillance equipment manufacturers, this device provides real-time awareness of nearby surveillance devices.

### Key Features

- **UK-Specific OUI Database**: Curated list of **230+ surveillance device OUIs** covering 80+ manufacturers
- **Multi-Protocol Scanning**: BLE and Wi-Fi (2.4GHz) MAC address detection
- **Proximity Alerts**: RSSI-based distance estimation with audio/visual feedback
- **Touchscreen UI**: Color-coded, scrollable detection list on 2.8" ILI9341 display
- **Data Logging**: CSV export to microSD card for analysis
- **Multiple Scan Modes**: Quick (2s), Normal (5s), Power-Save (15s)
- **Portable & Discrete**: Battery-powered, handheld form factor

## Hardware Requirements

### Primary Components

- **ESP32-2432S028** development board
  - ESP32 dual-core processor
  - 2.8" ILI9341 TFT touchscreen (240x320)
  - Built-in resistive touch controller
  - Integrated battery charging circuit

### Optional Components

- **MicroSD card** (4GB-32GB, FAT32 formatted) for logging
- **Passive buzzer** (connected to GPIO 25) for audio alerts
- **3.7V LiPo battery** (1000-2000mAh recommended)
- **3D printed case** for field deployment

### Pin Configuration

| Component | GPIO Pin |
|-----------|----------|
| TFT Display | Hardware SPI (pre-configured) |
| SD Card CS | GPIO 5 |
| Buzzer | GPIO 25 |
| LED Indicator | GPIO 4 |

## Software Setup

### Prerequisites

1. **PlatformIO** - Install via VS Code or CLI:
   ```bash
   pip install platformio
   ```

2. **VS Code** (optional but recommended):
   - Install PlatformIO IDE extension

### Installation

1. Clone this repository:
   ```bash
   git clone https://github.com/yourusername/uk-oui-spy.git
   cd uk-oui-spy
   ```

2. Build the project:
   ```bash
   pio run
   ```

3. Upload to ESP32:
   ```bash
   pio run --target upload
   ```

4. Monitor serial output:
   ```bash
   pio device monitor
   ```

### Dependencies

All dependencies are automatically managed by PlatformIO:

- **TFT_eSPI** (v2.5.43) - Display driver
- **NimBLE-Arduino** (v1.4.1) - Bluetooth Low Energy stack
- **ArduinoJson** (v6.21.3) - JSON parsing (future updates)

## Usage

### First Boot

1. Insert formatted microSD card (optional)
2. Power on the device
3. Wait for initialization (~3 seconds)
4. Scanning begins automatically

### Display Overview

```
┌─────────────────────────────────────┐
│ UK-OUI-SPY        SCAN  SD  12  ◄─── Status bar
│ Mode: NORMAL                         │
├─────────────────────────────────────┤
│ ┃ Hikvision                          │ ◄─── Detection entry
│ ┃ CCTV | RSSI:-65                    │      (Red bar = High relevance)
│ ┃ A4:DA:32:XX:XX:XX [BLE]            │
│ ┃ UK Police/Council CCTV             │
├─────────────────────────────────────┤
│ ┃ DJI                                │
│ ┃ Drone | RSSI:-78                   │
│ ┃ 60:60:1F:XX:XX:XX [WiFi]           │
│ ┃ Police Drones                      │
└─────────────────────────────────────┘
```

**Status Indicators:**
- **SCAN/IDLE** - Current scanning status
- **SD** - MicroSD card detected (green = available)
- **Number** - Total detections in memory

**Relevance Color Coding:**
- **Red Bar** = High relevance (police/government equipment)
- **Yellow Bar** = Medium relevance (retail/transport CCTV)
- **Green Bar** = Low relevance (consumer cameras)

### Touch Controls

- **Tap header area** - Cycle scan modes (QUICK → NORMAL → POWER-SAVE)
- **Swipe up/down** - Scroll detection list (future feature)
- **Long press** - Open settings (future feature)

### Scan Modes

| Mode | Interval | BLE Scan Time | Power Consumption |
|------|----------|---------------|-------------------|
| QUICK | 2 seconds | 1 second | High |
| NORMAL | 5 seconds | 3 seconds | Medium |
| POWER-SAVE | 15 seconds | 5 seconds | Low |

### Proximity Alerts

The device emits audio alerts based on signal strength (RSSI):

- **Very Close** (>-50 dBm): 3 rapid beeps, high pitch
- **Medium** (>-70 dBm): 2 beeps, medium pitch
- **Far** (>-90 dBm): Single beep

**Note:** Only HIGH and MEDIUM relevance devices trigger alerts to reduce false positives from consumer devices.

## OUI Database

The device includes **230+ OUI entries** from UK-relevant surveillance manufacturers (v1.0.1 expansion):

### High-Relevance Devices (Red) - Police/Government

**Body Cameras & Police Equipment:**
- **Axon Enterprise** ⭐ - UK police body camera standard (Met Police, most UK forces)
- **Axis Communications** - Body cameras, transport CCTV
- **WatchGuard Video** - Police vehicle cameras
- **Sepura** - UK police TETRA radios with BWV
- **Zepcam** - Body worn cameras (police/NHS)
- **Digital Barriers, Edesix, Reveal Media** - UK body camera specialists
- **Motorola Solutions** - ANPR systems, police body cams, TETRA

**CCTV & Surveillance:**
- **Hikvision** (12 OUIs) - Major UK police/council CCTV supplier
- **Avigilon** - HD surveillance, ANPR systems
- **FLIR Systems** - Thermal imaging cameras
- **Bosch Security** - ANPR, traffic systems
- **Milestone Systems** - Police VMS (XProtect)

**ANPR & Traffic Enforcement:**
- **Kapsch TrafficCom** ⭐⭐ - London ULEZ & Congestion Charging cameras
- **Genetec** - ANPR software platforms (AutoVu)
- **Jenoptik** - Speed/ANPR cameras (UK police)
- **NDI Recognition Systems** - ANPR specialists
- **SWARCO** - Traffic management/ANPR
- **Siemens** - Traffic CCTV, smart city infrastructure

**Drones:**
- **DJI** - Police drones, aerial surveillance (Enterprise/Matrice)
- **Yuneec** - H520 police drones
- **Autel Robotics** - Commercial/police drones

### Medium-Relevance Devices (Yellow) - Council/Transport/Retail

**UK Transport & Infrastructure:**
- **March Networks** ⭐ - Transport for London (TfL) buses/trains surveillance
- **Siemens** - Smart city cameras, transport systems
- **360 Vision Technology** - UK PTZ motorway cameras

**UK-Based Manufacturers:**
- **Wavestore** - UK VMS manufacturer
- **IndigoVision** - UK IP CCTV specialist
- **Dedicated Micros** - UK CCTV manufacturer (legacy)
- **Videcon** - UK CCTV installer/integrator

**Enterprise CCTV:**
- **Dahua Technology** (8 OUIs) - Council/retail/transport IP cameras
- **Hanwha/Samsung** (5 OUIs) - Retail/council CCTV, Wisenet series
- **Cisco Meraki** (3 OUIs) - Enterprise cloud cameras
- **Uniview** - Growing UK market share
- **Verkada** - Hybrid cloud CCTV
- **Vivotek** - IP surveillance cameras
- **Pelco** - Professional surveillance
- **Sony** - Government network cameras
- **Panasonic** - i-PRO cameras, transport systems
- **Canon, Honeywell, Dallmeier** - Professional security

### Low-Relevance Devices (Green) - Consumer

**Popular UK Consumer Brands:**
- **TP-Link** ⭐ - Tapo/Kasa cameras (very popular in UK)
- **Xiaomi** - Mi/Aqara cameras
- **Ring (Amazon)** - Consumer doorbell cameras, Stick Up Cam
- **Google Nest** - Cloud security cameras, Hello Doorbell
- **Anker/Eufy** - Consumer security cameras
- **Ubiquiti** (6 OUIs) - UniFi Protect systems
- **Reolink** - Consumer IP cameras
- **Arlo, Wyze, Blink** - Cloud cameras

**Dash Cameras:**
- **Nextbase** - UK dash cam market leader
- **BlackVue, Garmin, Thinkware, Viofo** - Premium dash cams
- **GoPro** - Action cameras

### Updating the Database

To add new OUIs, edit `include/oui_database.h`:

```cpp
{"XX:XX:XX", "Manufacturer Name", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Notes"},
```

OUI format: First 3 bytes of MAC address (e.g., `"A4:DA:32"`)

## Data Logging

Detections are automatically logged to `/detections.csv` on the microSD card:

```csv
Timestamp,MAC,OUI,Manufacturer,Category,Relevance,Deployment,RSSI,Type,Notes
1234567,A4:DA:32:XX:XX:XX,A4:DA:32,Hikvision,CCTV,HIGH,Police,-65,BLE,UK Police CCTV
```

### Analyzing Logs

Transfer the CSV file to your computer and analyze with:

- **Excel/LibreOffice** - Basic filtering and sorting
- **Python/Pandas** - Advanced analysis and visualization
- **GIS Software** - Location-based mapping (with GPS data)

Example Python analysis:

```python
import pandas as pd

df = pd.read_csv('detections.csv')
print(df['Manufacturer'].value_counts())
print(df.groupby('Category')['RSSI'].mean())
```

## Technical Details

### MAC Address Scanning

**BLE Scanning:**
- Uses NimBLE library for low-power scanning
- Detects advertised devices only (passive scan)
- Range: ~10-50 meters depending on environment

**Wi-Fi Scanning:**
- Station mode, no connection required
- Scans for access point beacons
- Range: ~50-200 meters depending on environment

**Limitations:**
- Only detects devices with active BLE/Wi-Fi
- MAC randomization may prevent detection
- Building materials affect range

### RSSI Distance Estimation

Approximate distance based on signal strength:

| RSSI (dBm) | Approximate Distance |
|------------|----------------------|
| -30 to -50 | 0-2 meters (very close) |
| -50 to -70 | 2-10 meters (near) |
| -70 to -90 | 10-30 meters (medium) |
| < -90 | 30+ meters (far) |

**Note:** RSSI is affected by obstacles, interference, and device transmission power.

### Memory Management

- **Circular buffer**: Stores last 50 detections in RAM
- **Deduplication**: Updates existing entries within 30-second window
- **Logging**: Offloads to SD card to prevent memory overflow

## Security & Privacy Considerations

### Legal Notice

This tool is designed for **security research, privacy awareness, and educational purposes only**. Users are responsible for complying with local laws regarding:

- Electronic surveillance
- Radio frequency monitoring
- Privacy regulations (GDPR, DPA 2018)
- Telecommunications regulations

**In the UK:**
- Passive radio monitoring is generally legal
- No encryption breaking or active interference
- Do not use for malicious surveillance or stalking
- Respect private property and restricted areas

### Ethical Use

- **Transparency**: This tool detects surveillance, it does not enable it
- **Public Awareness**: Helps citizens understand their surveillance environment
- **Defensive Security**: Identifies potential security risks
- **Research**: Supports academic study of surveillance infrastructure

**Do NOT use this tool to:**
- Stalk, harass, or invade privacy
- Interfere with law enforcement operations
- Trespass on private property
- Violate any applicable laws

## Troubleshooting

### Common Issues

**Display shows "No detections"**
- Ensure you're in an area with surveillance devices
- Try QUICK scan mode for faster detection
- Check BLE/WiFi are enabled in settings
- Some devices may use MAC randomization

**SD card not detected**
- Ensure card is FAT32 formatted
- Check card is properly inserted
- Try a different SD card (4-32GB)
- Some cards require slower SPI speeds

**No audio alerts**
- Check buzzer is connected to GPIO 25
- Verify alert mode is not SILENT
- Only HIGH/MEDIUM relevance triggers alerts
- Ensure buzzer polarity is correct

**Short battery life**
- Use POWER-SAVE scan mode
- Reduce screen brightness (future feature)
- Use larger capacity battery
- Disable logging if not needed

## Development Roadmap

### v6.1 - Enhanced UI
- [ ] Scrollable detection list
- [ ] Settings menu with touch controls
- [ ] Relevance filtering
- [ ] Screen brightness control

### v6.2 - Advanced Scanning
- [ ] Wi-Fi promiscuous mode packet capture
- [ ] Bluetooth Classic support
- [ ] Signal strength graphs
- [ ] Detection history view

### v6.3 - Connectivity
- [ ] GPS module integration
- [ ] Location tagging of detections
- [ ] Offline map display
- [ ] Multi-device aggregation via WiFi

### v7.0 - Intelligence
- [ ] Pattern recognition (recurring devices)
- [ ] Behavioral analysis
- [ ] Threat scoring
- [ ] Cloud sync (optional)

## Contributing

Contributions are welcome! Areas of interest:

1. **OUI Database Expansion** - Add more UK surveillance manufacturers
2. **UI Improvements** - Enhanced touchscreen interface
3. **Power Optimization** - Longer battery life
4. **Analysis Tools** - Python scripts for log analysis
5. **Case Designs** - 3D printable enclosures

### How to Contribute

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/new-ouis`)
3. Commit your changes (`git commit -m 'Add 10 new CCTV OUIs'`)
4. Push to the branch (`git push origin feature/new-ouis`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Disclaimer

This project is an independent security research tool and is not affiliated with, endorsed by, or connected to any law enforcement agency, surveillance manufacturer, or government entity. All manufacturer and product names are trademarks of their respective owners.

The authors and contributors are not responsible for any misuse of this tool. Users assume all responsibility for complying with applicable laws and regulations.

## Acknowledgments

- **ESP32 Community** - For excellent Arduino framework
- **Bodmer** - TFT_eSPI library
- **h2zero** - NimBLE-Arduino library
- **UK Privacy Advocates** - For raising awareness of surveillance issues
- **Open Source Community** - For making projects like this possible

## References

- [IEEE OUI Database](https://standards.ieee.org/products-programs/regauth/)
- [UK Surveillance Camera Code of Practice](https://www.gov.uk/government/publications/surveillance-camera-code-of-practice)
- [ESP32 Technical Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [GDPR Guidance (ICO)](https://ico.org.uk/for-organisations/guide-to-data-protection/guide-to-the-general-data-protection-regulation-gdpr/)

---

**Built with ESP32 | Powered by Open Source | For Security Awareness**
