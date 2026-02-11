/*
 * UK-OUI-SPY ESP32 - PRO EDITION
 * Professional UK Surveillance Device Detector
 *
 * Hardware: ESP32-2432S028 (2.8" ILI9341 TFT, capacitive touch)
 * Features: Multi-page UI, FreeRTOS, O(1) OUI Lookup, Secure Logging,
 *           Radar Visualization, Setup Wizard, Power Management.
 */

#define VERSION "2.4.0-PRO"

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
#include <Preferences.h>
#include "esp_sleep.h"
#include "oui_database.h"
#include "wifi_promiscuous.h"
#include "mbedtls/aes.h"

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
#define BAT_ADC 35

// UI Screens
enum Screen { SCREEN_WIZARD, SCREEN_MAIN, SCREEN_RADAR, SCREEN_SETTINGS, SCREEN_INFO };
Screen currentScreen = SCREEN_WIZARD;

// Scan modes
enum ScanMode { SCAN_QUICK = 0, SCAN_NORMAL = 1, SCAN_POWER_SAVE = 2 };
enum AlertMode { ALERT_SILENT = 0, ALERT_BUZZER = 1, ALERT_VIBRATE = 2 };

struct Detection {
    String macAddress;
    String manufacturer;
    DeviceCategory category;
    RelevanceLevel relevance;
    int8_t rssi;
    unsigned long timestamp;
    bool isBLE;
};

struct Config {
    ScanMode scanMode = SCAN_NORMAL;
    AlertMode alertMode = ALERT_BUZZER;
    bool enableBLE = true;
    bool enableWiFi = true;
    bool enableLogging = true;
    bool secureLogging = false;
    int brightness = 255;
    bool autoBrightness = true;
    char encryptionKey[17] = "UK-OUI-SPY-2026";
    bool setupComplete = false;
    int sleepTimeout = 300; // 5 minutes
};

// Global objects
TFT_eSPI tft = TFT_eSPI();
Config config;
Preferences preferences;
std::vector<Detection> detections;
SemaphoreHandle_t xDetectionMutex;
const int MAX_DETECTIONS = 50;
unsigned long lastScanTime = 0;
unsigned long lastInteractionTime = 0;
int scanInterval = 5000;
bool scanning = false;
bool sdCardAvailable = false;
float batteryVoltage = 0.0;
bool i2cAvailable = false;
int wizardStep = 0;

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
void drawWizardScreen();
void drawMainScreen();
void drawRadarScreen();
void drawSettingsScreen();
void drawInfoScreen();
void drawHeader(const char* title);
void drawNavbar();
void handleTouchGestures();
void setBrightness(int level);
void drawToggle(int x, int y, bool state, const char* label);
void encryptAndLog(String data);
float readBattery();
void saveConfig();
void loadConfig();
void enterDeepSleep();

// BLE Scan callback
class MyAdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        checkOUI(advertisedDevice->getAddress().toString().c_str(), advertisedDevice->getRSSI(), true);
    }
};

// FreeRTOS Tasks
void ScanTask(void *pvParameters) {
    for (;;) {
        if (config.setupComplete && millis() - lastScanTime >= scanInterval) {
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
        batteryVoltage = readBattery();
        updateDisplay();
        
        // Power Management
        if (millis() - lastInteractionTime > (config.sleepTimeout * 1000)) {
            enterDeepSleep();
        }
        
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

bool readCapacitiveTouch(uint16_t *x, uint16_t *y) {
    if (!i2cAvailable) return false;
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
    lastInteractionTime = millis();
    return true;
}

void setup() {
    Serial.begin(115200);
    xDetectionMutex = xSemaphoreCreateMutex();
    pinMode(LED_PIN, OUTPUT);
    pinMode(LED_G_PIN, OUTPUT);
    pinMode(LED_B_PIN, OUTPUT);
    pinMode(TFT_BL, OUTPUT);
    pinMode(BAT_ADC, INPUT);
    digitalWrite(TFT_BL, HIGH);
    
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Wire.beginTransmission(TOUCH_ADDR);
    if (Wire.endTransmission() == 0) i2cAvailable = true;
    
    initDisplay();
    loadConfig();
    if (!config.setupComplete) currentScreen = SCREEN_WIZARD;
    else currentScreen = SCREEN_MAIN;
    
    initSDCard();
    if (!loadOUIDatabaseFromSD("/oui.csv")) initializeStaticDatabase();
    initBLE();
    initWiFi();
    
    lastInteractionTime = millis();
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
        
        if (config.enableLogging && sdCardAvailable) {
            String logData = mac + "," + String(rssi) + "," + det.manufacturer;
            if (config.secureLogging) encryptAndLog(logData);
            else {
                File f = SD.open("/detections.csv", FILE_APPEND);
                if (f) { f.println(logData); f.close(); }
            }
        }
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
        case SCREEN_WIZARD: drawWizardScreen(); break;
        case SCREEN_MAIN: drawMainScreen(); break;
        case SCREEN_RADAR: drawRadarScreen(); break;
        case SCREEN_SETTINGS: drawSettingsScreen(); break;
        case SCREEN_INFO: drawInfoScreen(); break;
    }
}

void drawHeader(const char* title) {
    tft.fillRect(0, 0, 320, 30, 0x1082);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 7);
    tft.print(title);
    if (scanning) tft.fillCircle(250, 15, 4, TFT_CYAN);
    tft.drawRect(280, 8, 25, 12, TFT_WHITE);
    tft.fillRect(305, 11, 2, 6, TFT_WHITE);
    int batWidth = map(constrain(batteryVoltage * 100, 330, 420), 330, 420, 0, 21);
    tft.fillRect(282, 10, batWidth, 8, batteryVoltage < 3.5 ? TFT_RED : TFT_GREEN);
}

void drawNavbar() {
    tft.fillRect(0, 210, 320, 30, 0x0841);
    tft.drawFastHLine(0, 210, 320, 0x2104);
    tft.setTextSize(1);
    const char* labels[] = {"LIST", "RADAR", "CONFIG", "INFO"};
    for (int i = 0; i < 4; i++) {
        tft.setTextColor(currentScreen == i + 1 ? TFT_CYAN : 0x7BEF);
        tft.setCursor(20 + (i * 80), 220);
        tft.print(labels[i]);
    }
}

void drawWizardScreen() {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    drawHeader("WELCOME");
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    if (wizardStep == 0) {
        tft.setCursor(20, 60); tft.print("UK-OUI-SPY PRO EDITION");
        tft.setCursor(20, 80); tft.print("Professional Surveillance Detection");
        tft.setCursor(20, 120); tft.print("Tap 'NEXT' to begin setup.");
    } else if (wizardStep == 1) {
        tft.setCursor(20, 60); tft.print("SD CARD CHECK");
        tft.setCursor(20, 80); tft.print(sdCardAvailable ? "SD Card: DETECTED" : "SD Card: NOT FOUND");
        tft.setCursor(20, 100); tft.print("Please insert SD card for logging.");
    } else if (wizardStep == 2) {
        tft.setCursor(20, 60); tft.print("SETUP COMPLETE");
        tft.setCursor(20, 80); tft.print("Device is ready for field use.");
        tft.setCursor(20, 120); tft.print("Tap 'FINISH' to start scanning.");
    }
    tft.fillRoundRect(220, 160, 80, 30, 4, TFT_CYAN);
    tft.setTextColor(TFT_BLACK);
    tft.setCursor(240, 170); tft.print(wizardStep < 2 ? "NEXT" : "FINISH");
    tft.endWrite();
}

void drawMainScreen() {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    drawHeader("SURVEILLANCE LIST");
    xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
    auto snapshot = detections;
    xSemaphoreGive(xDetectionMutex);
    int y = 35;
    for (auto &det : snapshot) {
        if (y > 180) break;
        tft.fillRoundRect(5, y, 310, 32, 4, 0x18C3);
        tft.fillRect(5, y, 4, 32, getRelevanceColor(det.relevance));
        tft.setCursor(15, y + 6);
        tft.setTextColor(TFT_WHITE);
        tft.print(det.manufacturer);
        tft.setCursor(15, y + 18);
        tft.setTextColor(0xAD55);
        tft.printf("%s | %d dBm", getCategoryName(det.category), det.rssi);
        y += 36;
    }
    drawNavbar();
    tft.endWrite();
}

void drawRadarScreen() {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    drawHeader("PROXIMITY RADAR");
    int cx = 160, cy = 120, r = 80;
    tft.drawCircle(cx, cy, r, 0x2104);
    tft.drawCircle(cx, cy, r/2, 0x2104);
    tft.drawLine(cx-r, cy, cx+r, cy, 0x2104);
    tft.drawLine(cx, cy-r, cx, cy+r, 0x2104);
    xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
    auto snapshot = detections;
    xSemaphoreGive(xDetectionMutex);
    for (auto &det : snapshot) {
        float dist = map(constrain(det.rssi, -100, -30), -100, -30, r, 10);
        float angle = (float)random(0, 360) * PI / 180.0;
        int px = cx + dist * cos(angle);
        int py = cy + dist * sin(angle);
        tft.fillCircle(px, py, 4, getRelevanceColor(det.relevance));
    }
    drawNavbar();
    tft.endWrite();
}

void drawSettingsScreen() {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    drawHeader("CONFIGURATION");
    drawToggle(20, 50, config.enableBLE, "BLE Scanning");
    drawToggle(20, 80, config.enableWiFi, "WiFi Scanning");
    drawToggle(20, 110, config.enableLogging, "SD Logging");
    drawToggle(20, 140, config.secureLogging, "Secure (AES)");
    drawToggle(20, 170, config.autoBrightness, "Auto-Brightness");
    drawNavbar();
    tft.endWrite();
}

void drawInfoScreen() {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    drawHeader("SYSTEM STATUS");
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(20, 50); tft.printf("Firmware: %s", VERSION);
    tft.setCursor(20, 70); tft.printf("Battery: %.2f V", batteryVoltage);
    tft.setCursor(20, 90); tft.printf("OUI DB: %d entries", dynamicDatabase.size());
    tft.setCursor(20, 110); tft.printf("Memory: %d KB Free", ESP.getFreeHeap() / 1024);
    tft.setCursor(20, 130); tft.printf("Touch: %s", i2cAvailable ? "OK" : "ERROR");
    drawNavbar();
    tft.endWrite();
}

void handleTouchGestures() {
    uint16_t x, y;
    if (readCapacitiveTouch(&x, &y)) {
        if (currentScreen == SCREEN_WIZARD) {
            if (x > 220 && y > 160) {
                wizardStep++;
                if (wizardStep > 2) {
                    config.setupComplete = true;
                    saveConfig();
                    currentScreen = SCREEN_MAIN;
                }
            }
        } else if (y > 210) {
            int newScreen = x / 80;
            if (newScreen >= 0 && newScreen <= 3) currentScreen = (Screen)(newScreen + 1);
        } else if (currentScreen == SCREEN_SETTINGS) {
            if (x > 170 && x < 215) {
                if (y > 50 && y < 72) config.enableBLE = !config.enableBLE;
                else if (y > 80 && y < 102) config.enableWiFi = !config.enableWiFi;
                else if (y > 110 && y < 132) config.enableLogging = !config.enableLogging;
                else if (y > 140 && y < 162) config.secureLogging = !config.secureLogging;
                else if (y > 170 && y < 192) config.autoBrightness = !config.autoBrightness;
                saveConfig();
            }
        }
    }
}

void setBrightness(int level) {
    config.brightness = constrain(level, 0, 255);
    analogWrite(TFT_BL, config.brightness);
}

float readBattery() {
    int raw = analogRead(BAT_ADC);
    return (raw / 4095.0) * 2.0 * 3.3 * 1.1;
}

void encryptAndLog(String data) {
    mbedtls_aes_context aes;
    unsigned char key[16];
    memcpy(key, config.encryptionKey, 16);
    unsigned char input[16];
    unsigned char output[16];
    memset(input, 0, 16);
    strncpy((char*)input, data.c_str(), 15);
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 128);
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, input, output);
    mbedtls_aes_free(&aes);
    File f = SD.open("/secure.log", FILE_APPEND);
    if (f) { f.write(output, 16); f.close(); }
}

void saveConfig() {
    preferences.begin("uk-oui-spy", false);
    preferences.putBool("setup", config.setupComplete);
    preferences.putBool("ble", config.enableBLE);
    preferences.putBool("wifi", config.enableWiFi);
    preferences.putBool("log", config.enableLogging);
    preferences.putBool("secure", config.secureLogging);
    preferences.putBool("auto", config.autoBrightness);
    preferences.end();
}

void loadConfig() {
    preferences.begin("uk-oui-spy", true);
    config.setupComplete = preferences.getBool("setup", false);
    config.enableBLE = preferences.getBool("ble", true);
    config.enableWiFi = preferences.getBool("wifi", true);
    config.enableLogging = preferences.getBool("log", true);
    config.secureLogging = preferences.getBool("secure", false);
    config.autoBrightness = preferences.getBool("auto", true);
    preferences.end();
}

void enterDeepSleep() {
    tft.fillScreen(TFT_BLACK);
    digitalWrite(TFT_BL, LOW);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_SDA, 0); // Wake on touch
    esp_deep_sleep_start();
}
