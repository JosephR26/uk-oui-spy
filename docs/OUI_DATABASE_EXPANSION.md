# OUI Database Guide

## Overview

The OUI (Organizationally Unique Identifier) database is the core of UK-OUI-SPY's detection capability. Each entry maps a manufacturer's IEEE-assigned MAC address prefix to a device category and relevance level.

The current database contains **280+ OUI entries** covering surveillance equipment commonly deployed in the UK.

## How OUI Matching Works

Every Wi-Fi and Bluetooth device broadcasts a MAC address. The first 3 bytes (e.g. `A4:DA:32`) identify the manufacturer -- this is the OUI, assigned by the IEEE. When the device detects a signal, it extracts the OUI and looks it up in the database.

## Database Format

### SD Card Database (oui.csv)

The primary database is loaded from `oui.csv` on the SD card at boot:

```csv
OUI,Manufacturer,Category,Relevance,Deployment,Notes
A4:DA:32,Hikvision,CCTV,HIGH,Police,"UK Police/Council CCTV"
```

**Fields:**
- **OUI** -- 3-byte MAC prefix (XX:XX:XX)
- **Manufacturer** -- Company name
- **Category** -- Device type (CCTV, ANPR, Drone, Body Cam, Cloud CCTV, Traffic, Dash Cam, Doorbell, Facial Recog, Parking, Smart City)
- **Relevance** -- HIGH, MEDIUM, or LOW
- **Deployment** -- Typical use context (Police, Council, Consumer, etc.)
- **Notes** -- Additional context

### Static Fallback Database (oui_database.h)

A compiled-in fallback database in `include/oui_database.h` is used when no SD card is present. The SD card database takes priority when available.

## Adding New OUIs

### Method 1: Edit oui.csv (Recommended)

1. Find the manufacturer's OUI using one of these lookup tools:
   - [IEEE OUI Registry](https://standards.ieee.org/products-programs/regauth/)
   - [Wireshark OUI Lookup](https://www.wireshark.org/tools/oui-lookup.html)
   - [MAC Vendors](https://macvendors.com/)

2. Add a new line to `oui.csv` on your SD card:
   ```csv
   XX:XX:XX,Manufacturer Name,CCTV,HIGH,Police,"Description"
   ```

3. Reboot the device -- the new entry will be loaded automatically.

### Method 2: Edit the Source

Edit `include/oui_database.h` and rebuild the firmware. This adds entries to the compiled-in fallback database.

## Database Coverage

### By Category

| Category | Entries | Examples |
|----------|---------|----------|
| CCTV | ~130 | Hikvision, Dahua, Axis, Hanwha, Bosch |
| ANPR/Traffic | ~25 | Kapsch, Jenoptik, Genetec, Siemens, SWARCO |
| Body Cameras | ~12 | Axon, WatchGuard, Sepura, Zepcam |
| Cloud CCTV | ~30 | Cisco Meraki, Verkada, Eagle Eye Networks |
| Drones | ~8 | DJI, Autel, Yuneec, senseFly |
| Doorbell Cams | ~9 | Ring, Google Nest, Eufy |
| Facial Recog | ~10 | NEC, Cognitec, BriefCam, Idemia |
| Parking | ~8 | Conduent, ParkingEye, APCOA |
| Smart City | ~10 | Texas Instruments, TDK, Ezurio |
| Traffic | ~15 | Siemens, Kapsch, Redflex |
| Dash Cams | ~6 | Nextbase, Viofo, BlackVue |

### By Relevance

| Level | Entries | Typical Sources |
|-------|---------|-----------------|
| HIGH | ~70 | Police equipment, government CCTV, ANPR, facial recognition |
| MEDIUM | ~85 | Council CCTV, transport systems, commercial security |
| LOW | ~75 | Consumer cameras, doorbells, dash cams |

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

1. Note the device's MAC address (visible in the web portal or detection logs)
2. Look up the OUI at [IEEE](https://standards.ieee.org/products-programs/regauth/)
3. Open a GitHub issue with:
   - The OUI prefix
   - Manufacturer name
   - Device category
   - Evidence of UK deployment (product page, news article, procurement record)

## Updating the Database

Check the GitHub repository periodically for updated `oui.csv` and `priority.json` files. Copy the new files to your SD card to update.
