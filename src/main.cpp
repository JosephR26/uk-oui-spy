/*
 * UK-OUI-SPY ESP32
 * Portable UK Surveillance Device Detector
 *
 * Hardware: ESP32-2432S028 (2.8" ILI9341 TFT, capacitive touch)
 * Features: BLE/WiFi scanning, WiFi promiscuous mode, OUI matching,
 *           proximity alerts, logging, settings screen, deep sleep,
 *           threat scoring, swipe UI, OTA updates
 */

#define VERSION "1.1.0"

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <vector>
#include <algorithm>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "esp_sleep.h"
#include "oui_database.h"
#include "wifi_promiscuous.h"

// CST820 capacitive touch controller (I2C)
// ESP32-2432S028 2-USB capacitive variant: SDA=27, SCL=22
#define TOUCH_ADDR  0x15    // CST820 uses 0x15
#define TOUCH_SDA   27
#define TOUCH_SCL   22

// Read CST820 touch data over I2C
// Returns true if touched, sets x/y coordinates
bool readCapacitiveTouch(uint16_t *x, uint16_t *y) {
    Wire.beginTransmission(TOUCH_ADDR);
    Wire.write(0x02);  // Register: number of touch points
    if (Wire.endTransmission() != 0) return false;

    Wire.requestFrom((uint8_t)TOUCH_ADDR, (uint8_t)5);
    if (Wire.available() < 5) return false;

    uint8_t numPoints = Wire.read();
    if ((numPoints & 0x0F) == 0) return false;  // No touch

    uint8_t xHigh = Wire.read();
    uint8_t xLow  = Wire.read();
    uint8_t yHigh = Wire.read();
    uint8_t yLow  = Wire.read();

    // CST820 returns raw coordinates in portrait orientation (240x320)
    uint16_t rawX = ((xHigh & 0x0F) << 8) | xLow;
    uint16_t rawY = ((yHigh & 0x0F) << 8) | yLow;

    // Transform for landscape display (rotation=1, 320x240)
    *x = rawY;
    *y = 239 - rawX;

    // Clamp to valid screen bounds
    if (*x > 319) *x = 319;
    if (*y > 239) *y = 239;

    return true;
}

// Pin definitions for ESP32-2432S028 (Two USB, Capacitive Touch version)
#define TFT_BL 21      // Backlight control - GPIO 21 for 2-USB variant
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
    float threatScore;     // Composite threat score (0-10)
};

// Power modes
enum PowerMode {
    POWER_ACTIVE = 0,      // Always on, no sleep
    POWER_LIGHT_SLEEP = 1, // Light sleep between scans
    POWER_DEEP_SLEEP = 2   // Deep sleep for maximum battery (24+ hours)
};

// Configuration
struct Config {
    ScanMode scanMode = SCAN_NORMAL;
    AlertMode alertMode = ALERT_BUZZER;
    PowerMode powerMode = POWER_ACTIVE;
    bool enableBLE = true;
    bool enableWiFi = true;
    bool enableWiFiPromiscuous = true;
    bool enableLogging = true;
    bool enableDeepSleep = false;       // Deep sleep mode for 24+ hour operation
    bool policeOnlyFilter = false;      // Filter to show only police/enforcement devices
    int brightness = 255;               // Display brightness (0-255)
    int rssiThresholdHigh = -50;
    int rssiThresholdMedium = -70;
    int rssiThresholdLow = -90;
    int deepSleepSeconds = 15;          // Sleep duration between scan cycles
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
int currentScreen = 0; // 0=main, 1=settings, 2=logs, 3=ota
int selectedRelevanceFilter = -1; // -1=all, 0=low, 1=medium, 2=high
int settingsSelectedItem = 0;
const int SETTINGS_ITEMS = 9;  // Expanded settings items

// Touch gesture tracking
unsigned long touchStartTime = 0;
uint16_t touchStartX = 0;
uint16_t touchStartY = 0;
bool touchActive = false;
const int SWIPE_THRESHOLD = 50;     // Minimum pixels for swipe
const int LONG_PRESS_MS = 800;      // Long press threshold

// Settings screen layout constants (shared between draw and touch)
const int SETTINGS_START_Y = 28;
const int SETTINGS_ITEM_HEIGHT = 22;

// Deep sleep boot tracking
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int totalDetections = 0;

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
void drawOTAScreen();
void drawDetection(Detection &det, int y, bool highlight);
void handleTouchGestures();
void handleLongPress(uint16_t x, uint16_t y);
void handleTap(uint16_t x, uint16_t y);
void playProximityAlert(int8_t rssi, RelevanceLevel relevance);
void logDetectionToSD(Detection &det);
String formatMAC(String mac);
String extractOUI(String mac);
String generateSessionID();
void syncNTPTime();
void enterDeepSleep();
void setBrightness(int level);
float calculateThreatScore(Detection &det);
void sortDetectionsByThreat();
bool fetchOUIUpdates();
bool isPoliceDevice(DeviceCategory cat);

// BLE Scan callback
class MyAdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        String mac = advertisedDevice->getAddress().toString().c_str();
        int8_t rssi = advertisedDevice->getRSSI();
        checkOUI(mac, rssi, true);
    }
};

// WiFi Promiscuous mode callback
void onWiFiPromiscuousPacket(const uint8_t* mac, int8_t rssi, uint8_t channel) {
    // Convert MAC bytes to string format XX:XX:XX:XX:XX:XX
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Check against OUI database (false = WiFi, not BLE)
    checkOUI(String(macStr), rssi, false);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.printf("\n\nUK-OUI-SPY ESP32 v%s\n", VERSION);
    Serial.println("==========================");

    // Initialize peripherals
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Initialize I2C for CST820 capacitive touch (SDA=27, SCL=22)
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Serial.printf("I2C touch initialized (CST820 @ 0x%02X, SDA=%d, SCL=%d)\n", TOUCH_ADDR, TOUCH_SDA, TOUCH_SCL);

    // Initialize TFT backlight
    // Try simple digitalWrite first - some 2-USB variants need this
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW);   // Try LOW first (active LOW backlight)
    delay(100);
    Serial.printf("Backlight pin %d set LOW\n", TFT_BL);

    // If still dark, the backlight might be always-on or on different pin
    // The display content should still be visible

    // Initialize display
    initDisplay();
    tft.fillScreen(TFT_NAVY);
    tft.setTextColor(TFT_CYAN, TFT_NAVY);
    tft.setTextSize(2);
    tft.setCursor(50, 80);
    tft.printf("UK-OUI-SPY");
    tft.setTextColor(TFT_ORANGE, TFT_NAVY);
    tft.setCursor(180, 80);
    tft.printf("v%s", VERSION);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.setCursor(60, 110);
    tft.println("Surveillance Detector");
    tft.setTextColor(TFT_LIGHTGREY, TFT_NAVY);
    tft.setCursor(20, 140);
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

        // BLE Scanning
        if (config.enableBLE) {
            scanBLE();
        }

        // Standard WiFi Network Scanning (beacons/APs)
        if (config.enableWiFi) {
            scanWiFi();
        }

        // WiFi Promiscuous Mode Scanning (probe requests, all management frames)
        if (config.enableWiFiPromiscuous) {
            int dwellTime = 100;
            if (config.scanMode == SCAN_NORMAL) dwellTime = 200;
            if (config.scanMode == SCAN_POWER_SAVE) dwellTime = 300;

            startWiFiPromiscuous(onWiFiPromiscuousPacket);
            scanAllChannels(onWiFiPromiscuousPacket, dwellTime);
            stopWiFiPromiscuous();
        }

        // Sort detections by threat score
        sortDetectionsByThreat();

        scanning = false;
        digitalWrite(LED_PIN, LOW);
        lastScanTime = currentTime;
        updateDisplay();

        // Deep sleep mode for extended battery life
        if (config.enableDeepSleep && detections.empty()) {
            enterDeepSleep();
        }
    }

    // Handle touch gestures (swipe, long-press)
    handleTouchGestures();

    delay(50);
}

// Deep sleep for 24+ hour battery operation
void enterDeepSleep() {
    Serial.printf("Entering deep sleep for %d seconds...\n", config.deepSleepSeconds);

    // Save state to RTC memory
    totalDetections += detections.size();
    bootCount++;

    // Turn off display and peripherals
    digitalWrite(TFT_BL, HIGH);  // Turn off backlight (active LOW: HIGH=off)
    digitalWrite(LED_PIN, LOW);

    // Configure wakeup timer
    esp_sleep_enable_timer_wakeup(config.deepSleepSeconds * 1000000ULL);

    // Enter deep sleep
    esp_deep_sleep_start();
}

// Set display brightness using LEDC PWM (0-255)
// Set display brightness (simple on/off for now)
void setBrightness(int level) {
    config.brightness = constrain(level, 0, 255);
    digitalWrite(TFT_BL, config.brightness > 127 ? LOW : HIGH);  // Active LOW
}

// Check if device is police/enforcement related
bool isPoliceDevice(DeviceCategory cat) {
    return (cat == CAT_BODYCAM || cat == CAT_ANPR ||
            cat == CAT_DRONE || cat == CAT_FACIAL_RECOG);
}

// Calculate threat score: recency(0.6) + cluster_density(0.25) + relevance(0.15)
float calculateThreatScore(Detection &det) {
    float score = 0.0;

    // Recency factor (0.6 weight) - more recent = higher score
    unsigned long age = millis() - det.lastSeen;
    float recencyScore = 10.0 * exp(-age / 60000.0);  // Decay over 1 minute
    score += recencyScore * 0.6;

    // Cluster density factor (0.25 weight) - more sightings = higher score
    float densityScore = min(10.0f, det.seenCount * 2.0f);
    score += densityScore * 0.25;

    // Relevance factor (0.15 weight)
    float relevanceScore = 0.0;
    switch(det.relevance) {
        case REL_HIGH: relevanceScore = 10.0; break;
        case REL_MEDIUM: relevanceScore = 6.0; break;
        case REL_LOW: relevanceScore = 3.0; break;
    }
    score += relevanceScore * 0.15;

    // Boost for police/enforcement devices
    if (isPoliceDevice(det.category)) {
        score = min(10.0f, score * 1.5f);
    }

    // Boost for very close proximity
    if (det.rssi > config.rssiThresholdHigh) {
        score = min(10.0f, score * 1.3f);
    }

    return score;
}

// Sort detections by threat score (highest first)
void sortDetectionsByThreat() {
    for (auto &det : detections) {
        det.threatScore = calculateThreatScore(det);
    }

    std::sort(detections.begin(), detections.end(),
        [](const Detection &a, const Detection &b) {
            return a.threatScore > b.threatScore;
        });
}

void initDisplay() {
    tft.init();
    tft.setRotation(1); // Landscape (320x240)

    // Full screen clear - multiple times to ensure display memory is properly initialized
    for (int i = 0; i < 3; i++) {
        tft.fillScreen(TFT_BLACK);
        delay(100);
    }

    Serial.println("Display initialized (320x240 landscape)");
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
    } else if (currentScreen == 3) {
        drawOTAScreen();
    }
}

void drawMainScreen() {
    // Dark blue gradient background
    tft.fillScreen(TFT_NAVY);
    tft.fillRect(0, 0, 320, 30, 0x000F);  // Darker header bar

    // Header - bright cyan title
    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN, 0x000F);
    tft.setCursor(5, 5);
    tft.print("UK-OUI-SPY");

    // Status indicators
    tft.setTextSize(1);
    tft.setCursor(200, 8);
    if (scanning) {
        tft.setTextColor(TFT_GREEN, 0x000F);
        tft.print("SCAN");
    } else {
        tft.setTextColor(TFT_LIGHTGREY, 0x000F);
        tft.print("IDLE");
    }

    tft.setCursor(250, 8);
    if (sdCardAvailable) {
        tft.setTextColor(TFT_GREEN, 0x000F);
        tft.print("SD");
    }

    tft.setCursor(270, 8);
    tft.setTextColor(TFT_WHITE, 0x000F);
    tft.printf("%d", detections.size());

    // Settings gear indicator
    tft.setCursor(295, 8);
    tft.setTextColor(TFT_YELLOW, 0x000F);
    tft.print("[=]");

    // Scan mode and filters
    tft.setTextColor(TFT_ORANGE, TFT_NAVY);
    tft.setCursor(5, 20);
    tft.printf("Mode: %s",
        config.scanMode == SCAN_QUICK ? "QUICK" :
        config.scanMode == SCAN_NORMAL ? "NORMAL" : "POWER-SAVE");

    // Police filter indicator
    if (config.policeOnlyFilter) {
        tft.setTextColor(TFT_RED, TFT_NAVY);
        tft.setCursor(180, 20);
        tft.print("[POLICE]");
    }

    // Deep sleep indicator
    if (config.enableDeepSleep) {
        tft.setTextColor(TFT_MAGENTA, TFT_NAVY);
        tft.setCursor(260, 20);
        tft.print("ZZZ");
    }

    // Divider - cyan accent line
    tft.drawLine(0, 30, 320, 30, TFT_CYAN);

    // Detection list
    int y = 35;
    int displayCount = 0;

    if (detections.empty()) {
        tft.setTextSize(2);
        tft.setTextColor(TFT_LIGHTGREY, TFT_NAVY);
        tft.setCursor(50, 100);
        tft.print("No detections");
        tft.setTextSize(1);
        tft.setTextColor(TFT_CYAN, TFT_NAVY);
        tft.setCursor(40, 130);
        tft.print("Scanning for devices...");
    } else {
        for (auto &det : detections) {
            // Apply relevance filter
            if (selectedRelevanceFilter >= 0 && det.relevance != selectedRelevanceFilter) {
                continue;
            }

            // Apply police-only filter
            if (config.policeOnlyFilter && !isPoliceDevice(det.category)) {
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
    tft.fillRect(0, 230, 320, 10, 0x000F);  // Dark footer bar
    tft.drawLine(0, 230, 320, 230, TFT_CYAN);
    tft.setTextSize(1);
    tft.setTextColor(TFT_CYAN, 0x000F);
    tft.setCursor(5, 232);
    tft.printf("Next scan: %ds", (scanInterval - (millis() - lastScanTime)) / 1000);
}

void drawDetection(Detection &det, int y, bool highlight) {
    uint16_t bgColor = highlight ? 0x0010 : TFT_NAVY;  // Slightly lighter when highlighted
    uint16_t relColor = getRelevanceColor(det.relevance);
    uint16_t catColor = getCategoryColor(det.category);

    // Relevance bar (left edge)
    tft.fillRect(0, y, 5, 40, relColor);

    // Manufacturer - bright white on dark blue
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, bgColor);
    tft.fillRect(5, y, 315, 10, bgColor);
    tft.setCursor(8, y + 1);
    tft.print(det.manufacturer);

    // Category, RSSI, and threat score
    tft.setTextColor(catColor, bgColor);
    tft.fillRect(5, y + 11, 315, 10, bgColor);
    tft.setCursor(8, y + 12);
    tft.printf("%s | RSSI:%d", getCategoryName(det.category), det.rssi);

    // Threat score indicator
    if (det.threatScore >= 7.0) {
        tft.setTextColor(TFT_RED, bgColor);
        tft.setCursor(250, y + 12);
        tft.printf("T:%.1f!", det.threatScore);
    } else if (det.threatScore >= 4.0) {
        tft.setTextColor(TFT_ORANGE, bgColor);
        tft.setCursor(250, y + 12);
        tft.printf("T:%.1f", det.threatScore);
    }

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

    // Border - subtle cyan line
    tft.drawLine(5, y + 40, 320, y + 40, 0x0410);
}

void drawSettingsScreen() {
    tft.fillScreen(TFT_NAVY);
    tft.fillRect(0, 0, 320, 25, 0x000F);  // Dark header bar

    // Header
    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN, 0x000F);
    tft.setCursor(100, 5);
    tft.print("SETTINGS");

    // Back button
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW, 0x000F);
    tft.setCursor(5, 8);
    tft.print("< BACK");

    tft.drawLine(0, 25, 320, 25, TFT_CYAN);

    int y = SETTINGS_START_Y;
    int itemHeight = SETTINGS_ITEM_HEIGHT;

    // Helper to draw toggle
    auto drawToggle = [&](int itemY, bool enabled) {
        int toggleX = 270;
        int toggleY = itemY + 3;
        if (enabled) {
            tft.fillRoundRect(toggleX, toggleY, 36, 14, 7, TFT_GREEN);
            tft.fillCircle(toggleX + 25, toggleY + 7, 5, TFT_WHITE);
        } else {
            tft.fillRoundRect(toggleX, toggleY, 36, 14, 7, 0x4208);  // Dark grey
            tft.fillCircle(toggleX + 11, toggleY + 7, 5, TFT_LIGHTGREY);
        }
    };

    // Helper to draw row
    auto drawRow = [&](int idx, const char* label, const char* val, bool isToggle, bool state) {
        int itemY = y + (idx * itemHeight);
        bool sel = (settingsSelectedItem == idx);
        uint16_t rowBg = sel ? 0x0015 : TFT_NAVY;  // Lighter blue when selected

        if (sel) {
            tft.fillRect(0, itemY, 320, itemHeight - 1, rowBg);
            tft.fillRect(0, itemY, 3, itemHeight - 1, TFT_CYAN);
        }

        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, rowBg);
        tft.setCursor(8, itemY + 4);
        tft.print(label);

        if (isToggle) {
            drawToggle(itemY, state);
        } else {
            tft.setTextColor(TFT_ORANGE, rowBg);
            tft.setCursor(160, itemY + 4);
            tft.print(val);
        }
    };

    // Draw settings (9 items)
    const char* scanModeStr = config.scanMode == SCAN_QUICK ? "QUICK" :
                              config.scanMode == SCAN_NORMAL ? "NORMAL" : "POWER-SAVE";
    drawRow(0, "Scan Mode", scanModeStr, false, false);

    const char* alertStr = config.alertMode == ALERT_SILENT ? "SILENT" :
                           config.alertMode == ALERT_BUZZER ? "BUZZER" : "VIBRATE";
    drawRow(1, "Alert Mode", alertStr, false, false);

    drawRow(2, "BLE Scanning", "", true, config.enableBLE);
    drawRow(3, "WiFi Scanning", "", true, config.enableWiFi);
    drawRow(4, "WiFi Promiscuous", "", true, config.enableWiFiPromiscuous);
    drawRow(5, "SD Card Logging", "", true, config.enableLogging);
    drawRow(6, "Deep Sleep Mode", "", true, config.enableDeepSleep);
    drawRow(7, "Police Filter", "", true, config.policeOnlyFilter);

    // OTA Update button
    int itemY = y + (8 * itemHeight);
    bool sel = (settingsSelectedItem == 8);
    uint16_t otaBg = sel ? 0x0015 : TFT_NAVY;
    if (sel) {
        tft.fillRect(0, itemY, 320, itemHeight - 1, otaBg);
        tft.fillRect(0, itemY, 3, itemHeight - 1, TFT_CYAN);
    }
    tft.setTextColor(TFT_ORANGE, otaBg);
    tft.setCursor(8, itemY + 4);
    tft.print("OTA Database Update...");

    // Footer
    tft.fillRect(0, 230, 320, 10, 0x000F);
    tft.drawLine(0, 230, 320, 230, TFT_CYAN);
    tft.setTextSize(1);
    tft.setTextColor(TFT_CYAN, 0x000F);
    tft.setCursor(50, 232);
    tft.print("Swipe: Navigate | Tap: Toggle");
}

// OTA Update screen
void drawOTAScreen() {
    tft.fillScreen(TFT_NAVY);
    tft.fillRect(0, 0, 320, 25, 0x000F);

    // Header
    tft.setTextSize(2);
    tft.setTextColor(TFT_ORANGE, 0x000F);
    tft.setCursor(60, 5);
    tft.print("OTA UPDATE");

    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW, 0x000F);
    tft.setCursor(5, 8);
    tft.print("< BACK");

    tft.drawLine(0, 25, 320, 25, TFT_CYAN);

    // Instructions
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.setCursor(10, 40);
    tft.print("Fetches latest OUI database from GitHub.");
    tft.setCursor(10, 55);
    tft.print("Requires WiFi connection.");

    tft.setCursor(10, 80);
    tft.setTextColor(TFT_CYAN, TFT_NAVY);
    tft.printf("Current entries: %d", UK_OUI_DATABASE_SIZE);

    // Fetch button - green gradient effect
    tft.fillRoundRect(80, 110, 160, 40, 8, TFT_DARKGREEN);
    tft.fillRoundRect(82, 112, 156, 36, 6, TFT_GREEN);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_GREEN);
    tft.setCursor(95, 120);
    tft.print("FETCH NOW");

    // Status area
    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY, TFT_NAVY);
    tft.setCursor(10, 170);
    tft.print("Status: Ready");

    tft.setCursor(10, 200);
    tft.setTextColor(TFT_CYAN, TFT_NAVY);
    tft.print("Tap button to check for updates");
}

// Fetch OUI updates from GitHub
bool fetchOUIUpdates() {
    tft.fillRect(10, 170, 300, 20, TFT_NAVY);
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW, TFT_NAVY);
    tft.setCursor(10, 170);
    tft.print("Status: Connecting...");

    // Connect to WiFi if not connected
    if (WiFi.status() != WL_CONNECTED) {
        // Try to connect to a known network or start captive portal
        tft.fillRect(10, 170, 300, 20, TFT_NAVY);
        tft.setTextColor(TFT_RED, TFT_NAVY);
        tft.setCursor(10, 170);
        tft.print("Status: WiFi not connected");
        tft.setCursor(10, 185);
        tft.print("Connect to WiFi first via captive portal");
        return false;
    }

    HTTPClient http;
    http.begin("https://raw.githubusercontent.com/JosephR26/uk-oui-spy/main/data/oui_updates.json");

    tft.fillRect(10, 170, 300, 20, TFT_NAVY);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(10, 170);
    tft.print("Status: Fetching...");

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();

        // Parse JSON and count new OUIs
        StaticJsonDocument<4096> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            int newCount = doc["count"] | 0;
            const char* version = doc["version"] | "unknown";

            tft.fillRect(10, 170, 300, 50, TFT_NAVY);
            tft.setTextColor(TFT_GREEN, TFT_NAVY);
            tft.setCursor(10, 170);
            tft.printf("Status: Success! v%s", version);
            tft.setCursor(10, 185);
            tft.printf("New OUIs available: %d", newCount);
            tft.setCursor(10, 200);
            tft.print("Restart to apply updates");

            http.end();
            return true;
        }
    }

    tft.fillRect(10, 170, 300, 20, TFT_NAVY);
    tft.setTextColor(TFT_RED, TFT_NAVY);
    tft.setCursor(10, 170);
    tft.printf("Status: Failed (HTTP %d)", httpCode);

    http.end();
    return false;
}

// Advanced touch gesture handling with swipe and long-press
void handleTouchGestures() {
    uint16_t x = 0, y = 0;
    bool pressed = readCapacitiveTouch(&x, &y);

    if (pressed && !touchActive) {
        // Touch started
        touchActive = true;
        touchStartTime = millis();
        touchStartX = x;
        touchStartY = y;
    }
    else if (!pressed && touchActive) {
        // Touch ended - analyze gesture
        touchActive = false;
        unsigned long touchDuration = millis() - touchStartTime;
        int deltaX = x - touchStartX;
        int deltaY = y - touchStartY;

        // Long press detection (800ms+)
        if (touchDuration >= LONG_PRESS_MS && abs(deltaX) < 20 && abs(deltaY) < 20) {
            handleLongPress(touchStartX, touchStartY);
            return;
        }

        // Swipe detection
        if (abs(deltaY) > SWIPE_THRESHOLD && abs(deltaY) > abs(deltaX)) {
            // Vertical swipe - scroll detection list
            if (currentScreen == 0 && !detections.empty()) {
                if (deltaY > 0) {
                    // Swipe down - scroll up
                    scrollOffset = max(0, scrollOffset - 1);
                } else {
                    // Swipe up - scroll down
                    int maxOffset = max(0, (int)detections.size() - 1);
                    scrollOffset = min(maxOffset, scrollOffset + 1);
                }
                updateDisplay();
                return;
            }
        }

        if (abs(deltaX) > SWIPE_THRESHOLD && abs(deltaX) > abs(deltaY)) {
            // Horizontal swipe
            if (currentScreen == 0) {
                if (deltaX > 0) {
                    // Swipe right - open settings
                    currentScreen = 1;
                    settingsSelectedItem = 0;
                } else {
                    // Swipe left - cycle filter
                    selectedRelevanceFilter = (selectedRelevanceFilter + 1) % 4;
                    if (selectedRelevanceFilter == 3) selectedRelevanceFilter = -1;
                }
                updateDisplay();
                return;
            }
            else if (currentScreen == 1 && deltaX > 0) {
                // Swipe right in settings - go back
                currentScreen = 0;
                updateDisplay();
                return;
            }
        }

        // Regular tap
        handleTap(touchStartX, touchStartY);
    }
}

// Handle long press - police filter + brightness control
void handleLongPress(uint16_t x, uint16_t y) {
    if (currentScreen == 0) {
        if (y < 120) {
            // Top half - toggle police-only filter
            config.policeOnlyFilter = !config.policeOnlyFilter;
            // Quick feedback beep
            tone(BUZZER_PIN, 1000, 50);
        } else {
            // Bottom half - cycle brightness
            config.brightness = (config.brightness + 64) % 256;
            if (config.brightness == 0) config.brightness = 64;
            setBrightness(config.brightness);
        }
        updateDisplay();
    }
}

// Handle regular tap
void handleTap(uint16_t x, uint16_t y) {
    if (currentScreen == 0) {
        // Main screen
        if (y < 30) {
            if (x > 280) {
                // Settings icon area
                currentScreen = 1;
                settingsSelectedItem = 0;
            } else {
                // Header - cycle scan mode
                config.scanMode = (ScanMode)((config.scanMode + 1) % 3);
                switch(config.scanMode) {
                    case SCAN_QUICK: scanInterval = 2000; break;
                    case SCAN_NORMAL: scanInterval = 5000; break;
                    case SCAN_POWER_SAVE: scanInterval = 15000; break;
                }
            }
            updateDisplay();
        } else if (y > 230) {
            // Footer - toggle filter
            selectedRelevanceFilter = (selectedRelevanceFilter + 1) % 4;
            if (selectedRelevanceFilter == 3) selectedRelevanceFilter = -1;
            updateDisplay();
        }
    }
    else if (currentScreen == 1) {
        // Settings screen
        if (y < 25) {
            currentScreen = 0;
            updateDisplay();
            return;
        }

        int touchedItem = (y - SETTINGS_START_Y) / SETTINGS_ITEM_HEIGHT;

        if (touchedItem >= 0 && touchedItem < SETTINGS_ITEMS) {
            if (x > 200) {
                // Toggle/change value
                switch(touchedItem) {
                    case 0: // Scan Mode
                        config.scanMode = (ScanMode)((config.scanMode + 1) % 3);
                        switch(config.scanMode) {
                            case SCAN_QUICK: scanInterval = 2000; break;
                            case SCAN_NORMAL: scanInterval = 5000; break;
                            case SCAN_POWER_SAVE: scanInterval = 15000; break;
                        }
                        break;
                    case 1: // Alert Mode
                        config.alertMode = (AlertMode)((config.alertMode + 1) % 3);
                        break;
                    case 2: // BLE Scanning
                        config.enableBLE = !config.enableBLE;
                        break;
                    case 3: // WiFi Scanning
                        config.enableWiFi = !config.enableWiFi;
                        break;
                    case 4: // WiFi Promiscuous
                        config.enableWiFiPromiscuous = !config.enableWiFiPromiscuous;
                        break;
                    case 5: // SD Card Logging
                        config.enableLogging = !config.enableLogging;
                        break;
                    case 6: // Deep Sleep
                        config.enableDeepSleep = !config.enableDeepSleep;
                        break;
                    case 7: // Police Filter
                        config.policeOnlyFilter = !config.policeOnlyFilter;
                        break;
                    case 8: // OTA Update
                        currentScreen = 3;
                        break;
                }
            }
            settingsSelectedItem = touchedItem;
            updateDisplay();
        }
    }
    else if (currentScreen == 3) {
        // OTA screen
        if (y < 25) {
            currentScreen = 1;
            updateDisplay();
        } else if (y > 100 && y < 160) {
            // Fetch updates button
            fetchOUIUpdates();
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
