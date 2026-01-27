/*
 * UK-OUI-SPY ESP32 v6
 * Portable UK Surveillance Device Detector
 *
 * Hardware: ESP32-2432S028 (2.8" ILI9341 TFT, capacitive touch)
 * Features: BLE/WiFi scanning, OUI matching, proximity alerts, logging
 *
 * Note: This board uses FT6236 capacitive touch controller (I2C).
 *       For production use, implement proper capacitive touch handling.
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <SD.h>
#include <SPI.h>
#include <vector>
#include "oui_database.h"

// Pin definitions for ESP32-2432S028 (Two USB, Capacitive Touch version)
#define TFT_BL 27      // Backlight control - GPIO 27 for capacitive version!
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

// Detection structure - Enhanced for ML analysis
struct Detection {
    // Identification
    String macAddress;
    String oui;
    String manufacturer;
    DeviceCategory category;
    RelevanceLevel relevance;
    DeploymentType deployment;
    String notes;

    // Signal strength tracking
    int8_t rssi;           // Current RSSI
    int8_t rssiMin;        // Minimum RSSI seen
    int8_t rssiMax;        // Maximum RSSI seen
    float rssiAvg;         // Average RSSI

    // Temporal tracking
    unsigned long timestamp;     // Millis at detection
    unsigned long unixTime;      // Real Unix timestamp
    unsigned long firstSeen;     // First detection time
    unsigned long lastSeen;      // Last detection time
    int seenCount;               // Number of times detected

    // Connection type
    bool isBLE;

    // Calculated fields
    bool isStationary;     // Low RSSI variance = fixed infrastructure
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

// Session tracking for ML analysis
String sessionID = "";
unsigned long sessionStartTime = 0;
bool timeValid = false;

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
String generateSessionID();
void syncNTPTime();

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

    // Initialize TFT backlight (CRITICAL for ESP32-2432S028!)
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);  // Turn on backlight
    Serial.println("Backlight enabled");

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

    // Generate unique session ID for this boot/walk
    tft.setCursor(20, 210);
    tft.print("Session ID...");
    sessionID = generateSessionID();
    Serial.printf("Session ID: %s\n", sessionID.c_str());
    tft.print(" OK");

    // Sync time from NTP (requires WiFi)
    tft.setCursor(20, 230);
    tft.print("Syncing time...");
    syncNTPTime();
    if (timeValid) {
        tft.print(" OK");
    } else {
        tft.print(" SKIP");
    }

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

String generateSessionID() {
    // Generate unique session ID from ESP32 chip ID + boot time
    uint64_t chipid = ESP.getEfuseMac();
    unsigned long bootTime = millis();

    char sessionStr[32];
    snprintf(sessionStr, sizeof(sessionStr), "%04X%04X-%08lX",
             (uint16_t)(chipid >> 32), (uint16_t)chipid, bootTime);

    return String(sessionStr);
}

void syncNTPTime() {
    // Try to sync time from NTP server (requires WiFi)
    // Using GMT/UTC timezone
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    Serial.print("Syncing time from NTP...");
    struct tm timeinfo;
    int attempts = 0;
    while (!getLocalTime(&timeinfo) && attempts < 10) {
        Serial.print(".");
        delay(500);
        attempts++;
    }

    if (attempts < 10) {
        timeValid = true;
        sessionStartTime = time(nullptr);
        Serial.println(" OK!");
        Serial.printf("Current time: %s", asctime(&timeinfo));
    } else {
        timeValid = false;
        Serial.println(" FAILED");
        Serial.println("Will use relative timestamps");
    }
}

void initSDCard() {
    if (SD.begin(SD_CS)) {
        sdCardAvailable = true;
        Serial.println("SD Card initialized");
        tft.print(" OK");

        // Create log file header if new (Enhanced for ML analysis)
        File logFile = SD.open("/detections.csv", FILE_WRITE);
        if (logFile) {
            if (logFile.size() == 0) {
                logFile.println("SessionID,UnixTime,Timestamp,MAC,OUI,Manufacturer,Category,Relevance,Deployment,RSSI,RSSIMin,RSSIMax,RSSIAvg,SeenCount,FirstSeen,LastSeen,Duration,Type,IsStationary,Notes");
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
            // Identification
            det.macAddress = mac;
            det.oui = oui;
            det.manufacturer = UK_OUI_DATABASE[i].manufacturer;
            det.category = UK_OUI_DATABASE[i].category;
            det.relevance = UK_OUI_DATABASE[i].relevance;
            det.deployment = UK_OUI_DATABASE[i].deployment;
            det.notes = UK_OUI_DATABASE[i].notes;

            // Signal strength (will be updated if duplicate)
            det.rssi = rssi;
            det.rssiMin = rssi;
            det.rssiMax = rssi;
            det.rssiAvg = rssi;

            // Temporal (will be updated if duplicate)
            det.timestamp = millis();
            det.unixTime = timeValid ? time(nullptr) : 0;
            det.firstSeen = millis();
            det.lastSeen = millis();
            det.seenCount = 1;

            // Connection type
            det.isBLE = isBLE;

            // Calculated (will be determined over time)
            det.isStationary = false;

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
            // Update existing detection with enhanced tracking

            // Update RSSI statistics
            d.rssi = det.rssi;  // Current RSSI
            if (det.rssi < d.rssiMin) d.rssiMin = det.rssi;
            if (det.rssi > d.rssiMax) d.rssiMax = det.rssi;

            // Update average RSSI (weighted average)
            d.rssiAvg = ((d.rssiAvg * d.seenCount) + det.rssi) / (d.seenCount + 1);

            // Update temporal tracking
            d.seenCount++;
            d.lastSeen = now;
            d.timestamp = now;
            d.unixTime = timeValid ? time(nullptr) : 0;

            // Calculate if stationary (low RSSI variance = fixed infrastructure)
            int rssiVariance = d.rssiMax - d.rssiMin;
            d.isStationary = (rssiVariance < 15 && d.seenCount >= 3);

            // Log updated detection to SD card
            if (config.enableLogging && sdCardAvailable) {
                logDetectionToSD(d);
            }

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
        // Calculate duration in milliseconds
        unsigned long duration = det.lastSeen - det.firstSeen;

        // Enhanced ML-ready CSV format (20 columns)
        logFile.printf("%s,%lu,%lu,%s,%s,%s,%s,%s,%s,%d,%d,%d,%.1f,%d,%lu,%lu,%lu,%s,%s,%s\n",
            sessionID.c_str(),                    // SessionID
            det.unixTime,                         // UnixTime (real timestamp)
            det.timestamp,                        // Timestamp (millis since boot)
            det.macAddress.c_str(),               // MAC
            det.oui.c_str(),                      // OUI
            det.manufacturer.c_str(),             // Manufacturer
            getCategoryName(det.category),        // Category
            getRelevanceName(det.relevance),      // Relevance
            getDeploymentName(det.deployment),    // Deployment
            det.rssi,                             // RSSI (current)
            det.rssiMin,                          // RSSIMin
            det.rssiMax,                          // RSSIMax
            det.rssiAvg,                          // RSSIAvg (float with 1 decimal)
            det.seenCount,                        // SeenCount
            det.firstSeen,                        // FirstSeen
            det.lastSeen,                         // LastSeen
            duration,                             // Duration (calculated)
            det.isBLE ? "BLE" : "WiFi",          // Type
            det.isStationary ? "FIXED" : "MOBILE", // IsStationary
            det.notes.c_str()                     // Notes
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
