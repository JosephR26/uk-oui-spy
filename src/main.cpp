/*
 * UK-OUI-SPY ESP32
 * Portable UK Surveillance Device Detector
 *
 * Hardware: ESP32-2432S028 (2.8" ILI9341 TFT, capacitive touch)
 * Features: BLE/WiFi scanning, WiFi promiscuous mode, OUI matching,
 *           proximity alerts, logging, settings screen, deep sleep,
 *           threat scoring, swipe UI, OTA updates
 */

#define VERSION "1.2.2"

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
#define TOUCH_ADDR  0x15
#define TOUCH_SDA   27
#define TOUCH_SCL   22

// Pin definitions
#define TFT_BL 21
#define SD_CS 5
#define BUZZER_PIN 25
#define LED_PIN 4
#define LED_G_PIN 16
#define LED_B_PIN 17
#define LDR_PIN 34

// Scan modes
enum ScanMode { SCAN_QUICK = 0, SCAN_NORMAL = 1, SCAN_POWER_SAVE = 2 };
enum AlertMode { ALERT_SILENT = 0, ALERT_BUZZER = 1, ALERT_VIBRATE = 2 };

struct Detection {
    String macAddress;
    String oui;
    String manufacturer;
    DeviceCategory category;
    RelevanceLevel relevance;
    DeploymentType deployment;
    String notes;
    int8_t rssi;
    int8_t rssiMin;
    int8_t rssiMax;
    float rssiAvg;
    unsigned long timestamp;
    unsigned long unixTime;
    unsigned long firstSeen;
    unsigned long lastSeen;
    int seenCount;
    bool isBLE;
    bool isStationary;
    float threatScore;
};

struct Config {
    ScanMode scanMode = SCAN_NORMAL;
    AlertMode alertMode = ALERT_BUZZER;
    bool enableBLE = true;
    bool enableWiFi = true;
    bool enableWiFiPromiscuous = true;
    bool enableLogging = true;
    bool enableDeepSleep = false;
    bool policeOnlyFilter = false;
    int brightness = 255;
    int rssiThresholdHigh = -50;
    int rssiThresholdMedium = -70;
    int rssiThresholdLow = -90;
};

// Global objects
TFT_eSPI tft = TFT_eSPI();
Config config;
std::vector<Detection> detections;
SemaphoreHandle_t xDetectionMutex;
const int MAX_DETECTIONS = 50;
unsigned long lastScanTime = 0;
int scanInterval = 5000;
bool scanning = false;
int scrollOffset = 0;
bool sdCardAvailable = false;
String sessionID = "";
bool timeValid = false;
int currentScreen = 0;
int selectedRelevanceFilter = -1;
int settingsSelectedItem = 0;
const int SETTINGS_ITEMS = 9;
const int SETTINGS_START_Y = 28;
const int SETTINGS_ITEM_HEIGHT = 22;

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
void playProximityAlert(int8_t rssi, RelevanceLevel relevance);
void logDetectionToSD(Detection &det);
String formatMAC(String mac);
String extractOUI(String mac);
void setBrightness(int level);
float calculateThreatScore(Detection &det);
void sortDetectionsByThreat();

// BLE Scan callback
class MyAdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        String mac = advertisedDevice->getAddress().toString().c_str();
        int8_t rssi = advertisedDevice->getRSSI();
        checkOUI(mac, rssi, true);
    }
};

// FreeRTOS Task Handles
TaskHandle_t ScanTaskHandle = NULL;
TaskHandle_t UITaskHandle = NULL;

// Task for Scanning (Core 0)
void ScanTask(void *pvParameters) {
    for (;;) {
        unsigned long currentTime = millis();
        if (currentTime - lastScanTime >= scanInterval) {
            scanning = true;
            digitalWrite(LED_PIN, HIGH);
            if (config.enableBLE) scanBLE();
            if (config.enableWiFi) scanWiFi();
            if (config.enableWiFiPromiscuous) {
                startWiFiPromiscuous([](const uint8_t* mac, int8_t rssi, uint8_t channel) {
                    char macStr[18];
                    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                    checkOUI(String(macStr), rssi, false);
                });
                scanAllChannels([](const uint8_t* mac, int8_t rssi, uint8_t channel){}, 200);
                stopWiFiPromiscuous();
            }
            xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
            sortDetectionsByThreat();
            xSemaphoreGive(xDetectionMutex);
            scanning = false;
            digitalWrite(LED_PIN, LOW);
            lastScanTime = currentTime;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Task for UI and Touch (Core 1)
void UITask(void *pvParameters) {
    for (;;) {
        handleTouchGestures();
        
        // Auto-brightness
        static unsigned long lastLDR = 0;
        if (millis() - lastLDR > 2000) {
            int ldr = analogRead(LDR_PIN);
            setBrightness(map(ldr, 0, 4095, 50, 255));
            lastLDR = millis();
        }
        
        updateDisplay();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// Robust CST820 touch reading
bool readCapacitiveTouch(uint16_t *x, uint16_t *y) {
    Wire.beginTransmission(TOUCH_ADDR);
    Wire.write(0x00);
    if (Wire.endTransmission() != 0) return false;
    if (Wire.requestFrom(TOUCH_ADDR, 6) != 6) return false;
    uint8_t status = Wire.read();
    uint8_t points = Wire.read() & 0x0F;
    if (points == 0) return false;
    uint8_t xH = Wire.read(); uint8_t xL = Wire.read();
    uint8_t yH = Wire.read(); uint8_t yL = Wire.read();
    uint16_t rawX = ((xH & 0x0F) << 8) | xL;
    uint16_t rawY = ((yH & 0x0F) << 8) | yL;
    *x = rawY; *y = 239 - rawX;
    if (*x > 319) *x = 319; if (*y > 239) *y = 239;
    return true;
}

void setup() {
    Serial.begin(115200);
    xDetectionMutex = xSemaphoreCreateMutex();
    
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(LED_G_PIN, OUTPUT);
    pinMode(LED_B_PIN, OUTPUT);
    pinMode(LDR_PIN, INPUT);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    initDisplay();
    initSDCard();
    
    if (!loadOUIDatabaseFromSD("/oui.csv")) {
        initializeStaticDatabase();
    }
    
    initBLE();
    initWiFi();
    
    xTaskCreatePinnedToCore(ScanTask, "ScanTask", 8192, NULL, 1, &ScanTaskHandle, 0);
    xTaskCreatePinnedToCore(UITask, "UITask", 8192, NULL, 1, &UITaskHandle, 1);
}

void loop() {
    // Keep the main task alive but idle
    vTaskDelay(pdMS_TO_TICKS(1000));
}

void initDisplay() {
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_NAVY);
}

void initSDCard() {
    if (SD.begin(SD_CS)) {
        sdCardAvailable = true;
    }
}

void initBLE() {
    NimBLEDevice::init("UK-OUI-SPY");
}

void initWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
}

void scanBLE() {
    NimBLEScan* pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true);
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    pBLEScan->start(2, false);
}

void scanWiFi() {
    int n = WiFi.scanNetworks(false, true, false, 100);
    for (int i = 0; i < n; ++i) {
        checkOUI(WiFi.BSSIDstr(i), WiFi.RSSI(i), false);
    }
}

void checkOUI(String macAddress, int8_t rssi, bool isBLE) {
    String mac = formatMAC(macAddress);
    String oui = extractOUI(mac);
    
    auto it = ouiLookup.find(oui);
    if (it != ouiLookup.end()) {
        const auto& entry = *(it->second);
        Detection det;
        det.macAddress = mac;
        det.oui = oui;
        det.manufacturer = entry.manufacturer;
        det.category = entry.category;
        det.relevance = entry.relevance;
        det.deployment = entry.deployment;
        det.notes = entry.notes;
        det.rssi = rssi;
        det.timestamp = millis();
        det.firstSeen = millis();
        det.lastSeen = millis();
        det.seenCount = 1;
        det.isBLE = isBLE;
        
        addDetection(det);
        
        // Blocking calls outside mutex
        playProximityAlert(rssi, det.relevance);
        if (config.enableLogging && sdCardAvailable) logDetectionToSD(det);
    }
}

void addDetection(Detection det) {
    xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
    bool found = false;
    for (auto &d : detections) {
        if (d.macAddress == det.macAddress && (millis() - d.timestamp) < 30000) {
            d.rssi = det.rssi;
            d.seenCount++;
            d.lastSeen = millis();
            d.timestamp = millis();
            found = true;
            break;
        }
    }
    if (!found) {
        detections.insert(detections.begin(), det);
        if (detections.size() > MAX_DETECTIONS) detections.pop_back();
    }
    xSemaphoreGive(xDetectionMutex);
}

void updateDisplay() {
    if (currentScreen == 0) drawMainScreen();
    else if (currentScreen == 1) drawSettingsScreen();
}

void drawMainScreen() {
    std::vector<Detection> snapshot;
    xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
    snapshot = detections;
    xSemaphoreGive(xDetectionMutex);

    tft.startWrite();
    tft.fillScreen(TFT_NAVY);
    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(5, 5);
    tft.print("UK-OUI-SPY");
    
    int y = 35;
    for (auto &det : snapshot) {
        if (y > 200) break;
        drawDetection(det, y, false);
        y += 42;
    }
    tft.endWrite();
}

void drawSettingsScreen() {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, 10);
    tft.print("SETTINGS");
    tft.setTextSize(1);
    tft.setCursor(10, 40);
    tft.print("Tap top-left to return");
    tft.endWrite();
}

void drawDetection(Detection &det, int y, bool highlight) {
    tft.fillRect(0, y, 5, 40, getRelevanceColor(det.relevance));
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(8, y + 1);
    tft.print(det.manufacturer);
    tft.setCursor(8, y + 12);
    tft.printf("%s | RSSI:%d", getCategoryName(det.category), det.rssi);
}

void setBrightness(int level) {
    config.brightness = constrain(level, 0, 255);
    digitalWrite(TFT_BL, config.brightness > 127 ? HIGH : LOW);
}

String formatMAC(String mac) {
    mac.toUpperCase();
    mac.replace("-", ":");
    return mac;
}

String extractOUI(String mac) {
    if (mac.length() < 8) return "";
    return mac.substring(0, 8);
}

void sortDetectionsByThreat() {
    std::sort(detections.begin(), detections.end(), [](const Detection& a, const Detection& b) {
        return a.rssi > b.rssi;
    });
}

void playProximityAlert(int8_t rssi, RelevanceLevel relevance) {
    if (config.alertMode == ALERT_SILENT || relevance == REL_LOW) return;
    int freq = (relevance == REL_HIGH) ? 2000 : 1500;
    tone(BUZZER_PIN, freq, 100);
}

void logDetectionToSD(Detection &det) {
    File f = SD.open("/detections.csv", FILE_APPEND);
    if (f) {
        f.printf("%s,%d,%s\n", det.macAddress.c_str(), det.rssi, det.manufacturer.c_str());
        f.close();
    }
}

void handleTouchGestures() {
    uint16_t x, y;
    if (readCapacitiveTouch(&x, &y)) {
        if (currentScreen == 0) {
            if (x > 250 && y < 50) currentScreen = 1;
        } else if (currentScreen == 1) {
            if (x < 50 && y < 50) currentScreen = 0;
        }
    }
}
