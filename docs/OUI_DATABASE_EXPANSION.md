# OUI Database Guide

## Overview

The OUI (Organizationally Unique Identifier) database is the core of UK-OUI-SPY's detection capability. Each entry maps a manufacturer's IEEE-assigned MAC address prefix to a device category and relevance level.

The current database contains **230 OUI entries** covering surveillance equipment commonly deployed in the UK.

## How OUI Matching Works

Every Wi-Fi and Bluetooth device broadcasts a MAC address. The first 3 bytes (e.g. `A4:DA:32`) identify the manufacturer -- this is the OUI, assigned by the IEEE. When the device detects a signal, it extracts the OUI and looks it up in the compiled database.

## Database Format

The database is a static `const` array in `src/oui_database.cpp`:

```cpp
const OUIEntry OUI_DATABASE[] = {
    {"A4:DA:32", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Government/Council CCTV"},
    // ...
};
```

**Fields:**
- **OUI** -- 3-byte MAC prefix (XX:XX:XX)
- **Manufacturer** -- Company name
- **Category** -- Device type (CCTV, ANPR, Drone, Body Cam, Cloud CCTV, Traffic, Dash Cam, Doorbell, Facial Recog, Parking, Smart City)
- **Relevance** -- HIGH, MEDIUM, or LOW
- **Deployment** -- Typical use context (Police, Council, Transport, Retail, Private, Government)
- **Notes** -- Additional context

## Adding New OUIs

1. Find the manufacturer's OUI using one of these lookup tools:
   - [IEEE OUI Registry](https://standards.ieee.org/products-programs/regauth/)
   - [Wireshark OUI Lookup](https://www.wireshark.org/tools/oui-lookup.html)
   - [MAC Vendors](https://macvendors.com/)

2. Add a new entry to the `OUI_DATABASE[]` array in `src/oui_database.cpp`:
   ```cpp
   {"XX:XX:XX", "Manufacturer Name", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Description"},
   ```

3. Rebuild and flash the firmware.

## Database Coverage

### By Category

| Category | Entries | Examples |
|----------|---------|----------|
| CCTV | ~100 | Hikvision, Dahua, Axis, Hanwha, Bosch |
| ANPR/Traffic | ~25 | Kapsch, Jenoptik, Genetec, Siemens, SWARCO |
| Body Cameras | ~9 | Axon, Edesix, Reveal Media, Sepura, Zepcam |
| Cloud CCTV | ~20 | Cisco Meraki, Verkada, Eagle Eye Networks |
| Drones | ~11 | DJI, Autel, Yuneec, Skydio, senseFly |
| Doorbell Cams | ~9 | Ring, Google Nest, Eufy, Yale |
| Facial Recog | ~13 | NEC, Cognitec, BriefCam, Idemia, Clearview AI |
| Parking | ~8 | Conduent, ParkingEye, APCOA |
| Smart City | ~10 | Texas Instruments, TDK, Ezurio, Espressif |
| Dash Cams | ~7 | Nextbase, Viofo, BlackVue, Thinkware |

### By Relevance

| Level | Entries | Typical Sources |
|-------|---------|-----------------|
| HIGH | ~80 | Police equipment, government CCTV, ANPR, facial recognition |
| MEDIUM | ~80 | Council CCTV, transport systems, commercial security |
| LOW | ~70 | Consumer cameras, doorbells, dash cams |

### UK-Specific Highlights

**London Infrastructure:**
- Kapsch TrafficCom -- ULEZ and Congestion Charge Zone ANPR
- Siemens -- Smart city infrastructure
- March Networks -- TfL bus and station CCTV

**UK Police Equipment:**
- Axon -- Body cameras (Metropolitan Police, most UK forces)
- Sepura -- TETRA radio with integrated body-worn video
- WatchGuard -- Police vehicle cameras
- Motorola -- Airwave/ESN communications

**UK Manufacturers:**
- 360 Vision Technology -- PTZ cameras (motorways, councils)
- Dedicated Micros -- Legacy CCTV (still widely deployed)
- Wavestore -- Video management systems
- IndigoVision -- IP CCTV

## Verification Sources

All OUI assignments are verified against the IEEE OUI Registry. Manufacturer-to-category mappings are based on:

- IEEE OUI database
- Manufacturer product documentation
- UK police equipment procurement records
- Transport for London supplier information
- UK surveillance industry publications

## Contributing New OUIs

If you find a surveillance device not in the database:

1. Note the device's MAC address (visible in detection logs)
2. Look up the OUI at [IEEE](https://standards.ieee.org/products-programs/regauth/)
3. Open a GitHub issue with:
   - The OUI prefix
   - Manufacturer name
   - Device category
   - Evidence of UK deployment (product page, news article, procurement record)
