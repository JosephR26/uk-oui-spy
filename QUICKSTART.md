# Quick Start Guide

Get up and running in 5 minutes.

## What You Need

- **ESP32-2432S028** board (CYD with XPT2046 resistive touch)
- **MicroSD card** (4-32GB, FAT32)
- **USB cable** (Micro-USB or USB-C, depending on your board variant)
- **[VS Code](https://code.visualstudio.com/)** with the **[PlatformIO extension](https://platformio.org/install/ide?install=vscode)**

## Step 1: Clone & Build

Install [VS Code](https://code.visualstudio.com/) with the [PlatformIO extension](https://platformio.org/install/ide?install=vscode), then:

```bash
git clone https://github.com/JosephR26/uk-oui-spy.git
cd uk-oui-spy
pio run
```

Or open the project folder in VS Code and click the PlatformIO Upload button (arrow icon in the bottom toolbar).

## Step 2: Prepare SD Card

1. Format a microSD card as **FAT32**
2. Copy `examples/oui.csv` and `examples/priority.json` to the card root
3. Insert the card into the board's SD slot

## Step 3: Flash & Boot

```bash
pio run --target upload
```

If upload fails, hold the **BOOT** button on the board while uploading.

On first boot, the **Setup Wizard** checks your hardware (touch, SD card, battery). Tap **NEXT** then **GO** to proceed.

## Step 4: Start Scanning

The device begins scanning automatically. You'll see detections appear on screen as colour-coded cards:

- **Red** = High relevance (police/government equipment)
- **Orange** = Surveillance infrastructure
- **Yellow** = Medium relevance (traffic, council)
- **Green** = Low relevance (consumer devices)

Use the **navigation bar** at the bottom to switch between screens (LIST, RADAR, CONFIG, INFO).

## Step 5: Web Portal (Optional)

For a larger dashboard, connect your phone or laptop:

1. Connect to Wi-Fi: **OUI-SPY-PRO**
2. Password: **spypro2026**
3. Open browser: **http://192.168.4.1**

## Checking Logs

Detection data is logged to `detections.csv` on the SD card. You can:

- Open it directly in Excel or LibreOffice
- Run the analysis script: `python analysis/analyze_detections.py detections.csv`
- Use the web viewer: open `visualization/detections_viewer.html` in a browser

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Blank screen | Check USB power, press reset button |
| Touch not responding | Verify your board has XPT2046 (not capacitive touch) |
| SD card not detected | Reformat as FAT32, try a different card |
| No detections | Walk near known CCTV cameras, check scan mode |
| Upload fails | Hold BOOT button during upload, check COM port |

## Next Steps

- Read the full [User Manual](USER_MANUAL.md) for all features
- See [Hardware Setup](docs/HARDWARE_SETUP.md) for battery and enclosure options
- Check the [FAQ](FAQ.md) for common questions
- Update the OUI database with new manufacturers -- see [OUI Database Guide](docs/OUI_DATABASE_EXPANSION.md)

## Support

Open an issue on GitHub: https://github.com/JosephR26/uk-oui-spy/issues
