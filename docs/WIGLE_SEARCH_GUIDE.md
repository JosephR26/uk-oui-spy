# WiGLE Database Search Guide for Cardiff Surveillance

This guide provides specific OUI patterns to search in your WiGLE database to identify surveillance equipment deployed in Cardiff.

## Priority 1: CRITICAL - Facial Recognition Systems 🚨

Search for these OUIs to find **NEC NeoFace** and other facial recognition infrastructure:

### NEC Corporation (South Wales Police FR)
```
00:00:86  - NeoFace Live FR (CRITICAL - South Wales Police)
00:00:D1  - NEC FR Systems
00:40:66  - NEC FR Servers
00:1B:C0  - NEC Surveillance
```

### Other Facial Recognition Vendors
```
00:0E:3B  - Cognitec (FaceVACS FR System)
00:50:BA  - Cognitec (Facial Recognition)
00:50:56  - BriefCam (Video Analytics/FR)
A4:5E:60  - BriefCam (Forensic Video)
00:1A:6B  - Clearview AI (FR Database)
00:30:AB  - Idemia (Police Biometrics)
00:0E:2E  - Morpho (Legacy FR Systems)
00:50:C2  - Facewatch (Retail FR - Tesco/Co-op)
00:1C:23  - AnyVision (Retail FR)
A4:83:E7  - Auror (Retail Intelligence)
```

**Expected Locations:**
- Cardiff city center (Queen Street, St Mary Street)
- Principality Stadium area
- Major retail stores (Tesco, Co-op)
- Transport hubs (Cardiff Central, Cardiff Bay)

---

## Priority 2: HIGH - Police & Government Equipment

### Axon Enterprise (Police Body Cameras)
```
00:02:55  - UK Police Body Cams
00:18:F3  - Axon Body Cameras
6C:C7:EC  - Axon Fleet/Body Cams
```

### WatchGuard Video (Police Cameras)
```
00:0C:D4  - Police Dash/Body Cams
```

### Motorola Solutions (Police ANPR/Comms)
```
00:0A:28  - ANPR Systems
00:23:68  - Police Body Cameras
00:30:D3  - Traffic Systems
00:04:56  - Police Equipment
00:90:9C  - Traffic Management
00:D0:BC  - Public Safety ANPR
```

### Sepura (Police TETRA/BWV)
```
00:21:10  - Police Radio/BWV
```

**Expected Locations:**
- Police stations (Cardiff Central, Cardiff Bay)
- Major roads with ANPR
- Event locations (stadium, city center during events)

---

## Priority 3: MEDIUM - Council CCTV & Infrastructure

### Hikvision (Council/Police CCTV) - VERY COMMON
```
00:12:12  - UK Police/Council CCTV
28:57:BE  - IP Cameras
44:19:B6  - Network Cameras
BC:AD:28  - Transport CCTV
54:C4:15  - PTZ Cameras
C4:2F:90  - Smart Cameras
14:2D:27  - ANPR Systems
4C:BD:8F  - Network Cameras
68:E1:66  - IP CCTV
C0:56:E3  - Surveillance Cameras
D4:4B:5E  - DeepinMind ANPR
F0:1D:BC  - Smart Cameras
```

### Axis Communications (High-end Council CCTV)
```
00:40:8C  - Body Cams/CCTV
AC:CC:8E  - Network Cameras
B8:A4:4F  - Police Body Cameras
00:09:2D  - M-Series Cameras
00:50:C2  - P-Series Cameras
```

### Dahua Technology (Council/Retail)
```
00:12:16  - IP Cameras
08:60:6E  - Network CCTV
6C:C2:17  - Security Cameras
A0:BD:1D  - PTZ Cameras
78:D8:B5  - Traffic Cameras
00:26:37  - IP Cameras
2C:44:05  - Network Cameras
E8:CC:18  - HDCVI Cameras
F4:83:CD  - LPR Cameras
```

### Cardiff-Specific Council Infrastructure
```
00:0B:82  - Telent (Council CCTV Network)
00:30:65  - Telent (Traffic CCTV)
00:40:5A  - Vicon Industries (Professional CCTV)
00:0C:76  - Vicon Industries (VAX VMS)
00:0E:D7  - Oncam (360° Cameras)
00:19:3E  - Oncam (Grandeye Cameras)
00:1C:C4  - BCDVideo (Surveillance Servers)
00:12:FB  - Sunell (IP Cameras)
00:12:1E  - Geovision (Council CCTV)
00:09:6D  - Hanwha (Council Cameras)
```

**Expected Locations:**
- Entire city center
- Transport hubs
- Council buildings
- Major intersections
- Parks and public spaces

---

## Priority 4: ANPR & Traffic Enforcement

### Kapsch TrafficCom (ULEZ/Congestion - London, might be in Cardiff too)
```
00:03:52  - ULEZ/ANPR
00:21:5C  - Congestion Charging
```

### Jenoptik (Speed/ANPR Cameras)
```
00:0C:A4  - Speed/ANPR Cameras
```

### Genetec (ANPR Software)
```
00:0C:E5  - ANPR/Security Platform
00:15:C5  - AutoVu ANPR
```

### Siemens (Traffic Infrastructure)
```
00:0E:8C  - Traffic CCTV
00:50:7F  - Smart City Cameras
00:1B:1B  - Transport Systems
```

### SWARCO (Traffic Management)
```
00:30:05  - Traffic Signals/ANPR
```

**Expected Locations:**
- A470/M4 junctions
- Cardiff Bay link road
- Main arterial roads
- Speed camera locations
- Bus lanes

---

## Priority 5: Parking Enforcement

### Conduent (Major UK Contractor)
```
00:00:AA  - Parking Enforcement
00:08:02  - PCN Cameras
00:D0:B7  - Civil Enforcement
```

### NSL Services
```
00:1E:58  - Parking Enforcement
00:50:C2  - Bus Lane Cameras (note: shared OUI)
```

### APCOA Parking
```
00:0F:EA  - Car Park ANPR
00:30:48  - Parking Cameras
```

### ParkingEye (Capita)
```
00:26:5E  - ANPR Parking
```

### Euro Car Parks
```
00:1D:7E  - Private Parking
```

**Expected Locations:**
- Multi-story car parks (NCP, Q-Park)
- Retail car parks (St David's, Capitol Centre)
- On-street parking zones
- Cardiff Bay car parks
- Hospital/university parking

---

## Priority 6: Transport Surveillance

### March Networks (TfL-style Bus/Train Cameras)
```
00:03:C0  - Transit Surveillance
```

### 360 Vision Technology (UK Motorway PTZ)
```
00:0D:8B  - UK PTZ Cameras
```

**Expected Locations:**
- Cardiff Bus stops/depots
- Cardiff Central Station
- Cardiff Bay Station
- Train platforms
- Bus shelters

---

## Priority 7: Smart City Infrastructure (Your Field Test Findings!)

### Texas Instruments (Cardiff Smart Poles)
```
38:AB:41  - Cardiff Smart Poles ⭐ (YOU FOUND THIS!)
00:12:4B  - IoT Wireless Module
B0:B4:48  - CC2640 BLE Module
```

### TDK Corporation (Cardiff Poles)
```
00:80:98  - Cardiff Pole Sensors ⭐ (YOU FOUND THIS!)
00:1D:94  - Industrial IoT
```

### Ezurio/Laird (Cardiff Poles)
```
00:16:A4  - Cardiff Smart Infrastructure ⭐ (YOU FOUND THIS!)
00:50:C2  - Industrial Wireless (note: shared OUI)
```

### Fn-Link Technology (Cardiff Poles)
```
AC:64:CF  - Cardiff Pole Network ⭐ (YOU FOUND THIS!)
24:0A:C4  - Smart City WiFi
```

### Espressif (ESP32 - possible custom builds)
```
24:6F:28  - ESP32 IoT Module
30:AE:A4  - ESP32 WiFi/BLE
```

**Expected Locations:**
- Queen Street
- St Mary Street
- Principality Stadium perimeter
- Cardiff Bay waterfront
- Major transport routes

---

## WiGLE Search Tips

### CSV Export Search (if using wigle.net CSV export):
1. Download your Cardiff captures as CSV
2. Use grep/awk to filter by OUI prefix:
```bash
# Search for NEC (facial recognition)
grep -E "^00:00:86|^00:00:D1|^00:40:66|^00:1B:C0" wigle_cardiff.csv

# Search for Hikvision (council CCTV)
grep -E "^00:12:12|^28:57:BE|^44:19:B6|^BC:AD:28|^54:C4:15|^C4:2F:90" wigle_cardiff.csv

# Search for Cardiff smart poles (Texas Instruments)
grep -E "^38:AB:41|^00:12:4B|^B0:B4:48" wigle_cardiff.csv
```

### SQLite Database Search (if using local WiGLE app database):
```sql
-- Find all NEC equipment (facial recognition)
SELECT * FROM network WHERE bssid LIKE '00:00:86:%' OR bssid LIKE '00:00:D1:%' OR bssid LIKE '00:40:66:%' OR bssid LIKE '00:1B:C0:%';

-- Find all Hikvision cameras
SELECT * FROM network WHERE bssid LIKE '00:12:12:%' OR bssid LIKE '28:57:BE:%' OR bssid LIKE '44:19:B6:%';

-- Find Cardiff smart poles (Texas Instruments)
SELECT * FROM network WHERE bssid LIKE '38:AB:41:%' OR bssid LIKE '00:12:4B:%';

-- Get counts by manufacturer
SELECT SUBSTR(bssid, 1, 8) as oui_prefix, COUNT(*) as count
FROM network
WHERE bssid LIKE '00:00:86:%' OR bssid LIKE '00:12:12:%'
GROUP BY oui_prefix;
```

### Web Interface Search (wigle.net):
1. Go to https://wigle.net/
2. Use Advanced Search
3. Filter by location: Cardiff, Wales, UK
4. Search SSID/MAC containing these patterns
5. Export results

---

## What to Look For in Results

### High-Priority Indicators:

1. **NEC Equipment Near Event Venues** 🚨
   - If you find `00:00:86`, `00:00:D1`, or `00:40:66` near Principality Stadium or city center = FACIAL RECOGNITION LIKELY

2. **Pole Network Patterns**
   - Multiple `38:AB:41` (Texas Instruments) in close proximity = Networked surveillance infrastructure
   - Look for "AGD640-XXXXX-pole" SSID patterns

3. **High-Density Areas**
   - 10+ surveillance OUIs in single location = Major surveillance zone
   - Queen Street, St Mary Street, Cardiff Bay likely hotspots

4. **Unusual Retail Locations**
   - Facewatch (`00:50:C2`) in Tesco/Co-op = Active retail FR trial

---

## Reporting Back

When you find matches, please report:
1. **OUI prefix** (e.g., "00:00:86")
2. **Location** (street name or landmark)
3. **Count** (how many APs detected)
4. **SSID** (if visible and not hidden)
5. **Coordinates** (lat/lon if available)

This will help us:
- Confirm active FR deployments in Cardiff
- Map council CCTV coverage density
- Identify new surveillance vendors
- Expand the UK-OUI-SPY database with Cardiff-validated OUIs

---

## Example Search Workflow

```bash
# 1. Export your Cardiff WiGLE data to CSV

# 2. Search for critical FR systems
echo "=== NEC FACIAL RECOGNITION ==="
grep -E "00:00:86|00:00:D1|00:40:66|00:1B:C0" cardiff_wigle.csv | wc -l

# 3. Search for council CCTV (Hikvision most common)
echo "=== HIKVISION COUNCIL CCTV ==="
grep -E "00:12:12|28:57:BE|44:19:B6|BC:AD:28" cardiff_wigle.csv | wc -l

# 4. Search for smart poles (your findings)
echo "=== CARDIFF SMART POLES ==="
grep -E "38:AB:41|00:80:98|00:16:A4|AC:64:CF" cardiff_wigle.csv

# 5. Get top 20 locations by surveillance density
grep -E "00:12:12|28:57:BE|00:00:86|38:AB:41" cardiff_wigle.csv | \
  cut -d',' -f7,8 | sort | uniq -c | sort -rn | head -20
```

This will show you the hotspots!
