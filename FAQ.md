# FAQ & Troubleshooting

## Frequently Asked Questions

**Q: What is the UK-OUI-SPY PRO?**

A: It is a specialized device that detects nearby surveillance equipment (like CCTV, ANPR, and drones) by identifying the unique manufacturer codes (OUIs) in their Wi-Fi and Bluetooth signals.

**Q: Is this device legal to own and use in the UK?**

A: Yes. The device operates by passively listening to publicly broadcast signals, which is legal in the UK. It does not intercept, record, or decrypt any content. However, you are responsible for using the device ethically and in compliance with all local laws. See the full legal disclaimer.

**Q: Why can't I see a device that I know is there?**

A: There are several possibilities:
1.  The device may not be actively broadcasting Wi-Fi or Bluetooth signals.
2.  It may be using a wired connection (Ethernet).
3.  It may be using MAC address randomization, a technique that makes it harder to track.
4.  Its OUI may not yet be in our database. Please help us by reporting new devices!

**Q: How often should I update the `oui.csv` and `priority.json` files?**

A: We recommend updating them monthly. New devices and manufacturers appear regularly, and an up-to-date database is key to the device's effectiveness.

**Q: What is the battery life?**

A: A full charge typically provides 4-6 hours of continuous operation. The device will automatically enter a deep sleep mode after a period of inactivity to conserve power. You can wake it by tapping the screen.

**Q: Can I use the device while it is charging?**

A: Yes, the device is fully functional while connected to a USB power source.

## Troubleshooting Guide

**Problem: The device won't turn on.**

*   **Solution**: The battery is likely depleted. Charge the device for at least 30 minutes using the provided USB-C cable.

**Problem: The screen is black or unresponsive.**

*   **Solution 1**: The device may be in deep sleep mode. Tap the screen firmly to wake it.
*   **Solution 2**: Perform a hard reset by pressing and holding the power button for 10 seconds.

**Problem: The Setup Wizard shows "SD Card: N/A" or "Touch: FAIL".**

*   **SD Card**: Ensure the microSD card is formatted as FAT32 and is inserted correctly. Try a different brand of SD card if the problem persists.
*   **Touch**: This may indicate a hardware issue. Please contact customer support.

**Problem: The Web Portal (`http://192.168.4.1`) won't load.**

*   **Solution 1**: Ensure you are connected to the "OUI-SPY-PRO" Wi-Fi network and have entered the correct password (`spypro2026`).
*   **Solution 2**: On your phone, disable your mobile data temporarily. Some phones will try to use mobile data if the Wi-Fi network doesn't have an internet connection.
*   **Solution 3**: Make sure the Web Portal is enabled in the on-device Settings (CFG) page.

**Problem: I can't download the log file from the web portal.**

*   **Solution**: Ensure an SD card is inserted and that the device has logged at least one detection. The `detections.csv` file is created only after the first detection is logged.

---

**Contact Support**

If you are still experiencing issues, please contact our support team at:
**support@your-company.com** (Replace with your actual support email)
