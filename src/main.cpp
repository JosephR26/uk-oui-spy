/*
 * UK-OUI-SPY ESP32
 * Portable UK Surveillance Device Detector
 *
 * Hardware: ESP32-2432S028 (2.8" ILI9341 TFT, capacitive touch)
 * Features: Multi-page UI (Main, Settings, Logs, Info), FreeRTOS, O(1) OUI Lookup
 */

#define VERSION "1.3.3"

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

// UI Screens
enum Screen { SCREEN_MAIN, SCREEN_SETTINGS, SCREEN_LOGS, SCREEN_INFO };
Screen currentScreen = SCREEN_MAIN;

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
    unsigned long timestamp;
    int seenCount;
    bool isBLE;
};

struct Config {
    ScanMode scanMode = SCAN_NORMAL;
    AlertMode alertMode = ALERT_BUZZER;
    bool enableBLE = true;
    bool enableWiFi = true;
    bool enableLogging = true;
    int brightness = 255;
    bool autoBrightness = true;
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
bool sdCardAvailable = false;

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
void drawLogsScreen();
void drawInfoScreen();
void drawHeader(const char* title);
void drawNavbar();
void handleTouchGestures();
void setBrightness(int level);
void drawToggle(int x, int y, bool state, const char* label);

// BLE Scan callback
class MyAdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        checkOUI(advertisedDevice->getAddress().toString().c_str(), advertisedDevice->getRSSI(), true);
    }
};

// FreeRTOS Tasks
void ScanTask(void *pvParameters) {
    for (;;) {
        if (millis() - lastScanTime >= scanInterval) {
            scanning = true;
            digitalWrite(LED_PIN, HIGH);
            if (config.enableBLE) scanBLE();
            if (config.enableWiFi) scanWiFi();
            scanning = false;
            digitalWrite(LED_PIN, LOW);
            lastScanTime = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void UITask(void *pvParameters) {
    for (;;) {
        handleTouchGestures();
        if (config.autoBrightness) {
            static unsigned long lastLDR = 0;
            if (millis() - lastLDR > 2000) {
                int ldr = analogRead(LDR_PIN);
                setBrightness(map(ldr, 0, 4095, 50, 255));
                lastLDR = millis();
            }
        }
        updateDisplay();
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

bool readCapacitiveTouch(uint16_t *x, uint16_t *y) {
    Wire.beginTransmission(TOUCH_ADDR);
    Wire.write(0x00);
    if (Wire.endTransmission() != 0) return false;
    if (Wire.requestFrom(TOUCH_ADDR, 6) != 6) return false;
    Wire.read(); // Status
    if ((Wire.read() & 0x0F) == 0) return false;
    uint8_t xH = Wire.read(); uint8_t xL = Wire.read();
    uint8_t yH = Wire.read(); uint8_t yL = Wire.read();
    uint16_t rawX = ((xH & 0x0F) << 8) | xL;
    uint16_t rawY = ((yH & 0x0F) << 8) | yL;
    *x = rawY; *y = 239 - rawX;
    return true;
}

void setup() {
    Serial.begin(115200);
    xDetectionMutex = xSemaphoreCreateMutex();
    pinMode(LED_PIN, OUTPUT);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    initDisplay();
    initSDCard();
    if (!loadOUIDatabaseFromSD("/oui.csv")) initializeStaticDatabase();
    initBLE();
    initWiFi();
    xTaskCreatePinnedToCore(ScanTask, "ScanTask", 8192, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(UITask, "UITask", 8192, NULL, 1, NULL, 1);
}

void loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }

void initDisplay() { tft.init(); tft.setRotation(1); tft.fillScreen(TFT_BLACK); }
void initSDCard() { sdCardAvailable = SD.begin(SD_CS); }
void initBLE() { NimBLEDevice::init("UK-OUI-SPY"); }
void initWiFi() { WiFi.mode(WIFI_STA); WiFi.disconnect(); }

void scanBLE() {
    NimBLEScan* pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true);
    pBLEScan->start(2, false);
}

void scanWiFi() {
    int n = WiFi.scanNetworks(false, true, false, 100);
    for (int i = 0; i < n; ++i) checkOUI(WiFi.BSSIDstr(i), WiFi.RSSI(i), false);
}

void checkOUI(String macAddress, int8_t rssi, bool isBLE) {
    String mac = macAddress; mac.toUpperCase();
    String oui = mac.substring(0, 8);
    auto it = ouiLookup.find(oui);
    if (it != ouiLookup.end()) {
        Detection det;
        det.macAddress = mac;
        det.manufacturer = it->second->manufacturer;
        det.category = it->second->category;
        det.relevance = it->second->relevance;
        det.rssi = rssi;
        det.timestamp = millis();
        addDetection(det);
    }
}

void addDetection(Detection det) {
    xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
    bool found = false;
    for (auto &d : detections) {
        if (d.macAddress == det.macAddress) {
            d.rssi = det.rssi; d.timestamp = millis(); found = true; break;
        }
    }
    if (!found) {
        detections.insert(detections.begin(), det);
        if (detections.size() > MAX_DETECTIONS) detections.pop_back();
    }
    xSemaphoreGive(xDetectionMutex);
}

void updateDisplay() {
    switch (currentScreen) {
        case SCREEN_MAIN: drawMainScreen(); break;
        case SCREEN_SETTINGS: drawSettingsScreen(); break;
        case SCREEN_LOGS: drawLogsScreen(); break;
        case SCREEN_INFO: drawInfoScreen(); break;
    }
}

void drawHeader(const char* title) {
    tft.fillRect(0, 0, 320, 30, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 7);
    tft.print(title);
}

void drawNavbar() {
    tft.fillRect(0, 210, 320, 30, TFT_BLACK);
    tft.drawFastHLine(0, 210, 320, TFT_DARKGREY);
    tft.setTextSize(1);
    const char* labels[] = {"MAIN", "SET", "LOGS", "INFO"};
    for (int i = 0; i < 4; i++) {
        tft.setTextColor(currentScreen == i ? TFT_CYAN : TFT_WHITE);
        tft.setCursor(20 + (i * 80), 220);
        tft.print(labels[i]);
    }
}

void drawToggle(int x, int y, bool state, const char* label) {
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(x, y + 5);
    tft.print(label);
    tft.drawRect(x + 150, y, 40, 20, TFT_WHITE);
    if (state) tft.fillRect(x + 170, y + 2, 18, 16, TFT_GREEN);
    else tft.fillRect(x + 152, y + 2, 18, 16, TFT_RED);
}

void drawMainScreen() {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    drawHeader("LIVE SCAN");
    xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
    auto snapshot = detections;
    xSemaphoreGive(xDetectionMutex);
    int y = 35;
    for (auto &det : snapshot) {
        if (y > 180) break;
        tft.fillRect(0, y, 5, 30, getRelevanceColor(det.relevance));
        tft.setCursor(10, y + 5);
        tft.setTextColor(TFT_WHITE);
        tft.printf("%s | %d dBm", det.manufacturer.c_str(), det.rssi);
        y += 35;
    }
    drawNavbar();
    tft.endWrite();
}

void drawSettingsScreen() {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    drawHeader("SETTINGS");
    tft.setTextSize(1);
    drawToggle(20, 50, config.enableBLE, "BLE Scanning");
    drawToggle(20, 80, config.enableWiFi, "WiFi Scanning");
    drawToggle(20, 110, config.enableLogging, "SD Logging");
    drawToggle(20, 140, config.autoBrightness, "Auto-Brightness");
    drawNavbar();
    tft.endWrite();
}

void drawLogsScreen() {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    drawHeader("LOG HISTORY");
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    if (!sdCardAvailable) {
        tft.setCursor(20, 50); tft.print("SD Card: Not Found");
    } else {
        File f = SD.open("/detections.csv");
        if (!f) {
            tft.setCursor(20, 50); tft.print("No logs found.");
        } else {
            int y = 40;
            while (f.available() && y < 180) {
                String line = f.readStringUntil('\n');
                tft.setCursor(10, y);
                tft.print(line.substring(0, 40));
                y += 15;
            }
            f.close();
        }
    }
    drawNavbar();
    tft.endWrite();
}

void drawInfoScreen() {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    drawHeader("SYSTEM INFO");
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(20, 50); tft.printf("Version: %s", VERSION);
    tft.setCursor(20, 70); tft.printf("Hardware: ESP32-2432S028");
    tft.setCursor(20, 90); tft.printf("Display: 2.8\" ILI9341");
    tft.setCursor(20, 110); tft.printf("Touch: CST820 Capacitive");
    tft.setCursor(20, 130); tft.printf("Free Heap: %d KB", ESP.getFreeHeap() / 1024);
    tft.setCursor(20, 150); tft.printf("OUI DB Size: %d", dynamicDatabase.size());
    tft.setCursor(20, 170); tft.printf("Uptime: %d s", millis() / 1000);
    drawNavbar();
    tft.endWrite();
}

void handleTouchGestures() {
    uint16_t x, y;
    if (readCapacitiveTouch(&x, &y)) {
        if (y > 210) { // Navbar area
            int newScreen = x / 80;
            if (newScreen >= 0 && newScreen <= 3) currentScreen = (Screen)newScreen;
        } else if (currentScreen == SCREEN_SETTINGS) {
            if (x > 170 && x < 210) {
                if (y > 50 && y < 70) config.enableBLE = !config.enableBLE;
                else if (y > 80 && y < 100) config.enableWiFi = !config.enableWiFi;
                else if (y > 110 && y < 130) config.enableLogging = !config.enableLogging;
                else if (y > 140 && y < 160) config.autoBrightness = !config.autoBrightness;
            }
        }
    }
}

void setBrightness(int level) {
    config.brightness = constrain(level, 0, 255);
    analogWrite(TFT_BL, config.brightness);
}
