# Hardware Setup Guide

Complete hardware guide for the ESP32-2432S028 surveillance detector.

## Required Components

### ESP32-2432S028 Development Board

The primary target board, commonly known as the "Cheap Yellow Display" (CYD).

| Spec | Detail |
|------|--------|
| **Processor** | ESP32 dual-core @ 240MHz |
| **Display** | 2.8" ILI9341 TFT (240x320 pixels, SPI) |
| **Touch** | XPT2046 resistive touch controller (SPI) |
| **RAM** | 520KB SRAM |
| **Flash** | 4MB |
| **Connectivity** | Wi-Fi 802.11 b/g/n, Bluetooth 4.2 BLE |
| **Price** | ~15-20 GBP |

**Where to buy:**
- AliExpress: search "ESP32-2432S028"
- eBay: "ESP32 2.8 TFT display"
- Amazon UK: "ESP32 development board with display"

> **Important**: There are multiple CYD variants with different touch controllers. This firmware requires the **XPT2046 resistive touch** version (16-pin TSSOP chip on the PCB, active-low CS on GPIO 33 via SPI). If your board has a capacitive touch controller (CST820/FT6236 on I2C), it will not work with this firmware without modification.

### MicroSD Card

- **Capacity**: 4GB to 32GB
- **Format**: FAT32
- **Class**: Class 10 or higher recommended
- **Price**: ~3-5 GBP

Cards larger than 32GB may work but require manual FAT32 formatting.

## Optional Components

### LiPo Battery

- **Voltage**: 3.7V single cell
- **Capacity**: 1000-2000mAh recommended
- **Connector**: JST PH 2.0 2-pin
- **Price**: ~5-8 GBP

**Runtime estimates:**

| Battery | Normal Mode | Power-Save Mode |
|---------|-------------|-----------------|
| 1000mAh | 2-3 hours | 6-8 hours |
| 1500mAh | 3-5 hours | 10-12 hours |
| 2000mAh | 5-7 hours | 12-15 hours |

With deep sleep mode enabled, a 1000mAh battery can last 24+ hours.

A USB power bank (5V) is a simpler alternative for extended field use.

### GPS Module (Optional)

- Any serial GPS module (e.g. NEO-6M, NEO-7M)
- Enables location tagging on all detections
- Connect to a spare UART on the ESP32

## Pin Assignments

### Display (SPI)

| Pin | GPIO | Function |
|-----|------|----------|
| MISO | 12 | SPI data in |
| MOSI | 13 | SPI data out |
| SCLK | 14 | SPI clock |
| CS | 15 | Display chip select |
| DC | 2 | Data/command |
| RST | -1 | Not connected (tied high) |
| BL | 21 | Backlight (active HIGH) |

### Touch (SPI -- shared bus)

| Pin | GPIO | Function |
|-----|------|----------|
| T_CS | 33 | Touch chip select (active LOW) |
| T_IRQ | 36 | Touch interrupt (active LOW) |

The XPT2046 shares the SPI bus with the display (MISO/MOSI/SCLK). Only the chip select (CS) differs.

### Other

| Pin | GPIO | Function |
|-----|------|----------|
| SD CS | 5 | SD card chip select |
| LED R | 4 | RGB LED red channel |
| LED G | 16 | RGB LED green channel |
| LED B | 17 | RGB LED blue channel |
| LDR | 34 | Light-dependent resistor (ADC) |
| BAT | 35 | Battery voltage (ADC) |

## Assembly

### Step 1: Inspect the Board

1. Verify the TFT display is properly seated
2. Locate the XPT2046 touch controller IC (16-pin TSSOP, faint printing on chip)
3. Locate the MicroSD card slot (usually on the back/bottom edge)

### Step 2: Prepare the SD Card

```
Format: FAT32
Allocation unit size: 4096 bytes (default)
```

Insert the card into the slot -- it should click into place.

### Step 3: Battery Connection (Optional)

1. Check polarity on the JST connector (red = positive, black = negative)
2. Connect to the battery port on the board
3. **NEVER reverse polarity** -- this will damage the board

Most CYD boards have a built-in TP4056 charging circuit. Connect USB to charge the battery automatically.

## Enclosure Options

### 3D Printed

Search Thingiverse or Printables for "ESP32-2432S028 case". Look for designs with:
- USB port access
- SD card slot access
- Belt clip or lanyard mount
- Battery compartment

Print settings: PLA or PETG, 0.2mm layer height, 20% infill.

### Off-the-Shelf

Hammond 1551G series (95x65x25mm) with cutouts for display and USB. Or any generic ABS project box ~90x60x30mm.

### DIY

A mint tin (Altoids-style) with foam padding works well for a quick field case.

## Testing

### Power-On Test

1. Connect USB
2. Display should light up with the splash screen
3. Setup Wizard runs on first boot
4. SD indicator should show green if card is detected

### Touch Test

Tap the screen during the Setup Wizard. The NEXT button should respond to touch input. If touch doesn't work:
- Verify your board has XPT2046 (check the chip markings)
- Check that TOUCH_CS is set to GPIO 33 in `platformio.ini`

### SD Card Test

1. Insert a FAT32-formatted card (optionally with `priority.json` for enhanced features)
2. Boot the device
3. The INFO screen should show the OUI database count (230 entries)

## Troubleshooting

### Display blank

- Check USB power source (use a quality cable)
- Press the reset button on the board
- Verify `platformio.ini` display settings match your board

### Touch not responding

- Confirm your board uses XPT2046 (look for the 16-pin chip)
- Boards with capacitive touch (CST820/FT6236) need different firmware
- Check serial monitor for touch debug output

### SD card not found

- Reformat as FAT32 (not exFAT)
- Try a different card (some cheap cards have compatibility issues)
- Check GPIO 5 is not shorted or in use

### Device resets randomly

- Use a higher-current USB source (2A minimum)
- Check battery voltage (should be above 3.5V)
- Reduce scan frequency in settings

## Safety

- Never short-circuit battery terminals
- Don't expose to water (the board is not waterproof)
- Avoid crushing LiPo batteries (fire hazard)
- Store batteries at ~50% charge if unused for over a month

## Further Resources

- [ESP32 Datasheet](https://www.espressif.com/en/products/socs/esp32)
- [TFT_eSPI Library](https://github.com/Bodmer/TFT_eSPI)
- [PlatformIO Documentation](https://docs.platformio.org/)
- [XPT2046 Datasheet](https://www.buydisplay.com/download/ic/XPT2046.pdf)
