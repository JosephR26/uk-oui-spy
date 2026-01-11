/*
 * UK-OUI-SPY ESP32 v6
 * Portable UK Surveillance Device Detector
 *
 * Hardware: ESP32-2432S028R (2.8" ILI9341 touchscreen)
 * Features: BLE/WiFi scanning, OUI matching, proximity alerts, logging
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <SD.h>
#include <SPI.h>
#include <vector>
#include "oui_database.h"

// Pin definitions for ESP32-2432S028R
#define SD_CS 5
#define BUZZER_PIN 25  // Optional buzzer pin
#define LED_PIN 4      // Optional LED indicator

// Scan modes
enum ScanMode {
    SCAN_QUICK = 0,     // 1-2 seconds
    SCAN_NORMAL = 1,    // 5 seconds
    SCAN_POWER_SAVE = 2 // 10-15 seconds
};

// Alert modes
enum AlertMode {
    ALERT_SILENT = 0,
    ALERT_BUZZER = 1,
    ALERT_VIBRATE = 2
};

// Detection structure
struct Detection {
    String macAddress;
    String oui;
    String manufacturer;
    DeviceCategory category;
    RelevanceLevel relevance;
    DeploymentType deployment;
    String notes;
    int8_t rssi;
    unsigned long timestamp;
    bool isBLE;
};

// Configuration
struct Config {
    ScanMode scanMode = SCAN_NORMAL;
    AlertMode alertMode = ALERT_BUZZER;
    bool enableBLE = true;
    bool enableWiFi = true;
    bool enableLogging = true;
    int rssiThresholdHigh = -50;   // Very close
    int rssiThresholdMedium = -70; // Medium distance
    int rssiThresholdLow = -90;    // Far
};

// Global objects
TFT_eSPI tft = TFT_eSPI();
Config config;
std::vector<Detection> detections;
const int MAX_DETECTIONS = 50; // Circular buffer size
unsigned long lastScanTime = 0;
int scanInterval = 5000;
bool scanning = false;
int scrollOffset = 0;
bool sdCardAvailable = false;

// UI state
int currentScreen = 0; // 0=main, 1=settings, 2=logs
int selectedRelevanceFilter = -1; // -1=all, 0=low, 1=medium, 2=high

// Function prototypes
void initDisplay();
void initBLE();
void initWiFi();
void initSDCard();
void scanBLE();
void scanWiFi();
void checkOUI(String macAddress, int8_t rssi, bool isBLE);
void addDetection(Detection det);
void updateDisplay();
void drawMainScreen();
void drawSettingsScreen();
void drawDetection(Detection &det, int y, bool highlight);
void handleTouch();
void playProximityAlert(int8_t rssi, RelevanceLevel relevance);
void logDetectionToSD(Detection &det);
String formatMAC(String mac);
String extractOUI(String mac);

// BLE Scan callback
class MyAdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        String mac = advertisedDevice->getAddress().toString().c_str();
        int8_t rssi = advertisedDevice->getRSSI();
        checkOUI(mac, rssi, true);
    }
};

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nUK-OUI-SPY ESP32 v6");
    Serial.println("====================");

    // Initialize peripherals
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Initialize display
    initDisplay();
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 100);
    tft.println("UK-OUI-SPY v6");
    tft.setTextSize(1);
    tft.setCursor(20, 130);
    tft.println("Initializing...");

    // Initialize SD card
    tft.setCursor(20, 150);
    tft.print("SD Card...");
    initSDCard();

    // Initialize BLE
    tft.setCursor(20, 170);
    tft.print("BLE Scanner...");
    initBLE();

    // Initialize WiFi (for promiscuous mode scanning)
    tft.setCursor(20, 190);
    tft.print("WiFi Scanner...");
    initWiFi();

    delay(2000);

    Serial.println("\nOUI Database loaded:");
    Serial.printf("  %d UK surveillance device entries\n", UK_OUI_DATABASE_SIZE);
    Serial.println("\nStarting scan...");

    // Set scan interval based on mode
    switch(config.scanMode) {
        case SCAN_QUICK: scanInterval = 2000; break;
        case SCAN_NORMAL: scanInterval = 5000; break;
        case SCAN_POWER_SAVE: scanInterval = 15000; break;
    }

    updateDisplay();
}

void loop() {
    unsigned long currentTime = millis();

    // Periodic scanning
    if (currentTime - lastScanTime >= scanInterval) {
        scanning = true;
        digitalWrite(LED_PIN, HIGH);

        if (config.enableBLE) {
            scanBLE();
        }

        // WiFi promiscuous scanning would go here
        // (More complex - requires packet callback handling)

        scanning = false;
        digitalWrite(LED_PIN, LOW);
        lastScanTime = currentTime;
        updateDisplay();
    }

    // Handle touch input
    handleTouch();

    delay(50);
}

void initDisplay() {
    tft.init();
    tft.setRotation(1); // Landscape
    tft.fillScreen(TFT_BLACK);
    Serial.println("Display initialized (240x320)");
}

void initBLE() {
    NimBLEDevice::init("UK-OUI-SPY");
    Serial.println("BLE initialized");
}

void initWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    Serial.println("WiFi initialized (station mode)");
}

void initSDCard() {
    if (SD.begin(SD_CS)) {
        sdCardAvailable = true;
        Serial.println("SD Card initialized");
        tft.print(" OK");

        // Create log file header if new
        File logFile = SD.open("/detections.csv", FILE_WRITE);
        if (logFile) {
            if (logFile.size() == 0) {
                logFile.println("Timestamp,MAC,OUI,Manufacturer,Category,Relevance,Deployment,RSSI,Type,Notes");
            }
            logFile.close();
        }
    } else {
        sdCardAvailable = false;
        Serial.println("SD Card not found");
        tft.print(" FAIL");
    }
}

void scanBLE() {
    NimBLEScan* pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);

    int scanTime = 1; // seconds
    if (config.scanMode == SCAN_NORMAL) scanTime = 3;
    if (config.scanMode == SCAN_POWER_SAVE) scanTime = 5;

    pBLEScan->start(scanTime, false);
    pBLEScan->clearResults();
}

void scanWiFi() {
    // WiFi scanning for AP beacons
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
        String bssid = WiFi.BSSIDstr(i);
        int8_t rssi = WiFi.RSSI(i);
        checkOUI(bssid, rssi, false);
    }
    WiFi.scanDelete();
}

void checkOUI(String macAddress, int8_t rssi, bool isBLE) {
    String mac = formatMAC(macAddress);
    String oui = extractOUI(mac);

    // Check against UK database
    for (int i = 0; i < UK_OUI_DATABASE_SIZE; i++) {
        if (oui.equalsIgnoreCase(UK_OUI_DATABASE[i].oui)) {
            // Match found!
            Detection det;
            det.macAddress = mac;
            det.oui = oui;
            det.manufacturer = UK_OUI_DATABASE[i].manufacturer;
            det.category = UK_OUI_DATABASE[i].category;
            det.relevance = UK_OUI_DATABASE[i].relevance;
            det.deployment = UK_OUI_DATABASE[i].deployment;
            det.notes = UK_OUI_DATABASE[i].notes;
            det.rssi = rssi;
            det.timestamp = millis();
            det.isBLE = isBLE;

            addDetection(det);

            // Trigger proximity alert
            playProximityAlert(rssi, det.relevance);

            // Log to SD
            if (config.enableLogging && sdCardAvailable) {
                logDetectionToSD(det);
            }

            Serial.printf("[DETECT] %s | %s | %s | RSSI:%d | %s\n",
                mac.c_str(),
                det.manufacturer.c_str(),
                getCategoryName(det.category),
                rssi,
                getRelevanceName(det.relevance));

            break;
        }
    }
}

void addDetection(Detection det) {
    // Check for duplicates (same MAC within last 30 seconds)
    unsigned long now = millis();
    for (auto &d : detections) {
        if (d.macAddress == det.macAddress && (now - d.timestamp) < 30000) {
            // Update existing detection
            d.rssi = det.rssi;
            d.timestamp = now;
            return;
        }
    }

    // Add new detection
    detections.insert(detections.begin(), det);

    // Maintain circular buffer
    if (detections.size() > MAX_DETECTIONS) {
        detections.pop_back();
    }
}

void updateDisplay() {
    if (currentScreen == 0) {
        drawMainScreen();
    } else if (currentScreen == 1) {
        drawSettingsScreen();
    }
}

void drawMainScreen() {
    tft.fillScreen(TFT_BLACK);

    // Header
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(5, 5);
    tft.print("UK-OUI-SPY");

    // Status indicators
    tft.setTextSize(1);
    tft.setCursor(200, 8);
    if (scanning) {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.print("SCAN");
    } else {
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.print("IDLE");
    }

    tft.setCursor(250, 8);
    if (sdCardAvailable) {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.print("SD");
    }

    tft.setCursor(280, 8);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.printf("%d", detections.size());

    // Scan mode
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(5, 20);
    tft.printf("Mode: %s",
        config.scanMode == SCAN_QUICK ? "QUICK" :
        config.scanMode == SCAN_NORMAL ? "NORMAL" : "POWER-SAVE");

    // Divider
    tft.drawLine(0, 30, 320, 30, TFT_DARKGREY);

    // Detection list
    int y = 35;
    int displayCount = 0;

    if (detections.empty()) {
        tft.setTextSize(2);
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.setCursor(50, 100);
        tft.print("No detections");
        tft.setTextSize(1);
        tft.setCursor(40, 130);
        tft.print("Scanning for devices...");
    } else {
        for (auto &det : detections) {
            // Apply relevance filter
            if (selectedRelevanceFilter >= 0 && det.relevance != selectedRelevanceFilter) {
                continue;
            }

            if (displayCount >= scrollOffset && y < 230) {
                drawDetection(det, y, false);
                y += 42;
            }
            displayCount++;
        }
    }

    // Footer
    tft.drawLine(0, 230, 320, 230, TFT_DARKGREY);
    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setCursor(5, 235);
    tft.printf("Next scan: %ds", (scanInterval - (millis() - lastScanTime)) / 1000);
}

void drawDetection(Detection &det, int y, bool highlight) {
    uint16_t bgColor = highlight ? TFT_DARKGREY : TFT_BLACK;
    uint16_t relColor = getRelevanceColor(det.relevance);
    uint16_t catColor = getCategoryColor(det.category);

    // Relevance bar
    tft.fillRect(0, y, 5, 40, relColor);

    // Manufacturer
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, bgColor);
    tft.fillRect(5, y, 315, 10, bgColor);
    tft.setCursor(8, y + 1);
    tft.print(det.manufacturer);

    // Category and RSSI
    tft.setTextColor(catColor, bgColor);
    tft.fillRect(5, y + 11, 315, 10, bgColor);
    tft.setCursor(8, y + 12);
    tft.printf("%s | RSSI:%d", getCategoryName(det.category), det.rssi);

    // MAC and Type
    tft.setTextColor(TFT_CYAN, bgColor);
    tft.fillRect(5, y + 22, 315, 9, bgColor);
    tft.setCursor(8, y + 23);
    tft.printf("%s [%s]", det.macAddress.c_str(), det.isBLE ? "BLE" : "WiFi");

    // Notes
    tft.setTextColor(TFT_LIGHTGREY, bgColor);
    tft.fillRect(5, y + 31, 315, 9, bgColor);
    tft.setCursor(8, y + 32);
    tft.print(det.notes);

    // Border
    tft.drawLine(5, y + 40, 320, y + 40, TFT_DARKGREY);
}

void drawSettingsScreen() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(70, 100);
    tft.print("SETTINGS");
    tft.setTextSize(1);
    tft.setCursor(50, 130);
    tft.print("(Not yet implemented)");
}

void handleTouch() {
    uint16_t x = 0, y = 0;
    bool pressed = tft.getTouch(&x, &y);

    if (pressed) {
        // Simple touch handling - cycle scan mode
        if (y < 30) {
            // Header area - cycle scan mode
            config.scanMode = (ScanMode)((config.scanMode + 1) % 3);
            switch(config.scanMode) {
                case SCAN_QUICK: scanInterval = 2000; break;
                case SCAN_NORMAL: scanInterval = 5000; break;
                case SCAN_POWER_SAVE: scanInterval = 15000; break;
            }
            updateDisplay();
            delay(300); // Debounce
        }
    }
}

void playProximityAlert(int8_t rssi, RelevanceLevel relevance) {
    if (config.alertMode == ALERT_SILENT) return;

    // Only alert for medium/high relevance
    if (relevance == REL_LOW) return;

    // Calculate beep pattern based on RSSI
    int beepDuration = 50;
    int beepCount = 1;

    if (rssi > config.rssiThresholdHigh) {
        // Very close - rapid beeps
        beepCount = 3;
        beepDuration = 100;
    } else if (rssi > config.rssiThresholdMedium) {
        // Medium distance - double beep
        beepCount = 2;
        beepDuration = 75;
    }

    // High relevance = higher pitch
    int frequency = (relevance == REL_HIGH) ? 2000 : 1500;

    for (int i = 0; i < beepCount; i++) {
        tone(BUZZER_PIN, frequency, beepDuration);
        delay(beepDuration + 50);
    }
}

void logDetectionToSD(Detection &det) {
    File logFile = SD.open("/detections.csv", FILE_APPEND);
    if (logFile) {
        logFile.printf("%lu,%s,%s,%s,%s,%s,%s,%d,%s,%s\n",
            det.timestamp,
            det.macAddress.c_str(),
            det.oui.c_str(),
            det.manufacturer.c_str(),
            getCategoryName(det.category),
            getRelevanceName(det.relevance),
            getDeploymentName(det.deployment),
            det.rssi,
            det.isBLE ? "BLE" : "WiFi",
            det.notes.c_str()
        );
        logFile.close();
    }
}

String formatMAC(String mac) {
    // Ensure consistent MAC format (XX:XX:XX:XX:XX:XX)
    mac.toUpperCase();
    mac.replace("-", ":");
    return mac;
}

String extractOUI(String mac) {
    // Extract first 3 bytes (OUI)
    if (mac.length() < 8) return "";
    return mac.substring(0, 8); // XX:XX:XX
}
