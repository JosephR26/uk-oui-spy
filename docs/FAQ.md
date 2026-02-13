# Detailed FAQ

Extended frequently asked questions covering operation, legality, accuracy, and technical details.

## Detection & Accuracy

### What can this device detect?

The device detects surveillance equipment with active Bluetooth (BLE) or Wi-Fi radios, including:

- IP CCTV cameras with Wi-Fi/BLE
- Police body cameras (BLE)
- Police/commercial drones (Wi-Fi)
- ANPR systems with network connectivity
- Cloud-connected cameras (Ring, Nest, etc.)
- Smart doorbells
- Dash cameras with Wi-Fi
- Facial recognition infrastructure

**Cannot detect:**
- Analog CCTV (no wireless radio)
- Cameras with Wi-Fi/BLE disabled
- Devices using MAC address randomisation
- Devices in sleep mode
- Devices beyond range (~50m for BLE, ~200m for Wi-Fi)

### How accurate is the detection?

**OUI Matching:** Very accurate. The database is curated from verified sources and matches the first 3 bytes of the MAC address (manufacturer ID).

**Distance Estimation:** Approximate. RSSI varies by environment -- walls, metal, and interference all affect readings. Use RSSI as a relative indicator, not a precise distance measurement.

**False Positives:** Possible. Some consumer devices share OUIs with surveillance manufacturers. Use relevance filtering (HIGH = police/government only) and consider context (location, quantity of detections).

### How far can it detect devices?

| Technology | Indoor | Outdoor |
|------------|--------|---------|
| BLE | 10-30m | 30-50m |
| Wi-Fi 2.4GHz | 30-100m | 100-200m |
| Wi-Fi 5GHz | 20-50m | 50-100m |

Range varies with transmission power, building materials, interference, and weather.

### What do the relevance colours mean?

- **Red (HIGH)** -- Police equipment, government surveillance, ANPR, body cameras
- **Orange** -- Surveillance infrastructure, commercial security systems
- **Yellow (MEDIUM)** -- Council CCTV, transport surveillance, retail security
- **Green (LOW)** -- Consumer cameras, doorbells, dash cameras

### How do I interpret RSSI values?

| RSSI | Approximate Distance | Meaning |
|------|---------------------|---------|
| -30 to -50 dBm | 0-2m | Very close |
| -50 to -70 dBm | 2-10m | Nearby (same room) |
| -70 to -90 dBm | 10-30m | Medium (across street) |
| Below -90 dBm | 30m+ | Far away |

RSSI is not precise distance. Use as a relative indicator only.

## Technical

### How does the OUI database work?

OUI (Organizationally Unique Identifier) is the first 3 bytes of a MAC address, assigned by IEEE to manufacturers. For example, `A4:DA:32` = Hikvision.

When the device detects a Wi-Fi or BLE signal, it extracts the first 3 bytes of the MAC and looks them up in the database. If there's a match, the detection is displayed with manufacturer name, category, and relevance level.

### Can I add my own OUIs?

Yes. Edit the `oui.csv` file on your SD card to add new entries:

```csv
XX:XX:XX,Manufacturer Name,CCTV,HIGH,Police,"Description"
```

Or edit `include/oui_database.h` in the source and rebuild the firmware for entries in the static fallback database.

To find OUIs for a specific manufacturer, search:
- [IEEE OUI Lookup](https://standards.ieee.org/products-programs/regauth/)
- [Wireshark OUI Lookup](https://www.wireshark.org/tools/oui-lookup.html)
- [MAC Vendors](https://macvendors.com/)

### What's the difference between BLE and Wi-Fi scanning?

**BLE (Bluetooth Low Energy):** Shorter range (10-50m), lower power, detects body cameras, some IP cameras, IoT devices.

**Wi-Fi:** Longer range (30-200m), higher power consumption, detects IP cameras, drones, access points.

**Promiscuous Mode:** Captures raw Wi-Fi management frames (probe requests, beacons) -- detects devices that aren't broadcasting as access points. Provides ~30% more detections than standard Wi-Fi scanning.

Best practice: enable both BLE and Wi-Fi for complete coverage.

### How do I analyse detection logs?

Three tools are provided:

1. **CSV in Excel/LibreOffice** -- Open `detections.csv` directly, sort and filter columns
2. **Python script** -- `python analysis/analyze_detections.py detections.csv` -- statistical breakdown with terminal output
3. **Web viewer** -- Open `visualization/detections_viewer.html` in a browser, load the CSV for interactive charts and filtering

## Privacy & Security

### Does this device transmit data?

No. The device only receives signals passively. All detection data is stored locally on the SD card. There is no internet upload, no cloud sync, and no phone app communication.

The embedded web portal operates on a closed local Wi-Fi network created by the device -- it does not connect to the internet.

### Is the log data encrypted?

Optional. When AES-128 encryption is enabled in Settings, all logs written to the SD card are encrypted.

When encryption is disabled, data is stored as plain CSV. Recommendations:
- Remove the SD card when not in use
- Delete logs regularly if not needed
- Anonymise MAC addresses before sharing any data publicly

### What about GDPR?

MAC addresses may be considered personal data under GDPR. For personal security research, this is generally not an issue. For professional or commercial use, consult legal counsel regarding legitimate interest, data minimisation, and data subject rights.

## Legal

### Is passive radio monitoring legal in the UK?

Yes. The device only receives publicly broadcast signals. It does not transmit interfering signals, does not decrypt encrypted communications, and complies with the UK Wireless Telegraphy Act. See [LEGAL.md](../LEGAL.md) for the full disclaimer.

### What about the Computer Misuse Act?

Not applicable. The device performs passive monitoring only -- no unauthorised access to computers, no modification of data, no hacking or intrusion. Using detected information to access surveillance feeds without authorisation would be illegal, but the detection itself is lawful.

### Can I use this for journalism or research?

Yes, this is generally covered under journalistic freedom, academic research, public interest, and security research. Document your methodology and anonymise personal data before publication.

## More Questions?

- Check the [Quick Start Guide](../QUICKSTART.md)
- Read the [User Manual](../USER_MANUAL.md)
- See [Hardware Setup](HARDWARE_SETUP.md) for board-specific issues
- Open an issue on GitHub: https://github.com/JosephR26/uk-oui-spy/issues
