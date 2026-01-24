# Quick Start Guide - UK-OUI-SPY ESP32 v6

Get up and running in 5 minutes!

## Step 1: Install PlatformIO

**Option A - VS Code (Recommended)**
1. Install [VS Code](https://code.visualstudio.com/)
2. Install PlatformIO IDE extension from marketplace
3. Restart VS Code

**Option B - Command Line**
```bash
pip install platformio
```

## Step 2: Clone & Build

```bash
# Clone repository
git clone https://github.com/yourusername/uk-oui-spy.git
cd uk-oui-spy

# Build firmware
pio run

# You should see: "SUCCESS" at the end
```

## Step 3: Upload to ESP32

```bash
# Connect ESP32-2432S028 via USB
# Auto-detect and upload
pio run --target upload

# If upload fails, hold BOOT button and try again
```

## Step 4: First Use

1. **Optional**: Insert formatted microSD card (FAT32)
2. Power on device (USB or battery)
3. Wait 3 seconds for initialization
4. Device starts scanning automatically!

## What You'll See

```
┌─────────────────────────────────┐
│ UK-OUI-SPY        SCAN  SD  0   │  ← Status
│ Mode: NORMAL                     │
├─────────────────────────────────┤
│ [Detection list appears here]   │
│                                  │
│ Scanning for devices...          │
└─────────────────────────────────┘
```

## First Detection

When a surveillance device is detected:

```
┌─────────────────────────────────┐
│ UK-OUI-SPY        SCAN  SD  1   │
│ Mode: NORMAL                     │
├─────────────────────────────────┤
│ ┃ Hikvision                      │  ← Manufacturer
│ ┃ CCTV | RSSI:-65                │  ← Category & signal
│ ┃ A4:DA:32:XX:XX:XX [BLE]        │  ← MAC & type
│ ┃ UK Police/Council CCTV         │  ← Notes
└─────────────────────────────────┘
```

- **Red bar** = High relevance (police/gov)
- **RSSI** = Signal strength (-30 to -100)
- **Beep** = Proximity alert (if buzzer connected)

## Basic Controls

- **Tap top of screen** → Change scan mode (QUICK/NORMAL/POWER-SAVE)
- **Leave running** → Auto-logs to SD card

## Checking Logs

1. Power off device
2. Remove microSD card
3. Insert into computer
4. Open `detections.csv` in Excel/LibreOffice

Or use the analysis tool:

```bash
pip install pandas
python analysis/analyze_detections.py /path/to/detections.csv
```

## Troubleshooting

**No detections showing**
- Walk near CCTV cameras or surveillance equipment
- Try different scan mode (tap screen top)
- Some devices use MAC randomization

**Screen blank**
- Check USB power
- Try pressing reset button
- Check TFT connections (if DIY build)

**SD card not detected**
- Ensure FAT32 format
- Try different SD card
- Check GPIO 5 connection

## Next Steps

- Read full [README.md](README.md) for detailed features
- Update OUI database with new manufacturers
- Analyze logs with Python tools
- Contribute new features!

## Support

- GitHub Issues: https://github.com/yourusername/uk-oui-spy/issues
- Discussions: Check repository discussions tab

---

**Happy Surveillance Detecting! 🕵️**
