/*
 * UK-OUI-SPY PRO EDITION v3.0.0
 * Professional UK Surveillance Device Detector
 *
 * Hardware: ESP32-2432S028 (2.8" ILI9341 TFT, capacitive touch)
 * Features: Tiered Priority Display, Correlation Detection Engine,
 *           FreeRTOS Dual-Core, O(1) OUI Lookup, Secure Logging,
 *           Radar Visualization, Setup Wizard, Power Management.
 */

#define VERSION "3.1.0-PRO"

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <vector>
#include <algorithm>
#include <map>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "esp_sleep.h"
#include "oui_database.h"
#include "wifi_promiscuous.h"
#include "mbedtls/aes.h"

// ============================================================
// HARDWARE PIN DEFINITIONS
// ============================================================

// CST820 capacitive touch controller (I2C)
#define TOUCH_ADDR  0x15

#ifdef BOARD_CYD_2USB
// 2-USB CYD variant: capacitive touch (CST820), SDA=33, SCL=32, RST=GPIO 25
#define TOUCH_SDA   33
#define TOUCH_SCL   32
#define TOUCH_RST   25
#else
// Default / original CYD variant (resistive touch)
#define TOUCH_SDA   27
#define TOUCH_SCL   22
#endif

// Backlight polarity (derived from platformio.ini)
#ifndef TFT_BACKLIGHT_ON
#define TFT_BACKLIGHT_ON HIGH
#endif
#if TFT_BACKLIGHT_ON == HIGH
#define TFT_BACKLIGHT_OFF LOW
#else
#define TFT_BACKLIGHT_OFF HIGH
#endif

#define SD_CS 5
#define LED_PIN 4
#define LED_G_PIN 16
#define LED_B_PIN 17
#define LDR_PIN 34
#define BAT_ADC 35

// ============================================================
// PRIORITY TIER SYSTEM
// ============================================================

#define PRIORITY_CRITICAL  5   // High Value Targets (drones, govt CCTV)
#define PRIORITY_HIGH      4   // Surveillance Infrastructure
#define PRIORITY_MODERATE  3   // Vehicle CCTV / Bodycams
#define PRIORITY_LOW       2   // Consumer Surveillance
#define PRIORITY_BASELINE  1   // ISP routers, consumer CPE (filtered)

// Premium colour palette (565 format)
#define COL_BG         0x0000  // Pure black
#define COL_CARD       0x18C3  // Dark charcoal card
#define COL_CARD_HI    0x2945  // Slightly lighter card for high-value
#define COL_HEADER     0x1082  // Dark header bar
#define COL_NAVBAR     0x0841  // Dark navbar
#define COL_ACCENT     0x07FF  // Cyan accent
#define COL_TIER5      0xF800  // Red - Critical
#define COL_TIER4      0xFD20  // Orange - High
#define COL_TIER3      0xFFE0  // Yellow - Moderate
#define COL_TIER2      0x07E0  // Green - Low
#define COL_TIER1      0x4208  // Grey - Baseline
#define COL_ALERT_BG   0x4000  // Dark red alert background
#define COL_DIMTEXT    0xAD55  // Dimmed text

// ============================================================
// DATA STRUCTURES
// ============================================================

// UI Screens
enum Screen { SCREEN_WIZARD, SCREEN_MAIN, SCREEN_RADAR, SCREEN_SETTINGS, SCREEN_INFO };
Screen currentScreen = SCREEN_WIZARD;

enum ScanMode { SCAN_QUICK = 0, SCAN_NORMAL = 1, SCAN_POWER_SAVE = 2 };
enum AlertMode { ALERT_SILENT = 0, ALERT_LED = 1, ALERT_VIBRATE = 2 };

// Priority entry loaded from priority.json
struct PriorityEntry {
    String oui;
    String label;
    String context;
    String correlationGroup;
    int priority;
    float confidence;
};

// Correlation rule loaded from priority.json
struct CorrelationRule {
    String id;
    String name;
    String description;
    std::vector<String> requiredGroups;
    int minDevices;
    String alertLevel;
};

// Active correlation alert
struct CorrelationAlert {
    String name;
    String description;
    String alertLevel;
    unsigned long timestamp;
};

// Enhanced detection with priority
struct Detection {
    String macAddress;
    String manufacturer;
    String context;
    String correlationGroup;
    DeviceCategory category;
    RelevanceLevel relevance;
    int priority;
    int8_t rssi;
    unsigned long timestamp;
    unsigned long firstSeen;
    int sightings;
    bool isBLE;
};

struct Config {
    ScanMode scanMode = SCAN_NORMAL;
    AlertMode alertMode = ALERT_LED;
    bool enableBLE = true;
    bool enableWiFi = true;
    bool enableLogging = true;
    bool secureLogging = false;
    bool showBaseline = false;  // Filter baseline by default
    int brightness = 255;
    bool autoBrightness = true;
    char encryptionKey[17] = "UK-OUI-SPY-2026";
    bool setupComplete = false;
    int sleepTimeout = 300;
};

// ============================================================
// GLOBALS
// ============================================================

TFT_eSPI tft = TFT_eSPI();
Config config;
Preferences preferences;
std::vector<Detection> detections;
std::vector<PriorityEntry> priorityDB;
std::vector<CorrelationRule> correlationRules;
std::vector<CorrelationAlert> activeAlerts;
std::map<String, PriorityEntry*> priorityLookup;  // OUI -> PriorityEntry
SemaphoreHandle_t xDetectionMutex;

const int MAX_DETECTIONS = 50;
const int MAX_ALERTS = 5;
unsigned long lastScanTime = 0;
unsigned long lastInteractionTime = 0;
int scanInterval = 5000;
bool scanning = false;
bool sdCardAvailable = false;
float batteryVoltage = 0.0;
bool i2cAvailable = false;
int wizardStep = 0;
int scrollOffset = 0;
int maxScroll = 0;

// ============================================================
// FUNCTION PROTOTYPES
// ============================================================

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
bool loadPriorityDB(const char* path);
void runCorrelationEngine();
void drawCorrelationBanner();
void alertLED(int priority);
uint16_t getTierColor(int priority);
const char* getTierLabel(int priority);
int getTierStars(int priority);

// ============================================================
// PRIORITY DATABASE LOADER
// ============================================================

bool loadPriorityDB(const char* path) {
    if (!SD.exists(path)) return false;
    File file = SD.open(path);
    if (!file) return false;

    // Allocate JSON document (adjust size for your DB)
    DynamicJsonDocument doc(16384);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.printf("Priority JSON parse error: %s\n", error.c_str());
        return false;
    }

    priorityDB.clear();
    priorityLookup.clear();

    // Load entries
    JsonArray entries = doc["entries"];
    for (JsonObject entry : entries) {
        PriorityEntry pe;
        pe.oui = entry["oui"].as<String>();
        pe.label = entry["label"].as<String>();
        pe.context = entry["context"].as<String>();
        pe.correlationGroup = entry["correlation_group"].as<String>();
        pe.priority = entry["priority"].as<int>();
        pe.confidence = entry["confidence"].as<float>();
        priorityDB.push_back(pe);
    }

    // Build lookup table
    for (auto& pe : priorityDB) {
        priorityLookup[pe.oui] = &pe;
    }

    // Load correlation rules
    correlationRules.clear();
    JsonArray rules = doc["correlation_rules"];
    for (JsonObject rule : rules) {
        CorrelationRule cr;
        cr.id = rule["id"].as<String>();
        cr.name = rule["name"].as<String>();
        cr.description = rule["description"].as<String>();
        cr.minDevices = rule["min_devices"].as<int>();
        cr.alertLevel = rule["alert_level"].as<String>();
        JsonArray groups = rule["required_groups"];
        for (const char* g : groups) {
            cr.requiredGroups.push_back(String(g));
        }
        correlationRules.push_back(cr);
    }

    Serial.printf("Priority DB: %d entries, %d rules loaded\n", priorityDB.size(), correlationRules.size());
    return true;
}

// Fallback priority entries if no SD card
void initializeStaticPriorityDB() {
    priorityDB.clear();
    priorityLookup.clear();

    // Field-validated high-value targets
    priorityDB.push_back({"00:60:37", "Skydio Controller", "Autonomous drone ops", "skydio_ops", 5, 0.98});
    priorityDB.push_back({"90:9F:33", "Sky Drone", "Autonomous drone ops", "skydio_ops", 5, 0.95});
    priorityDB.push_back({"60:60:1F", "DJI", "Commercial/police drone", "dji_ops", 5, 0.97});
    priorityDB.push_back({"B8:69:F4", "Ubiquiti Networks", "UniFi cameras/APs", "ubiquiti_infra", 4, 0.88});
    priorityDB.push_back({"00:12:12", "Hikvision", "Govt/council CCTV", "hikvision_net", 5, 0.99});
    priorityDB.push_back({"00:40:8C", "Axis Communications", "Professional IP cameras", "axis_net", 5, 0.99});
    priorityDB.push_back({"A4:DA:32", "Dahua Technology", "Govt/council CCTV", "dahua_net", 5, 0.99});
    priorityDB.push_back({"3C:EF:8C", "Dahua Technology", "IP cameras and NVRs", "dahua_net", 5, 0.97});
    priorityDB.push_back({"E0:50:8B", "Genetec", "Facial recognition", "genetec_net", 5, 0.96});
    priorityDB.push_back({"00:18:7D", "Pelco (Motorola)", "Police/transport CCTV", "pelco_net", 5, 0.95});
    priorityDB.push_back({"D8:60:CF", "Smart Dashcam", "Delivery/bodycam", "vehicle_cam", 3, 0.90});
    priorityDB.push_back({"74:83:C2", "GoPro", "Action cams/bodycam", "gopro_cam", 3, 0.92});
    priorityDB.push_back({"28:87:BA", "GoPro", "Action cams (alt OUI)", "gopro_cam", 3, 0.92});
    priorityDB.push_back({"6C:C2:17", "Dahua Technology", "Security cameras", "dahua_net", 5, 0.97});
    priorityDB.push_back({"B0:A7:B9", "Reolink", "WiFi cameras", "reolink_cam", 3, 0.88});
    priorityDB.push_back({"50:C7:BF", "TP-Link Tapo", "Tapo cameras", "tplink_cam", 2, 0.85});
    priorityDB.push_back({"18:E8:29", "Ubiquiti Networks", "UniFi cameras", "ubiquiti_infra", 4, 0.88});
    priorityDB.push_back({"74:83:C2", "Ubiquiti Networks", "UniFi cameras", "ubiquiti_infra", 4, 0.88});
    priorityDB.push_back({"18:B4:30", "Nest (Google)", "Consumer doorbell cam", "nest_home", 2, 0.85});
    priorityDB.push_back({"EC:71:DB", "Ring (Amazon)", "Consumer doorbell cam", "ring_home", 2, 0.85});
    priorityDB.push_back({"74:DA:88", "Sky CPE", "Consumer broadband", "consumer_isp", 1, 0.99});
    priorityDB.push_back({"FC:F8:AE", "BT/EE Hub", "Consumer broadband", "consumer_isp", 1, 0.99});
    priorityDB.push_back({"20:8B:FB", "TP-Link", "Consumer networking", "consumer_isp", 1, 0.99});

    for (auto& pe : priorityDB) {
        priorityLookup[pe.oui] = &pe;
    }

    // Static correlation rules
    correlationRules.clear();
    CorrelationRule skydio;
    skydio.id = "skydio_active_ops";
    skydio.name = "SKYDIO OPS ACTIVE";
    skydio.description = "Skydio controller + drone both detected";
    skydio.requiredGroups.push_back("skydio_ops");
    skydio.minDevices = 2;
    skydio.alertLevel = "CRITICAL";
    correlationRules.push_back(skydio);

    CorrelationRule dji;
    dji.id = "dji_active_ops";
    dji.name = "DJI DRONE OPS";
    dji.description = "DJI drone platform detected";
    dji.requiredGroups.push_back("dji_ops");
    dji.minDevices = 1;
    dji.alertLevel = "HIGH";
    correlationRules.push_back(dji);

    CorrelationRule cluster;
    cluster.id = "surveillance_cluster";
    cluster.name = "SURVEILLANCE CLUSTER";
    cluster.description = "Multiple surveillance devices - monitored zone";
    cluster.requiredGroups.push_back("hikvision_net");
    cluster.requiredGroups.push_back("axis_net");
    cluster.requiredGroups.push_back("dahua_net");
    cluster.requiredGroups.push_back("pelco_net");
    cluster.requiredGroups.push_back("ubiquiti_infra");
    cluster.minDevices = 3;
    cluster.alertLevel = "HIGH";
    correlationRules.push_back(cluster);

    CorrelationRule faceRecog;
    faceRecog.id = "facial_recognition_zone";
    faceRecog.name = "FACE RECOG ZONE";
    faceRecog.description = "Facial recognition infrastructure detected";
    faceRecog.requiredGroups.push_back("genetec_net");
    faceRecog.minDevices = 1;
    faceRecog.alertLevel = "CRITICAL";
    correlationRules.push_back(faceRecog);
}

// ============================================================
// CORRELATION ENGINE
// ============================================================

void runCorrelationEngine() {
    // Count devices per correlation group
    std::map<String, int> groupCounts;
    xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
    for (auto& det : detections) {
        if (!det.correlationGroup.isEmpty()) {
            groupCounts[det.correlationGroup]++;
        }
    }
    xSemaphoreGive(xDetectionMutex);

    // Evaluate each rule
    activeAlerts.clear();
    for (auto& rule : correlationRules) {
        int matchingDevices = 0;
        for (auto& group : rule.requiredGroups) {
            auto it = groupCounts.find(group);
            if (it != groupCounts.end()) {
                matchingDevices += it->second;
            }
        }
        if (matchingDevices >= rule.minDevices) {
            CorrelationAlert alert;
            alert.name = rule.name;
            alert.description = rule.description;
            alert.alertLevel = rule.alertLevel;
            alert.timestamp = millis();
            activeAlerts.push_back(alert);

            // Trigger LED alerts for critical correlations
            if (rule.alertLevel == "CRITICAL") {
                alertLED(5);
            } else if (rule.alertLevel == "HIGH") {
                alertLED(4);
            }
        }
    }
}

// ============================================================
// ALERT SYSTEM
// ============================================================

// LED alert patterns — red flash only, scaled by severity.
// Indexed by priority level (0–5).  Tier 0–2 are silent.
static const struct { uint16_t onMs; uint16_t offMs; uint8_t pulses; } LED_ALERT[] = {
    /*0 unused */ {  0,  0, 0},
    /*1 BASE   */ {  0,  0, 0},
    /*2 LOW    */ {  0,  0, 0},
    /*3 MOD    */ {100,  0, 1},
    /*4 HIGH   */ {200,  0, 1},
    /*5 CRIT   */ { 50, 50, 5},
};

void alertLED(int priority) {
    int idx = constrain(priority, 0, 5);
    const auto &a = LED_ALERT[idx];
    if (a.pulses == 0) return;
    for (uint8_t i = 0; i < a.pulses; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(a.onMs);
        digitalWrite(LED_PIN, LOW);
        if (a.offMs && i < a.pulses - 1) delay(a.offMs);
    }
}

// ============================================================
// TIER DISPLAY HELPERS
// ============================================================

uint16_t getTierColor(int priority) {
    switch (priority) {
        case 5: return COL_TIER5;
        case 4: return COL_TIER4;
        case 3: return COL_TIER3;
        case 2: return COL_TIER2;
        default: return COL_TIER1;
    }
}

const char* getTierLabel(int priority) {
    switch (priority) {
        case 5: return "HIGH VALUE TARGET";
        case 4: return "SURVEILLANCE INFRA";
        case 3: return "VEHICLE CCTV";
        case 2: return "CONSUMER SURVEIL";
        default: return "BASELINE";
    }
}

int getTierStars(int priority) {
    return constrain(priority, 1, 5);
}

// ============================================================
// BLE SCAN CALLBACK
// ============================================================

class MyAdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        checkOUI(advertisedDevice->getAddress().toString().c_str(), advertisedDevice->getRSSI(), true);
    }
};

// ============================================================
// FREERTOS TASKS
// ============================================================

void ScanTask(void *pvParameters) {
    for (;;) {
        if (config.setupComplete && millis() - lastScanTime >= (unsigned long)scanInterval) {
            scanning = true;
            if (config.enableBLE) {
                digitalWrite(LED_B_PIN, HIGH);  // Blue = BLE scan
                scanBLE();
                digitalWrite(LED_B_PIN, LOW);
            }
            if (config.enableWiFi) {
                digitalWrite(LED_G_PIN, HIGH);  // Green = WiFi scan
                scanWiFi();
                digitalWrite(LED_G_PIN, LOW);
            }
            scanning = false;
            lastScanTime = millis();

            // Run correlation engine after each scan cycle
            runCorrelationEngine();
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

        if (millis() - lastInteractionTime > (unsigned long)(config.sleepTimeout * 1000)) {
            enterDeepSleep();
        }

        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

// ============================================================
// TOUCH DRIVER (CST820)
// ============================================================

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
    // Landscape rotation mapping
    *x = rawY;
    *y = 239 - rawX;
    lastInteractionTime = millis();
    return true;
}

// ============================================================
// SETUP & LOOP
// ============================================================

void setup() {
    Serial.begin(115200);
    Serial.println("UK-OUI-SPY PRO v" VERSION " starting...");
    xDetectionMutex = xSemaphoreCreateMutex();

    // Pin setup
    pinMode(LED_PIN, OUTPUT);
    pinMode(LED_G_PIN, OUTPUT);
    pinMode(LED_B_PIN, OUTPUT);
    pinMode(TFT_BL, OUTPUT);
    pinMode(BAT_ADC, INPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);

#ifdef BOARD_CYD_2USB
    // CST820 reset sequence
    pinMode(TOUCH_RST, OUTPUT);
    digitalWrite(TOUCH_RST, LOW);
    delay(10);
    digitalWrite(TOUCH_RST, HIGH);
    delay(50);
#endif

    // Touch controller I2C init
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Wire.beginTransmission(TOUCH_ADDR);
    if (Wire.endTransmission() == 0) {
        i2cAvailable = true;
        Serial.println("CST820 touch found at 0x15");
    } else {
        Serial.println("CST820 touch NOT found - check wiring");
    }

    initDisplay();
    loadConfig();
    currentScreen = config.setupComplete ? SCREEN_MAIN : SCREEN_WIZARD;

    initSDCard();

    // Load OUI database (SD first, then static fallback)
    if (!loadOUIDatabaseFromSD("/oui.csv")) initializeStaticDatabase();

    // Load priority database (SD first, then static fallback)
    if (!loadPriorityDB("/priority.json")) initializeStaticPriorityDB();

    initBLE();
    initWiFi();

    lastInteractionTime = millis();
    xTaskCreatePinnedToCore(ScanTask, "ScanTask", 8192, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(UITask, "UITask", 12288, NULL, 1, NULL, 1);
}

void loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }

// ============================================================
// PERIPHERAL INIT
// ============================================================

void initDisplay() { tft.init(); tft.setRotation(1); tft.fillScreen(COL_BG); }
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

// ============================================================
// OUI CHECK WITH PRIORITY ENRICHMENT
// ============================================================

void checkOUI(String macAddress, int8_t rssi, bool isBLE) {
    String mac = macAddress;
    mac.toUpperCase();
    String oui = mac.substring(0, 8);

    // Check OUI database first
    auto ouiIt = ouiLookup.find(toOuiKey(oui));
    if (ouiIt == ouiLookup.end()) return;  // Not a known surveillance OUI

    // Build detection
    Detection det;
    det.macAddress = mac;
    det.manufacturer = ouiIt->second->manufacturer;
    det.category = ouiIt->second->category;
    det.relevance = ouiIt->second->relevance;
    det.rssi = rssi;
    det.timestamp = millis();
    det.firstSeen = millis();
    det.sightings = 1;
    det.isBLE = isBLE;
    det.priority = (det.relevance == REL_HIGH) ? 4 : (det.relevance == REL_MEDIUM) ? 3 : 2;
    det.context = "";
    det.correlationGroup = "";

    // Enrich with priority database
    auto priIt = priorityLookup.find(oui);
    if (priIt != priorityLookup.end()) {
        det.manufacturer = priIt->second->label;
        det.context = priIt->second->context;
        det.correlationGroup = priIt->second->correlationGroup;
        det.priority = priIt->second->priority;
    }

    addDetection(det);

    // Alert based on priority
    if (det.priority >= PRIORITY_MODERATE) {
        alertLED(det.priority);
    }

    // Log
    if (config.enableLogging && sdCardAvailable) {
        String logData = mac + "," + String(rssi) + "," + det.manufacturer +
                         "," + String(det.priority) + "," + det.context;
        if (config.secureLogging) encryptAndLog(logData);
        else {
            File f = SD.open("/detections.csv", FILE_APPEND);
            if (f) { f.println(logData); f.close(); }
        }
    }
}

void addDetection(Detection det) {
    xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
    bool found = false;
    for (auto &d : detections) {
        if (d.macAddress == det.macAddress) {
            d.rssi = det.rssi;
            d.timestamp = millis();
            d.sightings++;
            found = true;
            break;
        }
    }
    if (!found) {
        detections.insert(detections.begin(), det);
        if ((int)detections.size() > MAX_DETECTIONS) detections.pop_back();
    }
    // Sort by priority (highest first), then by RSSI (strongest first)
    std::sort(detections.begin(), detections.end(), [](const Detection& a, const Detection& b) {
        if (a.priority != b.priority) return a.priority > b.priority;
        return a.rssi > b.rssi;
    });
    xSemaphoreGive(xDetectionMutex);
}

// ============================================================
// DISPLAY SYSTEM
// ============================================================

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
    tft.fillRect(0, 0, 320, 28, COL_HEADER);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 6);
    tft.print(title);

    // Scanning indicator
    if (scanning) {
        tft.fillCircle(240, 14, 4, COL_ACCENT);
        tft.setTextSize(1);
        tft.setCursor(248, 10);
        tft.setTextColor(COL_ACCENT);
        tft.print("SCAN");
    }

    // Battery icon
    tft.drawRect(285, 7, 25, 12, TFT_WHITE);
    tft.fillRect(310, 10, 2, 6, TFT_WHITE);
    int batPct = constrain(map((int)(batteryVoltage * 100), 330, 420, 0, 100), 0, 100);
    int batWidth = map(batPct, 0, 100, 0, 21);
    uint16_t batColor = batPct < 20 ? TFT_RED : (batPct < 50 ? TFT_YELLOW : TFT_GREEN);
    tft.fillRect(287, 9, batWidth, 8, batColor);
}

void drawNavbar() {
    tft.fillRect(0, 210, 320, 30, COL_NAVBAR);
    tft.drawFastHLine(0, 210, 320, 0x2104);
    tft.setTextSize(1);
    const char* labels[] = {"LIST", "RADAR", "CONFIG", "INFO"};
    for (int i = 0; i < 4; i++) {
        bool active = (currentScreen == (Screen)(i + 1));
        if (active) {
            tft.fillRoundRect(5 + (i * 80), 212, 70, 24, 3, 0x1082);
        }
        tft.setTextColor(active ? COL_ACCENT : 0x7BEF);
        tft.setCursor(20 + (i * 80), 220);
        tft.print(labels[i]);
    }
}

// ============================================================
// TIERED MAIN SCREEN
// ============================================================

void drawMainScreen() {
    tft.startWrite();
    tft.fillScreen(COL_BG);
    drawHeader("SURVEILLANCE LIST");

    // Correlation alert banner (if active)
    int yStart = 30;
    if (!activeAlerts.empty()) {
        drawCorrelationBanner();
        yStart = 52;
    }

    // Take snapshot
    xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
    auto snapshot = detections;
    xSemaphoreGive(xDetectionMutex);

    // Filter baseline if setting is off
    if (!config.showBaseline) {
        snapshot.erase(
            std::remove_if(snapshot.begin(), snapshot.end(),
                [](const Detection& d) { return d.priority <= PRIORITY_BASELINE; }),
            snapshot.end()
        );
    }

    // Calculate scroll limits
    maxScroll = max(0, (int)snapshot.size() - 4);
    scrollOffset = constrain(scrollOffset, 0, maxScroll);

    // Draw tier headers and items
    int y = yStart;
    int currentTier = -1;
    int itemIndex = 0;

    for (int i = scrollOffset; i < (int)snapshot.size() && y < 200; i++) {
        auto& det = snapshot[i];

        // Draw tier separator when tier changes
        if (det.priority != currentTier) {
            currentTier = det.priority;
            uint16_t tierCol = getTierColor(currentTier);

            // Tier header bar
            tft.fillRect(0, y, 320, 14, 0x0000);
            tft.fillRect(2, y + 1, 3, 12, tierCol);
            tft.setTextSize(1);
            tft.setTextColor(tierCol);
            tft.setCursor(10, y + 3);

            // Stars
            for (int s = 0; s < getTierStars(currentTier); s++) {
                tft.print("*");
            }
            tft.printf(" %s", getTierLabel(currentTier));
            y += 16;
            if (y >= 200) break;
        }

        // Draw detection card
        uint16_t cardBg = (det.priority >= PRIORITY_CRITICAL) ? COL_CARD_HI : COL_CARD;
        tft.fillRoundRect(5, y, 310, 30, 3, cardBg);

        // Priority colour bar on left edge
        tft.fillRect(5, y, 4, 30, getTierColor(det.priority));

        // Protocol badge
        uint16_t badgeCol = det.isBLE ? 0x001F : 0x07E0;  // Blue for BLE, Green for WiFi
        tft.fillRoundRect(270, y + 3, 40, 12, 2, badgeCol);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(275, y + 5);
        tft.print(det.isBLE ? "BLE" : "WiFi");

        // Manufacturer name
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(14, y + 4);
        String displayName = det.manufacturer;
        if (displayName.length() > 28) displayName = displayName.substring(0, 25) + "...";
        tft.print(displayName);

        // Second line: MAC + RSSI + sightings
        tft.setTextColor(COL_DIMTEXT);
        tft.setCursor(14, y + 17);
        tft.printf("%s  %ddBm", det.macAddress.substring(0, 8).c_str(), det.rssi);
        if (det.sightings > 1) {
            tft.printf("  x%d", det.sightings);
        }

        y += 33;
        itemIndex++;
    }

    // Scroll indicator
    if (maxScroll > 0) {
        int barHeight = max(10, 170 / max(1, (int)snapshot.size()));
        int barY = map(scrollOffset, 0, maxScroll, yStart, 200 - barHeight);
        tft.fillRoundRect(316, barY, 3, barHeight, 1, COL_ACCENT);
    }

    // Empty state
    if (snapshot.empty()) {
        tft.setTextSize(1);
        tft.setTextColor(COL_DIMTEXT);
        tft.setCursor(80, 110);
        tft.print("Scanning for devices...");
    }

    drawNavbar();
    tft.endWrite();
}

// ============================================================
// CORRELATION ALERT BANNER
// ============================================================

void drawCorrelationBanner() {
    if (activeAlerts.empty()) return;

    auto& alert = activeAlerts[0];  // Show most recent
    uint16_t bgCol = (alert.alertLevel == "CRITICAL") ? COL_ALERT_BG : 0x4200;

    tft.fillRect(0, 29, 320, 22, bgCol);

    // Flashing indicator
    bool blink = (millis() / 300) % 2;
    if (blink) {
        tft.fillCircle(12, 40, 4, COL_TIER5);
    }

    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(22, 33);
    tft.printf("[%s]", alert.alertLevel.c_str());
    tft.setTextColor(COL_TIER5);
    tft.setCursor(22, 43);
    tft.print(alert.name);
}

// ============================================================
// RADAR SCREEN (Premium Radar-Style Map)
// ============================================================

void drawRadarScreen() {
    tft.startWrite();
    tft.fillScreen(COL_BG);
    drawHeader("PROXIMITY RADAR");

    int cx = 160, cy = 118, r = 80;

    // Radar rings with distance labels
    for (int ring = 1; ring <= 3; ring++) {
        int rr = (r * ring) / 3;
        tft.drawCircle(cx, cy, rr, 0x1082);
    }
    // Crosshairs
    tft.drawLine(cx - r, cy, cx + r, cy, 0x1082);
    tft.drawLine(cx, cy - r, cx, cy + r, 0x1082);

    // Distance labels
    tft.setTextSize(1);
    tft.setTextColor(0x4208);
    tft.setCursor(cx + r/3 + 2, cy + 2); tft.print("10m");
    tft.setCursor(cx + 2*r/3 + 2, cy + 2); tft.print("30m");
    tft.setCursor(cx + r + 2, cy + 2); tft.print("50m+");

    // Centre dot (you)
    tft.fillCircle(cx, cy, 3, COL_ACCENT);

    // Plot detections
    xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
    auto snapshot = detections;
    xSemaphoreGive(xDetectionMutex);

    for (auto& det : snapshot) {
        if (!config.showBaseline && det.priority <= PRIORITY_BASELINE) continue;

        // Distance from RSSI
        float dist = map(constrain(det.rssi, -100, -30), -100, -30, r, 5);

        // Deterministic angle from MAC hash
        unsigned long hash = 0;
        for (char c : det.macAddress) hash = hash * 31 + c;
        float angle = (float)(hash % 360) * PI / 180.0;

        int px = cx + (int)(dist * cos(angle));
        int py = cy + (int)(dist * sin(angle));

        // Draw device dot sized by priority
        int dotSize = (det.priority >= 4) ? 5 : 3;
        tft.fillCircle(px, py, dotSize, getTierColor(det.priority));

        // Label for high-value targets
        if (det.priority >= PRIORITY_HIGH) {
            tft.setTextSize(1);
            tft.setTextColor(getTierColor(det.priority));
            String shortName = det.manufacturer.substring(0, 8);
            tft.setCursor(px + dotSize + 2, py - 3);
            tft.print(shortName);
        }
    }

    // Correlation alert overlay on radar
    if (!activeAlerts.empty()) {
        tft.setTextSize(1);
        tft.setTextColor(COL_TIER5);
        tft.setCursor(5, 200);
        tft.printf("ALERT: %s", activeAlerts[0].name.c_str());
    }

    drawNavbar();
    tft.endWrite();
}

// ============================================================
// SETTINGS SCREEN
// ============================================================

void drawSettingsScreen() {
    tft.startWrite();
    tft.fillScreen(COL_BG);
    drawHeader("CONFIGURATION");
    drawToggle(20, 45, config.enableBLE, "BLE Scanning");
    drawToggle(20, 72, config.enableWiFi, "WiFi Scanning");
    drawToggle(20, 99, config.enableLogging, "SD Logging");
    drawToggle(20, 126, config.secureLogging, "Secure (AES)");
    drawToggle(20, 153, config.autoBrightness, "Auto-Brightness");
    drawToggle(20, 180, config.showBaseline, "Show Baseline");
    drawNavbar();
    tft.endWrite();
}

void drawToggle(int x, int y, bool state, const char* label) {
    // Toggle track
    tft.fillRoundRect(x + 150, y, 44, 22, 11, state ? COL_ACCENT : 0x4208);
    // Toggle knob
    int knobX = state ? (x + 150 + 24) : (x + 150 + 4);
    tft.fillCircle(knobX + 7, y + 11, 8, TFT_WHITE);
    // Label
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(x, y + 6);
    tft.print(label);
}

// ============================================================
// INFO SCREEN
// ============================================================

void drawInfoScreen() {
    tft.startWrite();
    tft.fillScreen(COL_BG);
    drawHeader("SYSTEM STATUS");
    tft.setTextSize(1);

    int y = 40;
    auto drawRow = [&](const char* label, String value) {
        tft.setTextColor(COL_DIMTEXT);
        tft.setCursor(20, y);
        tft.print(label);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(170, y);
        tft.print(value);
        y += 18;
    };

    drawRow("Firmware:", VERSION);
    drawRow("Battery:", String(batteryVoltage, 2) + " V");
    drawRow("OUI Database:", String(dynamicDatabase.size()) + " entries");
    drawRow("Priority DB:", String(priorityDB.size()) + " entries");
    drawRow("Corr. Rules:", String(correlationRules.size()) + " rules");
    drawRow("Active Alerts:", String(activeAlerts.size()));
    drawRow("Detections:", String(detections.size()));
    drawRow("Free Memory:", String(ESP.getFreeHeap() / 1024) + " KB");
    drawRow("Touch:", i2cAvailable ? "OK" : "ERROR");
    drawRow("SD Card:", sdCardAvailable ? "OK" : "NOT FOUND");

    drawNavbar();
    tft.endWrite();
}

// ============================================================
// WIZARD SCREEN
// ============================================================

void drawWizardScreen() {
    tft.startWrite();
    tft.fillScreen(COL_BG);
    drawHeader("WELCOME");
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    if (wizardStep == 0) {
        tft.setCursor(20, 60); tft.print("UK-OUI-SPY PRO EDITION v" VERSION);
        tft.setCursor(20, 80); tft.print("Professional Surveillance Detection");
        tft.setCursor(20, 110); tft.setTextColor(COL_DIMTEXT);
        tft.print("Tiered Priority System with");
        tft.setCursor(20, 125); tft.print("Correlation Detection Engine");
        tft.setCursor(20, 155); tft.setTextColor(TFT_WHITE);
        tft.print("Tap 'NEXT' to begin setup.");
    } else if (wizardStep == 1) {
        tft.setCursor(20, 60); tft.print("HARDWARE CHECK");
        tft.setCursor(20, 85); tft.printf("Touch: %s", i2cAvailable ? "OK" : "ERROR");
        tft.setCursor(20, 100); tft.printf("SD Card: %s", sdCardAvailable ? "DETECTED" : "NOT FOUND");
        tft.setCursor(20, 115); tft.printf("OUI DB: %d entries", dynamicDatabase.size());
        tft.setCursor(20, 130); tft.printf("Priority DB: %d entries", priorityDB.size());
        tft.setCursor(20, 145); tft.printf("Corr. Rules: %d", correlationRules.size());
    } else {
        tft.setCursor(20, 60); tft.print("SETUP COMPLETE");
        tft.setCursor(20, 85); tft.setTextColor(COL_ACCENT);
        tft.print("Device is ready for field use.");
        tft.setCursor(20, 110); tft.setTextColor(COL_DIMTEXT);
        tft.print("Baseline devices will be filtered.");
        tft.setCursor(20, 125); tft.print("Change in CONFIG > Show Baseline.");
        tft.setCursor(20, 155); tft.setTextColor(TFT_WHITE);
        tft.print("Tap 'FINISH' to start scanning.");
    }
    tft.fillRoundRect(220, 170, 80, 28, 4, COL_ACCENT);
    tft.setTextColor(COL_BG);
    tft.setCursor(232, 178);
    tft.print(wizardStep < 2 ? "NEXT" : "FINISH");
    tft.endWrite();
}

// ============================================================
// TOUCH HANDLING
// ============================================================

void handleTouchGestures() {
    static unsigned long lastTouch = 0;
    uint16_t x, y;
    if (!readCapacitiveTouch(&x, &y)) return;
    if (millis() - lastTouch < 250) return;  // Debounce
    lastTouch = millis();

    if (currentScreen == SCREEN_WIZARD) {
        if (x > 220 && y > 170) {
            wizardStep++;
            if (wizardStep > 2) {
                config.setupComplete = true;
                saveConfig();
                currentScreen = SCREEN_MAIN;
            }
        }
    } else if (y > 210) {
        // Navbar tap
        int newScreen = x / 80;
        if (newScreen >= 0 && newScreen <= 3) {
            currentScreen = (Screen)(newScreen + 1);
            scrollOffset = 0;  // Reset scroll on page change
        }
    } else if (currentScreen == SCREEN_MAIN) {
        // Scroll: top half = scroll up, bottom half = scroll down
        if (y < 120 && scrollOffset > 0) {
            scrollOffset--;
        } else if (y >= 120 && y < 210 && scrollOffset < maxScroll) {
            scrollOffset++;
        }
    } else if (currentScreen == SCREEN_SETTINGS) {
        // Toggle taps (hit area on the toggle switches)
        if (x > 150 && x < 220) {
            if (y > 45 && y < 67) config.enableBLE = !config.enableBLE;
            else if (y > 72 && y < 94) config.enableWiFi = !config.enableWiFi;
            else if (y > 99 && y < 121) config.enableLogging = !config.enableLogging;
            else if (y > 126 && y < 148) config.secureLogging = !config.secureLogging;
            else if (y > 153 && y < 175) config.autoBrightness = !config.autoBrightness;
            else if (y > 180 && y < 202) config.showBaseline = !config.showBaseline;
            saveConfig();
        }
    }
}

// ============================================================
// UTILITIES
// ============================================================

void setBrightness(int level) {
    config.brightness = constrain(level, 0, 255);
    #if TFT_BACKLIGHT_ON == HIGH
    analogWrite(TFT_BL, config.brightness);
    #else
    analogWrite(TFT_BL, 255 - config.brightness);
    #endif
}

float readBattery() {
    int raw = analogRead(BAT_ADC);
    return (raw / 4095.0) * 2.0 * 3.3 * 1.1;
}

void encryptAndLog(String data) {
    mbedtls_aes_context aes;
    unsigned char key[16];
    memcpy(key, config.encryptionKey, 16);

    // Multi-block encryption for longer data
    int dataLen = data.length();
    int blocks = (dataLen + 15) / 16;

    File f = SD.open("/secure.log", FILE_APPEND);
    if (!f) return;

    // Write block count header
    f.write((uint8_t)blocks);

    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 128);

    for (int b = 0; b < blocks; b++) {
        unsigned char input[16];
        unsigned char output[16];
        memset(input, 0, 16);
        int offset = b * 16;
        int copyLen = min(16, dataLen - offset);
        if (copyLen > 0) memcpy(input, data.c_str() + offset, copyLen);
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, input, output);
        f.write(output, 16);
    }

    mbedtls_aes_free(&aes);
    f.close();
}

void saveConfig() {
    preferences.begin("uk-oui-spy", false);
    preferences.putBool("setup", config.setupComplete);
    preferences.putBool("ble", config.enableBLE);
    preferences.putBool("wifi", config.enableWiFi);
    preferences.putBool("log", config.enableLogging);
    preferences.putBool("secure", config.secureLogging);
    preferences.putBool("auto", config.autoBrightness);
    preferences.putBool("baseline", config.showBaseline);
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
    config.showBaseline = preferences.getBool("baseline", false);
    preferences.end();
}

void enterDeepSleep() {
    tft.fillScreen(COL_BG);
    tft.setTextSize(1);
    tft.setTextColor(COL_DIMTEXT);
    tft.setCursor(100, 115);
    tft.print("SLEEPING...");
    delay(500);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_OFF);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(LED_G_PIN, LOW);
    digitalWrite(LED_B_PIN, LOW);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_SDA, 0);
    esp_deep_sleep_start();
}
