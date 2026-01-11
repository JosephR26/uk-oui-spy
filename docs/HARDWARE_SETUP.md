# Hardware Setup Guide - UK-OUI-SPY ESP32 v6

Complete hardware assembly and configuration guide for the ESP32-2432S028R surveillance detector.

## Table of Contents

1. [Required Components](#required-components)
2. [Optional Components](#optional-components)
3. [Assembly Instructions](#assembly-instructions)
4. [Wiring Diagram](#wiring-diagram)
5. [Power Configuration](#power-configuration)
6. [Enclosure Options](#enclosure-options)
7. [Testing](#testing)
8. [Troubleshooting](#troubleshooting)

## Required Components

### Primary Hardware

**ESP32-2432S028R Development Board**
- **Processor**: ESP32 dual-core @ 240MHz
- **Display**: 2.8" ILI9341 TFT (240x320 pixels)
- **Touch**: Resistive touch controller
- **RAM**: 520KB SRAM
- **Flash**: 4MB
- **Connectivity**: WiFi 802.11 b/g/n, Bluetooth 4.2 BLE
- **Price**: ~£15-20 GBP

**Where to buy:**
- AliExpress: Search "ESP32-2432S028R"
- eBay: ESP32 2.8" TFT display
- Amazon UK: "ESP32 development board with display"

### Storage

**MicroSD Card**
- **Capacity**: 4GB to 32GB
- **Format**: FAT32
- **Class**: Class 10 or higher recommended
- **Price**: ~£3-5 GBP

**Note**: Larger cards (64GB+) may work but require exFAT formatting which may not be compatible.

## Optional Components

### Audio Alerts

**Passive Buzzer**
- **Type**: Passive (piezo) buzzer
- **Voltage**: 3-5V
- **Frequency**: 2-4kHz
- **Connection**: GPIO 25
- **Price**: ~£0.50-1 GBP

**Active Buzzer (alternative)**
- Simpler to drive but less flexible frequency control
- Connect positive to GPIO 25, negative to GND

### Power Supply

**LiPo Battery**
- **Voltage**: 3.7V single cell
- **Capacity**: 1000-2000mAh recommended
- **Connector**: JST PH 2.0 2-pin
- **Price**: ~£5-8 GBP

**Runtime estimates:**
- 1000mAh: 2-3 hours (Normal mode)
- 1500mAh: 3-5 hours (Normal mode)
- 2000mAh: 5-7 hours (Power-save mode)

**USB Power Bank (alternative)**
- Any 5V USB power bank works
- Connect via USB-C or Micro-USB (board dependent)
- Easier to replace/recharge

### Portable Configuration

**Optional Extras:**
- **Vibration Motor**: For silent alerts (connect to GPIO 25 instead of buzzer)
- **LED Indicator**: External LED for status (GPIO 4)
- **Tactile Buttons**: For physical controls (GPIO 35, 0, 14)
- **Lanyard**: For wearable operation

## Assembly Instructions

### Step 1: Prepare the ESP32 Board

1. **Inspect the board**
   - Check for damaged components
   - Ensure TFT display is properly seated
   - Verify touch digitizer connection

2. **Test basic functionality**
   - Connect via USB
   - Power on and check if display lights up
   - Verify touch response (if pre-loaded with demo)

### Step 2: MicroSD Card Setup

1. **Format the card**
   ```
   Format: FAT32
   Allocation unit size: 4096 bytes (default)
   Volume label: UKOUI
   ```

2. **Test card detection**
   - Insert card into slot on back of ESP32 board
   - Ensure it clicks into place
   - Card slot is usually on the bottom edge

### Step 3: Optional Buzzer Installation

**Passive Buzzer Wiring:**
```
Buzzer Positive (+) → GPIO 25
Buzzer Negative (-) → GND
```

**Physical mounting:**
- Use double-sided tape or hot glue
- Position away from display for clear sound
- Can be mounted on back of enclosure

### Step 4: Battery Connection (Optional)

**LiPo Battery:**
1. Check polarity on JST connector (red = +, black = -)
2. Connect to battery port on ESP32 board
3. **NEVER reverse polarity** - will damage board

**Charging:**
- Most ESP32-2432S028R boards have built-in TP4056 charging
- Connect USB to charge battery automatically
- Red LED = charging, Blue/Green LED = charged

## Wiring Diagram

```
┌─────────────────────────────────────┐
│     ESP32-2432S028R (Top View)      │
│                                     │
│  ┌─────────────────────────────┐   │
│  │                             │   │
│  │      2.8" TFT Display       │   │
│  │         (ILI9341)           │   │
│  │                             │   │
│  │         Touch Layer         │   │
│  │                             │   │
│  └─────────────────────────────┘   │
│                                     │
│  [GPIO 25] ─────────► Buzzer (+)   │
│  [GND]     ─────────► Buzzer (-)   │
│  [GPIO 4]  ─────────► LED (+)      │
│                                     │
│  [Battery JST] ◄────── LiPo 3.7V   │
│  [USB Port]    ◄────── Charging    │
└─────────────────────────────────────┘

Back View:
┌─────────────────────────────────────┐
│                                     │
│       [MicroSD Card Slot]           │
│                                     │
│  ┌─────────┐                        │
│  │ ESP32   │  [Reset Button]        │
│  │ Module  │  [Boot Button]         │
│  └─────────┘                        │
└─────────────────────────────────────┘
```

## Power Configuration

### Power Sources

| Source | Voltage | Current | Runtime | Best For |
|--------|---------|---------|---------|----------|
| USB 5V | 5V | 500mA | Unlimited | Development/Testing |
| LiPo 3.7V | 3.7-4.2V | 1000mAh | 2-3hrs | Portable Operation |
| Power Bank | 5V | 2000mAh+ | 5-8hrs | Extended Field Use |

### Power Consumption

**Typical Current Draw:**
- **Idle**: ~80mA
- **BLE Scanning**: ~120mA
- **WiFi Scanning**: ~150mA
- **Display On**: +40mA
- **Buzzer Active**: +20mA (brief pulses)

**Power Saving Tips:**
- Use Power-Save scan mode (15s intervals)
- Reduce display brightness (future feature)
- Disable WiFi if only using BLE
- Use silent mode (no buzzer)

## Enclosure Options

### 3D Printed Cases

**Recommended designs:**
- Search Thingiverse: "ESP32-2432S028R case"
- Printables.com: ESP32 TFT case designs

**Features to look for:**
- Access to USB port for charging
- MicroSD card slot accessibility
- Buzzer hole for audio output
- Belt clip or lanyard mount
- Battery compartment

**Print Settings:**
- Material: PLA or PETG
- Layer height: 0.2mm
- Infill: 20%
- Supports: Usually required

### Off-the-Shelf Enclosures

**Hammond plastic cases:**
- Model: 1551G series (95x65x25mm)
- Requires cutouts for display and USB

**Generic project boxes:**
- Size: 90x60x30mm minimum
- Material: ABS plastic
- Modify with drill/Dremel for ports

### DIY Field Case

**Materials:**
- Mint tin or Altoids tin
- Foam padding
- Velcro or elastic strap

**Advantages:**
- Cheap and accessible
- Built-in shielding
- Easy to modify

## Testing

### Initial Power-On Test

1. **Connect USB**
2. **Observe startup sequence**:
   - Display should light up
   - "UK-OUI-SPY v6" splash screen
   - Initialization messages
   - Start scanning

3. **Check SD card**:
   - "SD" indicator should show green
   - If red/missing, reseat card

4. **Test touch**:
   - Tap top of screen
   - Should cycle scan modes
   - Watch for mode change

### BLE Scan Test

1. **Enable Bluetooth on phone**
2. **Place phone near device**
3. **Wait 5-10 seconds**
4. **Check detection**:
   - May detect phone if using surveillance OUI
   - More likely to detect nearby cameras/devices

### SD Card Logging Test

1. **Let device run for 2 minutes**
2. **Power off**
3. **Remove SD card**
4. **Insert into computer**
5. **Check for `/detections.csv`**
6. **Verify data is logging**

### Buzzer Test

1. **Walk near known CCTV camera**
2. **Listen for beeps**
3. **Closer = faster beeps**
4. **High relevance = higher pitch**

**Note**: Only HIGH and MEDIUM relevance devices trigger alerts.

## Troubleshooting

### Display Issues

**Symptom**: Blank white screen
- **Cause**: Display not initialized
- **Fix**: Check TFT cable connection, reflash firmware

**Symptom**: Display shows garbled output
- **Cause**: Wrong display driver
- **Fix**: Verify platformio.ini settings (ILI9341)

**Symptom**: Touch not responding
- **Cause**: Touch calibration needed
- **Fix**: Most boards work without calibration, check wiring

### SD Card Issues

**Symptom**: "SD Card not found" message
- **Cause**: Card not inserted or wrong format
- **Fix**:
  1. Check card is fully inserted
  2. Reformat as FAT32
  3. Try different SD card
  4. Check GPIO 5 not shorted

**Symptom**: Detections not logging
- **Cause**: SD write error
- **Fix**:
  1. Check card has free space
  2. Verify file permissions
  3. Try slower SD card class

### Buzzer Issues

**Symptom**: No sound from buzzer
- **Cause**: Wrong buzzer type or wiring
- **Fix**:
  1. Verify GPIO 25 connection
  2. Check buzzer polarity
  3. Ensure passive (not active) buzzer
  4. Test with simple tone() function

**Symptom**: Buzzer always on or random noise
- **Cause**: Floating GPIO or interference
- **Fix**: Add 10K pulldown resistor to GPIO 25

### Power Issues

**Symptom**: Device resets randomly
- **Cause**: Insufficient power
- **Fix**:
  1. Use higher current USB source (2A)
  2. Check battery voltage (should be >3.5V)
  3. Reduce scan frequency

**Symptom**: Battery not charging
- **Cause**: TP4056 charging circuit issue
- **Fix**:
  1. Check USB cable quality
  2. Verify battery polarity
  3. Test with different battery

### Detection Issues

**Symptom**: No detections showing
- **Cause**: Not near surveillance devices or MAC randomization
- **Fix**:
  1. Walk near known CCTV cameras
  2. Try different scan mode (Quick)
  3. Verify BLE/WiFi enabled in config
  4. Some devices use MAC randomization

**Symptom**: Too many false detections
- **Cause**: OUI collision with consumer devices
- **Fix**: Filter by relevance level (HIGH only)

## Maintenance

### Regular Checks

- **SD Card**: Reformat every 6 months or 100,000 detections
- **Battery**: Replace after 300-500 charge cycles (~1 year)
- **Buzzer**: Test monthly for functionality
- **Firmware**: Check for updates quarterly

### Cleaning

- **Display**: Use microfiber cloth, screen-safe cleaner
- **Case**: Wipe with damp cloth
- **Ports**: Compressed air for dust removal

### Storage

- **Battery**: Store at 50% charge if unused >1 month
- **SD Card**: Keep in device or anti-static bag
- **Temperature**: Store at 15-25°C, avoid extreme heat/cold

## Safety Notes

⚠️ **Electrical Safety:**
- Never short circuit battery terminals
- Don't expose to water (not waterproof)
- Avoid crushing LiPo battery (fire hazard)

⚠️ **Usage Safety:**
- Don't use while driving
- Be aware of surroundings when monitoring
- Respect private property

⚠️ **Legal Safety:**
- Comply with local laws
- Don't interfere with surveillance systems
- Use for passive monitoring only

## Further Resources

- **ESP32 Datasheet**: https://www.espressif.com/en/products/socs/esp32
- **TFT_eSPI Library**: https://github.com/Bodmer/TFT_eSPI
- **PlatformIO Docs**: https://docs.platformio.org/

---

**Hardware setup complete!** Proceed to firmware upload and configuration.
