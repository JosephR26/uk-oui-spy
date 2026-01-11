# Frequently Asked Questions (FAQ)

Common questions about UK-OUI-SPY, its operation, legality, and usage.

## General Questions

### What is UK-OUI-SPY?

UK-OUI-SPY is a portable device based on ESP32 that detects nearby surveillance equipment by scanning for Bluetooth and WiFi signals. It matches detected MAC addresses against a database of known UK surveillance manufacturers to identify CCTV cameras, ANPR systems, drones, body cameras, and other monitoring devices.

### Is this legal in the UK?

**Yes, passive radio monitoring is legal in the UK.** The device:
- Only receives signals (passive monitoring)
- Does not transmit interfering signals
- Does not decrypt encrypted communications
- Does not break any encryption
- Complies with UK Wireless Telegraphy Act

**However:**
- Using this data for illegal purposes is not permitted
- You must comply with GDPR if collecting personal data
- Respect private property and trespassing laws
- Do not interfere with law enforcement operations

**We are not lawyers** - consult legal counsel if you have specific concerns about your use case.

### What can this device detect?

The device can detect surveillance equipment with active Bluetooth (BLE) or WiFi radios, including:

✅ **High Detection Rate:**
- IP CCTV cameras with WiFi/BLE
- Police body cameras (BLE)
- Police/commercial drones (WiFi)
- ANPR systems with network connectivity
- Cloud-connected cameras
- Smart doorbells
- Dash cameras with WiFi

❌ **Cannot Detect:**
- Analog CCTV (no wireless)
- Cameras with WiFi/BLE disabled
- Cameras using MAC randomization
- Devices in sleep mode
- Extremely far devices (>50m typically)

### How accurate is the detection?

**OUI Matching: Very Accurate**
- Database curated from verified sources
- Matches first 3 bytes of MAC (manufacturer ID)
- Updated regularly with new OUIs

**Distance Estimation: Approximate**
- RSSI varies by environment
- Walls, metal, and interference affect readings
- Estimates: ±50% accuracy
- Use as relative indicator, not precise measurement

**False Positives:**
- Some consumer devices share OUIs with surveillance manufacturers
- Relevance filtering helps (HIGH = police/gov only)
- Context matters (location, quantity)

### How far can it detect devices?

**Typical Ranges:**

| Technology | Indoor | Outdoor | Notes |
|------------|--------|---------|-------|
| **BLE** | 10-30m | 30-50m | Most body cams, some cameras |
| **WiFi 2.4GHz** | 30-100m | 100-200m | Most IP cameras, drones |
| **WiFi 5GHz** | 20-50m | 50-100m | Newer high-end cameras |

**Factors Affecting Range:**
- Device transmission power
- Building materials (concrete, metal reduce range)
- Interference from other devices
- Weather (rain reduces range)
- Antenna orientation

## Technical Questions

### Why do I see no detections?

**Common causes:**

1. **Not near surveillance devices**
   - Walk near known CCTV cameras
   - Try public areas with obvious cameras
   - Police stations, train stations, city centers

2. **Devices have WiFi/BLE disabled**
   - Many older CCTV systems are wired only
   - Some devices disable wireless to save power

3. **MAC randomization**
   - Some modern devices randomize MAC addresses
   - Defeats OUI-based detection
   - More common in consumer devices

4. **Wrong scan mode**
   - Try Quick scan mode for faster detection
   - Ensure BLE/WiFi both enabled in config

5. **Device malfunction**
   - Check serial monitor for errors
   - Verify antenna not damaged
   - Reflash firmware

### What's the difference between BLE and WiFi scanning?

**BLE (Bluetooth Low Energy):**
- **Range**: 10-50m
- **Power**: Very low
- **Detects**: Body cameras, some IP cameras, IoT devices
- **Pros**: Low power consumption, good for wearable devices
- **Cons**: Shorter range, fewer devices

**WiFi (2.4GHz/5GHz):**
- **Range**: 30-200m
- **Power**: Higher
- **Detects**: IP cameras, drones, access points
- **Pros**: Longer range, more surveillance devices use WiFi
- **Cons**: Higher power consumption

**Best Practice:** Enable both for complete coverage.

### How does the OUI database work?

**OUI (Organizationally Unique Identifier):**
- First 3 bytes of MAC address (XX:XX:XX)
- Assigned by IEEE to manufacturers
- Example: `A4:DA:32` = Hikvision

**Our Database:**
1. Contains 80+ OUIs of UK-relevant surveillance manufacturers
2. Each entry has:
   - OUI (3 bytes)
   - Manufacturer name
   - Category (CCTV, ANPR, etc.)
   - Relevance (HIGH/MEDIUM/LOW)
   - Typical deployment (Police, Council, etc.)
   - Notes

3. When a device is detected:
   - Extract first 3 bytes of MAC
   - Look up in database
   - If match → display detection
   - If no match → ignore

**Limitations:**
- Only detects manufacturers in database
- Shared OUIs may cause false positives
- New manufacturers not yet added

### Can I add my own OUIs?

**Yes! Two methods:**

**Method 1: Edit Database (requires reflashing)**
1. Open `include/oui_database.h`
2. Add entry to array:
   ```cpp
   {"XX:XX:XX", "Manufacturer", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Notes"},
   ```
3. Rebuild and upload firmware

**Method 2: Dynamic Loading (v1.1 planned)**
- Will support loading OUIs from SD card
- No reflashing required
- JSON format for easy editing

**Where to find OUIs:**
- [IEEE OUI Lookup](https://www.wireshark.org/tools/oui-lookup.html)
- [MAC Vendors Database](https://macvendors.com/)
- Scan unknown devices and research MAC

### Why isn't WiFi promiscuous mode working?

WiFi promiscuous mode is **implemented but not yet integrated** in v1.0.

**Current WiFi scanning:**
- Uses standard WiFi.scanNetworks()
- Detects access points (APs) only
- Simpler, more stable

**Promiscuous mode (coming in v1.1):**
- Captures all WiFi packets
- Detects non-AP devices (clients)
- More complex, requires packet parsing

**Workaround:**
- BLE scanning detects most body cams and wearables
- Current WiFi scanning detects IP cameras (they broadcast SSIDs)

## Usage Questions

### How long does the battery last?

**Depends on scan mode and battery capacity:**

**1000mAh LiPo:**
- Quick Mode (2s interval): 2-3 hours
- Normal Mode (5s interval): 3-4 hours
- Power-Save Mode (15s interval): 6-8 hours

**2000mAh LiPo:**
- Quick Mode: 4-6 hours
- Normal Mode: 6-8 hours
- Power-Save Mode: 12-15 hours

**Tips for longer battery life:**
- Use Power-Save mode
- Disable WiFi if only using BLE
- Use silent mode (no buzzer)
- Lower display brightness (future feature)

### What do the relevance colors mean?

**🔴 RED (HIGH Relevance)**
- Police equipment
- Government surveillance
- ANPR systems
- Body cameras
- Typically requires legal authority

**🟡 YELLOW (MEDIUM Relevance)**
- Council/municipal CCTV
- Transport surveillance
- Retail security
- Professional systems
- Typically for public safety

**🟢 GREEN (LOW Relevance)**
- Consumer cameras
- Private home security
- Doorbell cameras
- Dash cameras
- Typically personal use

### How do I interpret RSSI values?

**RSSI (Received Signal Strength Indicator):**

| RSSI Range | Distance | Meaning |
|------------|----------|---------|
| -30 to -50 dBm | 0-2m | **Very close** - Device right next to you |
| -50 to -70 dBm | 2-10m | **Near** - Same room or nearby |
| -70 to -90 dBm | 10-30m | **Medium** - Across street or building |
| Below -90 dBm | 30m+ | **Far** - Multiple buildings away |

**Important Notes:**
- RSSI is NOT precise distance
- Walls and obstacles reduce signal
- Use as relative indicator only
- Closer = higher RSSI (less negative)

### Why do some detections repeat?

**Normal Behavior:**
- Devices broadcast continuously
- Scanner picks them up each scan cycle
- Deduplication merges detections within 30 seconds

**If seeing many repeats:**
- Check if device is stationary (e.g., fixed CCTV)
- Deduplication is working (timestamps update)
- Log file will show all instances

**Benefits:**
- Track signal strength changes (movement)
- Verify device is still present
- Build confidence in detection

### How do I analyze the log files?

**Three tools provided:**

**1. CSV in Excel/LibreOffice**
- Open `detections.csv` directly
- Sort and filter columns
- Create pivot tables
- Good for basic analysis

**2. Python Analysis Script**
```bash
python analysis/analyze_detections.py detections.csv
```
- Statistical breakdown
- Top manufacturers
- Category analysis
- Terminal output with colors

**3. Web Visualization Tool**
- Open `visualization/detections_viewer.html`
- Load CSV file
- Interactive charts
- Filter and explore

## Privacy & Security Questions

### Does this device transmit data?

**No external transmission:**
- No WiFi client connection (station mode only for scanning)
- No Bluetooth connections to other devices
- No internet upload
- No phone app communication

**Data stored locally:**
- MicroSD card only
- You control the data
- No cloud sync (optional in future versions)

### Is the data encrypted?

**v1.0:** No encryption on SD card
- Data stored as plain CSV
- Anyone with SD card can read it

**Recommendations:**
1. Remove SD card when not in use
2. Encrypt your computer storage
3. Delete logs regularly
4. Anonymize MACs before sharing

**Future:** Encrypted logging option planned for v1.2

### What data is collected?

**Per Detection:**
- Timestamp (when detected)
- MAC address (device identifier)
- OUI (manufacturer ID)
- Manufacturer name
- Device category
- Relevance level
- Deployment type
- RSSI (signal strength)
- Type (BLE or WiFi)
- Notes

**NOT Collected:**
- Your location (unless GPS added)
- Device communications content
- Personal information
- Encryption keys
- Network traffic

### Should I be concerned about GDPR?

**Personal Use:** Generally not a GDPR issue
- MAC addresses may be personal data under GDPR
- Private research/security awareness likely exempt

**Professional/Commercial Use:** GDPR applies
- Document legitimate interest
- Implement data minimization
- Provide data security
- Allow data subject rights
- Consult legal counsel

**Recommendations:**
- Anonymize data before sharing
- Don't publish full MAC addresses
- Use for security research only
- Follow ICO guidance

## Legal Questions

### Can I use this for journalism/research?

**Yes, likely covered under:**
- Journalistic freedom
- Academic research
- Public interest
- Security research

**Best Practices:**
- Document methodology
- Anonymize personal data
- Obtain legal advice for publication
- Follow journalistic ethics codes

**Examples:**
- ✅ Investigating public space surveillance
- ✅ Researching privacy implications
- ✅ Security awareness projects
- ❌ Stalking or harassment
- ❌ Corporate espionage

### Can I use this to avoid surveillance?

**Educational purpose:** Understanding surveillance in your environment

**Not designed for:**
- Evading lawful surveillance
- Interfering with police operations
- Criminal activity
- Trespassing to avoid detection

**Reality Check:**
- Knowing camera locations doesn't mean you're unobserved
- Cameras may have no wireless (undetectable)
- Other surveillance methods exist (human, optical, etc.)

### What about the Computer Misuse Act?

**UK Computer Misuse Act 1990:**

**Not applicable to UK-OUI-SPY because:**
- No unauthorized access to computers
- No modification of computer material
- Passive monitoring only
- No hacking or intrusion

**Illegal uses that WOULD violate CMA:**
- Using detected info to hack cameras
- Accessing surveillance feeds without authorization
- Disabling security systems

**This device is passive monitoring only** - like using eyes to see cameras.

### Can police confiscate this device?

**Unlikely, but possible if:**
- Used during commission of a crime
- Suspected to be evidence
- In restricted area (e.g., airport secure zone)

**Your rights:**
- Device is legal to own and use
- Passive monitoring is lawful
- No obligation to hand over without warrant

**If questioned:**
- Explain it's a security research tool
- Show it's passive monitoring only
- Provide documentation
- Cooperate professionally

**Advice:** Don't use in obviously sensitive areas (police stations, military bases, etc.)

## Troubleshooting Questions

### Display is blank/garbled

**Blank screen:**
1. Check power (USB or battery charged)
2. Press reset button
3. Check TFT cable connection
4. Verify display settings in platformio.ini

**Garbled output:**
1. Wrong display driver (should be ILI9341)
2. Incorrect pin configuration
3. SPI communication issue
4. Reflash firmware

### SD card not detected

**Checklist:**
1. Card inserted fully (clicks in place)
2. Card formatted as FAT32
3. Card capacity 4-32GB (SDHC)
4. Try different SD card
5. Check GPIO 5 connection

**Format command (Windows):**
```cmd
format F: /FS:FAT32
```

**Format command (Mac/Linux):**
```bash
sudo mkfs.vfat -F 32 /dev/sdX1
```

### No buzzer sound

**Checks:**
1. Buzzer connected to GPIO 25
2. Buzzer polarity correct (+/-)
3. Passive buzzer (not active)
4. Alert mode not silent
5. Detection is MEDIUM/HIGH relevance
6. Volume (some buzzers are quiet)

**Test code:**
```cpp
tone(BUZZER_PIN, 2000, 200); // Should beep
```

### Device keeps resetting

**Power issues:**
1. USB power insufficient (try 2A charger)
2. Battery voltage low (<3.5V)
3. Power supply poor quality
4. Loose connections

**Software issues:**
1. Memory overflow (too many detections)
2. Stack corruption
3. Watchdog timeout
4. Check serial monitor for errors

## Future Development Questions

### Will there be firmware updates?

**Yes! Planned updates:**

**v1.1 (Q2 2025):**
- WiFi promiscuous mode integration
- Enhanced touch UI with scrolling
- Full settings screen
- Dynamic OUI loading from SD card

**v1.2 (Q3 2025):**
- GPS integration
- Location tagging
- Offline mapping
- Encrypted logging

**v2.0 (Q4 2025):**
- Pattern recognition
- Device tracking
- Multi-unit networking
- Advanced analytics

**How to update:**
- Download new firmware
- Upload via PlatformIO
- Settings preserved on SD card

### Can I contribute?

**Absolutely! Contributions welcome:**

**Code:**
- Bug fixes
- New features
- Performance improvements
- Pull requests on GitHub

**Database:**
- New OUI entries
- Manufacturer research
- Category improvements

**Documentation:**
- Tutorials
- Use cases
- Translations
- Videos

**Testing:**
- Field reports
- Bug reports
- Performance data

**See**: CONTRIBUTING.md (coming soon)

### Will there be a mobile app?

**Planned for v1.3:**
- Bluetooth connection to ESP32
- Real-time detection on phone
- Better battery life (offload processing)
- Cloud sync (optional)

**Platforms:**
- Android first (easier BLE)
- iOS later (App Store restrictions)

**Features:**
- Live detection feed
- Map visualization
- Detection history
- Alert settings

## More Questions?

**Resources:**
- 📖 Read the full [README.md](../README.md)
- 🔧 Check [HARDWARE_SETUP.md](HARDWARE_SETUP.md)
- 📊 View [CHANGELOG.md](../CHANGELOG.md)
- 💬 GitHub Discussions (ask community)
- 🐛 GitHub Issues (report bugs)

**Still stuck?**
Open an issue on GitHub with:
- Your question
- What you've tried
- Serial monitor output
- Photos (if hardware issue)

---

**Happy Surveillance Detecting!** 🕵️‍♀️🔍
