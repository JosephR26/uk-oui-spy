# FAQ & Troubleshooting

## General

**Q: What is UK-OUI-SPY PRO?**

A: An open-source, portable device that detects nearby surveillance equipment (CCTV, ANPR, drones, body cameras, etc.) by identifying manufacturer codes (OUIs) in their Wi-Fi and Bluetooth signals. Built on the ESP32 platform with a 2.8" touchscreen.

**Q: Is this legal in the UK?**

A: The device passively receives publicly broadcast signal metadata (MAC addresses and signal strength). It does not intercept, record, or decrypt any communications content. Passive reception of broadcast metadata is not prohibited under the Wireless Telegraphy Act 2006, provided no private communications content is intercepted or disclosed (Section 48). You are responsible for using it ethically and in compliance with all applicable laws. See [LEGAL.md](LEGAL.md) for the full disclaimer.

**Q: What can't it detect?**

A: Devices that don't broadcast Wi-Fi or Bluetooth (analog CCTV, wired-only cameras), devices with radios disabled or in sleep mode, and devices using MAC address randomisation.

**Q: How often should I update the OUI database?**

A: Check the repository monthly for updated `oui.csv` and `priority.json` files. New devices and manufacturers appear regularly.

**Q: What battery life can I expect?**

A: With a 1000mAh LiPo in normal scan mode: 2-3 hours. With deep sleep enabled: 24+ hours. Powered via USB, runtime is unlimited.

**Q: Can I use it while charging?**

A: Yes, the device works normally while connected to USB power.

## Hardware

**Q: Which ESP32 board do I need?**

A: The **ESP32-2432S028** (CYD) with **XPT2046 resistive touch** (SPI). There are several CYD variants -- check that your board has the XPT2046 chip (16-pin TSSOP) rather than a capacitive touch controller (CST820/FT6236).

**Q: My touch screen doesn't respond. What's wrong?**

A: Most likely your board has a different touch controller. This firmware is built for XPT2046 (resistive, SPI). Boards with capacitive touch (CST820 or FT6236 on I2C) require firmware modifications. Check the chip markings on your PCB.

**Q: What SD card should I use?**

A: Any microSD card from 4GB to 32GB, formatted as FAT32. Class 10 recommended. Cards larger than 32GB may need manual FAT32 formatting.

## Troubleshooting

**Problem: The screen is blank or unresponsive.**

- Check USB power (try a different cable or power source)
- Press the reset button on the board
- If the screen was working before, re-flash the firmware

**Problem: Setup Wizard shows "SD Card: N/A".**

- Ensure the card is formatted as FAT32 and inserted fully
- Try a different SD card brand
- Check that GPIO 5 is not shorted

**Problem: Setup Wizard shows "Touch: FAIL".**

- Verify your board uses XPT2046 (check the 16-pin chip)
- If you have a capacitive touch board, the firmware needs modification

**Problem: The Web Portal won't load.**

- Confirm you're connected to the "OUI-SPY-PRO" Wi-Fi network (password: `spypro2026`)
- Disable mobile data on your phone temporarily
- Check that the Web Portal is enabled in the CFG screen

**Problem: No detections showing.**

- Walk near known CCTV cameras or surveillance equipment
- Ensure BLE and Wi-Fi scanning are both enabled in settings
- Some devices use MAC randomisation and won't be detected
- Their OUI may not be in the database yet -- report it as an issue

**Problem: Device keeps resetting.**

- Use a USB power source capable of 2A
- If on battery, check voltage is above 3.5V
- Reduce scan frequency in settings

---

**Still need help?** Open an issue on GitHub: https://github.com/JosephR26/uk-oui-spy/issues
