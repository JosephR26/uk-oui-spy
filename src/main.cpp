/*
 * UK-OUI-SPY PRO EDITION v3.0.0
 * Professional UK Surveillance Device Detector
 *
 * Hardware: ESP32-2432S028 (2.8" ILI9341 TFT, XPT2046 resistive touch)
 * Features: Tiered Priority Display, Correlation Detection Engine,
 *           WiFi Promiscuous Sniffing, FreeRTOS Dual-Core,
 *           Radar Visualization, Setup Wizard, SD Session Logging.
 */

#define VERSION "3.4.0-PRO"

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <SD.h>
#include <SPI.h>
#include <vector>
#include <algorithm>
#include <map>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "esp_sleep.h"
#include <time.h>
#include "oui_database.h"
#include "wifi_promiscuous.h"
#include "web_portal.h"

// Web Portal AP Configuration
#define AP_SSID     "OUI-SPY-PRO"
#define AP_PASS     "spypro2026"
#define AP_CHANNEL  6
#define AP_MAX_CONN 4

// ============================================================
// HARDWARE PIN DEFINITIONS
// ============================================================

// XPT2046 resistive touch on HSPI (display uses VSPI internally via TFT_eSPI)
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

// ============================================================
// TOUCH CALIBRATION CONSTANTS
// Source: Fr4nkFletcher/ESP32-Marauder-Cheap-Yellow-Display
//   Display.cpp calData for CYD_28: { 350, 3465, 188, 3431, 2 }
//   (raw_x: 350–3465 | raw_y: 188–3431 | flags: y-inverted)
//
// XPT2046_Touchscreen at rotation=1 transforms raw ADC as:
//   p.x = 4096 - raw_y   →  4096-3431 = 665   to  4096-188 = 3908
//   p.y = raw_x           →  350  to  3465
//
// Stored in NVS via saveConfig/loadConfig so they survive reboot
// and can be updated in the field without reflashing.
// ============================================================
#define TOUCH_CAL_X_MIN_DEFAULT   244   // measured: TL px  (left edge)
#define TOUCH_CAL_X_MAX_DEFAULT  3742   // measured: BR px  (right edge)
#define TOUCH_CAL_Y_MIN_DEFAULT   333   // measured: TL py  (top edge)
#define TOUCH_CAL_Y_MAX_DEFAULT  3348   // measured: BL py  (bottom edge)

// Backlight polarity (derived from platformio.ini)
#ifndef TFT_BACKLIGHT_ON
#define TFT_BACKLIGHT_ON HIGH
#endif
#if TFT_BACKLIGHT_ON == HIGH
#define TFT_BACKLIGHT_OFF LOW
#else
#define TFT_BACKLIGHT_OFF HIGH
#endif

// SD card — VSPI bus (separate from display HSPI)
#define SD_CS    5
#define SD_MOSI  23
#define SD_MISO  19
#define SD_SCLK  18

#define LED_PIN 4
#define LED_G_PIN 16
#define LED_B_PIN 17
#define LDR_PIN 34
#define BAT_ADC 35
#define BOOT_BTN 0    // BOOT button (active LOW, has internal pull-up)

SPIClass sdSPI(VSPI);
SPIClass touchscreenSPI(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

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
// Uses char[] instead of String to avoid 732 separate heap allocations for 183 entries.
// The entire priorityDB vector becomes one contiguous block — no fragmentation.
struct PriorityEntry {
    char oui[9];
    char label[32];
    char context[48];
    char correlationGroup[24];
    int priority;
    float confidence;
    PriorityEntry() { oui[0]=0; label[0]=0; context[0]=0; correlationGroup[0]=0; priority=0; confidence=0.0f; }
    PriorityEntry(const char* o, const char* l, const char* c, const char* g, int p, float f) {
        strncpy(oui, o, 8);  oui[8]=0;
        strncpy(label, l, 31); label[31]=0;
        strncpy(context, c, 47); context[47]=0;
        strncpy(correlationGroup, g, 23); correlationGroup[23]=0;
        priority=p; confidence=f;
    }
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

// BLE extra data extracted from advertisement packet
struct BLEMeta {
    String company;      // from manufacturer specific data (2-byte BT SIG company ID)
    String svcHint;      // classified from 16-bit service UUIDs
    int8_t txPower = 0;
    bool hasTxPower = false;
    bool publicAddr = false;   // true = public/OUI-resolvable, false = random
};

// Forward-declared here so SD lookup functions can reference it before the global block
bool sdCardAvailable = false;

// Bluetooth SIG assigned company IDs (most common consumer/surveillance relevant)
// ── BT SIG company lookup ─────────────────────────────────────────────────
// Primary: SD /btcompany.bin binary search (32-byte records: 2-byte LE ID + 30-byte name)
// Fallback: small RAM table for the most common devices when SD unavailable
#define BT_RECORD_SIZE 32
#define BT_NAME_SIZE   30

std::map<uint16_t, String> btCache;  // LRU cache, max 64 entries

String sdLookupBTCompany(uint16_t companyId) {
    if (!sdCardAvailable) return "";
    auto it = btCache.find(companyId);
    if (it != btCache.end()) return it->second;

    File f = SD.open("/btcompany.bin", FILE_READ);
    if (!f) return "";

    uint32_t count = 0;
    f.read((uint8_t*)&count, 4);

    String result = "";
    int32_t lo = 0, hi = (int32_t)count - 1;
    uint8_t rec[BT_RECORD_SIZE];
    while (lo <= hi) {
        int32_t mid = (lo + hi) / 2;
        f.seek(4 + (uint32_t)mid * BT_RECORD_SIZE);
        if (f.read(rec, BT_RECORD_SIZE) != BT_RECORD_SIZE) break;
        uint16_t recId = rec[0] | ((uint16_t)rec[1] << 8);
        if (recId == companyId) {
            rec[BT_RECORD_SIZE - 1] = '\0';
            result = String((char*)(rec + 2));
            Serial.printf("[BT-SD] 0x%04X -> %s\n", companyId, result.c_str());
            break;
        } else if (recId < companyId) { lo = mid + 1; }
        else                           { hi = mid - 1; }
    }
    f.close();
    if (btCache.size() >= 64) btCache.erase(btCache.begin());
    btCache[companyId] = result;
    return result;
}

// RAM fallback — most common BLE manufacturers
struct BTCompany { uint16_t id; const char* name; };
static const BTCompany BT_COMPANY_DB[] = {
    {0x0002,"Intel"},        {0x0006,"Microsoft"},  {0x000F,"Broadcom"},
    {0x0013,"Texas Instr."}, {0x0019,"Qualcomm"},   {0x0024,"STMicro"},
    {0x0046,"Parrot"},       {0x0059,"Nordic Semi"},{0x0075,"Samsung"},
    {0x004C,"Apple"},        {0x00E0,"Google"},     {0x011B,"Amazon"},
    {0x0138,"Xiaomi"},       {0x02E5,"Espressif"},  {0x1049,"DJI Drone"},
};
static const int BT_COMPANY_COUNT = sizeof(BT_COMPANY_DB)/sizeof(BT_COMPANY_DB[0]);

String lookupBTCompany(uint16_t id) {
    // SD first, then RAM fallback
    String sdResult = sdLookupBTCompany(id);
    if (!sdResult.isEmpty()) return sdResult;
    for (int i = 0; i < BT_COMPANY_COUNT; i++) {
        if (BT_COMPANY_DB[i].id == id) return String(BT_COMPANY_DB[i].name);
    }
    return "";
}

// Map common 16-bit BLE service UUIDs to human-readable device type hints
String getBLESvcHint(uint16_t uuid) {
    switch (uuid) {
        case 0x1812: return "Input Device";
        case 0x180D: return "Wearable";
        case 0x1816: return "Cycling";
        case 0x1819: return "GPS/Location";
        case 0x181A: return "Env Sensor";
        case 0x180F: return "Battery Dev";
        case 0x1826: return "Fitness";
        case 0xFD5A: return "Tile Tracker";
        case 0xFD6F: return "Apple Proximity";
        case 0xFE9F: return "Google Nearby";
        case 0xFEAA: return "Eddystone Beacon";
        case 0xFE56: return "Camera (GoPro)";
        case 0xFEBB: return "Body Camera";      // Motorola Solutions PMLN profile
        case 0x1804: return "TX Power";
        case 0x1805: return "Current Time";     // common in body cams / comms devices
        case 0x180A: return "Device Info";
        case 0xFE03: return "ANT+ Sensor";
        case 0xFE8A: return "Body Camera";    // Axon/TASER body-worn camera profile
        case 0xFCA0: return "Drone Control";  // DJI proprietary service
        case 0xFD82: return "Security Dev";   // Generic security device marker
        case 0xFEC0: return "Law Enforce";    // Law enforcement comms device
        case 0xFD5C: return "Asset Tracker";  // Samsung SmartTag / Galaxy
        case 0xFC3D: return "AirTag";         // Apple AirTag precision-finding
        case 0xFEDF: return "Tile Tracker";   // Tile Bluetooth tracker (alt UUID)
        case 0x1800: return "Generic Access";
        case 0x1802: return "Alert Device";   // Immediate Alert — body-worn alarm
        case 0x1803: return "Link Loss Dev";  // Link Loss — proximity badge/tracker
        default:     return "";
    }
}

// Enhanced detection with priority
struct Detection {
    String macAddress;
    String manufacturer;
    String ssid;           // WiFi SSID or BLE advertised name
    String bleCompany;     // BT SIG company (from manufacturer specific data)
    String bleSvcHint;     // classified from service UUIDs
    String context;
    String correlationGroup;
    DeviceCategory category;
    RelevanceLevel relevance;
    int priority;
    int8_t rssi;
    int8_t txPower = 0;
    bool hasTxPower = false;
    bool blePublicAddr = false;
    unsigned long timestamp;
    unsigned long firstSeen;
    int sightings;
    bool isBLE;
    DeploymentType deployment = DEPLOY_PRIVATE;
    float confidence = 0.0f;
    uint8_t channel = 0;      // WiFi channel (0 = unknown/BLE)
    int threatScore = 0;      // Composite score 0-100 (priority + proximity + persistence + confidence)
};

struct Config {
    ScanMode scanMode = SCAN_NORMAL;
    AlertMode alertMode = ALERT_LED;
    bool enableBLE = true;
    bool enableWiFi = true;
    bool enableLogging = true;
    bool showBaseline = true;   // Show all devices by default
    bool enableWebPortal = true;
    int brightness = 255;
    bool autoBrightness = true;
    char encryptionKey[17] = "UK-OUI-SPY-2026";
    char apPassword[20]    = "spypro2026";       // web portal hotspot password (8-19 chars)
    bool setupComplete = false;
    int sleepTimeout = 1800;  // seconds — default 30 min, persisted in NVS
    // Touch calibration — Fr4nkFletcher CYD_28 validated defaults
    int calXMin = TOUCH_CAL_X_MIN_DEFAULT;
    int calXMax = TOUCH_CAL_X_MAX_DEFAULT;
    int calYMin = TOUCH_CAL_Y_MIN_DEFAULT;
    int calYMax = TOUCH_CAL_Y_MAX_DEFAULT;
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
AsyncWebServer webServer(80);
DNSServer dnsServer;
bool webPortalActive = false;

const int MAX_DETECTIONS = 50;
const int MAX_ALERTS = 5;
unsigned long lastScanTime = 0;
unsigned long lastInteractionTime = 0;
int scanInterval = 5000;
bool scanning = false;
char sessionId[10] = "----";      // generated at boot from esp_random()
char sessionLogPath[32] = "/detections.csv";  // set to /sessions/XXXX-XXXX.csv after SD init
char wifiSsid[64] = "";           // loaded from /wifi.txt on SD card
char wifiPass[64] = "";
bool staCredentialsFound = false;

// ============================================================
// SD OUI LOOKUP  (binary search on /oui.bin — 35 bytes/record)
// Format: 4-byte LE count header + records sorted by 3-byte OUI
// ============================================================
#define OUI_RECORD_SIZE 35
std::map<String, String> ouiCache;   // lookup cache, max 64 entries

String sdLookupOUI(const String& oui) {
    // oui must be "XX:XX:XX" uppercase 8-char string
    if (!sdCardAvailable) return "";

    auto it = ouiCache.find(oui);
    if (it != ouiCache.end()) return it->second;

    uint8_t target[3];
    target[0] = (uint8_t)strtol(oui.substring(0, 2).c_str(), nullptr, 16);
    target[1] = (uint8_t)strtol(oui.substring(3, 5).c_str(), nullptr, 16);
    target[2] = (uint8_t)strtol(oui.substring(6, 8).c_str(), nullptr, 16);

    File f = SD.open("/oui.bin", FILE_READ);
    if (!f) { Serial.printf("[OUI-SD] ERR: /oui.bin open failed for %s\n", oui.c_str()); return ""; }

    uint32_t count = 0;
    f.read((uint8_t*)&count, 4);

    String result = "";
    int32_t lo = 0, hi = (int32_t)count - 1;
    uint8_t rec[OUI_RECORD_SIZE];

    while (lo <= hi) {
        int32_t mid = (lo + hi) / 2;
        f.seek(4 + (uint32_t)mid * OUI_RECORD_SIZE);
        if (f.read(rec, OUI_RECORD_SIZE) != OUI_RECORD_SIZE) break;
        int cmp = memcmp(target, rec, 3);
        if (cmp == 0) {
            rec[OUI_RECORD_SIZE - 1] = '\0';
            result = String((char*)(rec + 3));
            Serial.printf("[OUI-SD] %s -> %s\n", oui.c_str(), result.c_str());
            break;
        } else if (cmp < 0) { hi = mid - 1; }
        else                 { lo = mid + 1; }
    }
    f.close();

    if (ouiCache.size() >= 64) ouiCache.erase(ouiCache.begin());
    ouiCache[oui] = result;
    return result;
}

float batteryVoltage = 0.0;
bool touchAvailable = false;
int wizardStep = 0;
int scrollOffset = 0;
int maxScroll = 0;

// Display dirty-flag system — only redraw when state changes.
// Eliminates the full-screen clear every 30 ms that causes flicker.
volatile bool displayDirty = true;

// Scan statistics for display feedback
volatile int lastBLECount = 0;
volatile int lastWiFiCount = 0;
volatile int totalScanned = 0;
volatile int totalMatched = 0;
volatile int totalEvicted = 0;  // devices silently dropped when detection buffer was full

// ============================================================
// FUNCTION PROTOTYPES
// ============================================================

void initDisplay();
void initTouch();
void initBLE();
void initWiFi();
void initSDCard();
void scanBLE();
void scanWiFi();
void checkOUI(String macAddress, int8_t rssi, bool isBLE, String name = "", BLEMeta* bleMeta = nullptr, uint8_t channel = 0);
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
void handleBootButton();
void setBrightness(int level);
void drawToggle(int x, int y, bool state, const char* label);
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
void setupWebServer();
int computeThreatScore(const Detection& det);

// ============================================================
// PRIORITY DATABASE LOADER
// ============================================================

bool loadPriorityDB(const char* path) {
    if (!SD.exists(path)) return false;
    File file = SD.open(path);
    if (!file) return false;

    // Allocate JSON document — 32768 comfortably fits 183-entry priority.json
    // (65536 caused heap fragmentation crash on 2nd WiFi promiscuous cycle)
    DynamicJsonDocument doc(32768);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.printf("[WARN] Priority JSON parse error: %s — static fallback\n", error.c_str());
        return false;
    }

    priorityDB.clear();
    priorityLookup.clear();
    priorityDB.reserve(200);  // one allocation for the whole vector, no realloc churn

    // Load entries
    JsonArray entries = doc["entries"];
    for (JsonObject entry : entries) {
        PriorityEntry pe;
        strncpy(pe.oui,              entry["oui"]               | "", 8);  pe.oui[8]=0;
        strncpy(pe.label,            entry["label"]             | "", 31); pe.label[31]=0;
        strncpy(pe.context,          entry["context"]           | "", 47); pe.context[47]=0;
        strncpy(pe.correlationGroup, entry["correlation_group"] | "", 23); pe.correlationGroup[23]=0;
        pe.priority   = entry["priority"].as<int>();
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

    if (priorityDB.empty()) {
        Serial.println("[WARN] Priority JSON has no entries — static fallback");
        return false;
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
    priorityDB.push_back({"28:87:BA", "GoPro", "Action cams/bodycam", "gopro_cam", 3, 0.92});
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

    // Compute a hash of the current alerts so we can detect content changes,
    // not just count changes (e.g. different alerts with the same count).
    auto hashAlerts = [](const std::vector<CorrelationAlert>& alerts) -> uint32_t {
        uint32_t h = 0;
        for (auto& a : alerts) {
            for (char c : a.name) h = h * 31 + c;
            for (char c : a.alertLevel) h = h * 31 + c;
        }
        return h;
    };
    uint32_t prevHash = hashAlerts(activeAlerts);

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
    if (hashAlerts(activeAlerts) != prevHash) {
        displayDirty = true;
    }
}

// ============================================================
// ALERT SYSTEM
// ============================================================

// LED alert patterns — red flash only, scaled by severity.
// Indices must match PRIORITY_* constants (1–5).  Tiers below MODERATE are silent.
static const struct { uint16_t onMs; uint16_t offMs; uint8_t pulses; } LED_ALERT[] = {
    [0]                  = {  0,  0, 0},
    [PRIORITY_BASELINE]  = {  0,  0, 0},
    [PRIORITY_LOW]       = {  0,  0, 0},
    [PRIORITY_MODERATE]  = {100,  0, 1},
    [PRIORITY_HIGH]      = {200,  0, 1},
    [PRIORITY_CRITICAL]  = { 50, 50, 5},
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
// COMPOSITE THREAT SCORE  (0 – 100)
// ============================================================
//
// Factors:
//   Priority tier   (0–75):  the OUI/priority tier, weighted heaviest
//   RSSI proximity  (0–15):  closer = higher threat (stationary near you)
//   Persistence     (0– 5):  repeated sightings = device is dwelling, not passing
//   Confidence      (0– 5):  from priority DB confidence rating
//
// Score drives display sort order so the most actionable devices
// always appear at the top regardless of discovery time.
// ============================================================

int computeThreatScore(const Detection& det) {
    // Base from priority tier (5 tiers × 15 = max 75)
    int score = det.priority * 15;

    // Proximity bonus from RSSI
    if      (det.rssi >= -50) score += 15; // Very close (<5m typical)
    else if (det.rssi >= -60) score += 10; // Close     (~10m)
    else if (det.rssi >= -70) score +=  5; // Medium    (~20m)
    else if (det.rssi >= -80) score +=  2; // Far       (~50m)
    // <-80 dBm = distant / passing, no bonus

    // Persistence bonus — device staying in range across multiple scan cycles
    if      (det.sightings >= 10) score += 5;
    else if (det.sightings >=  5) score += 3;
    else if (det.sightings >=  2) score += 1;

    // Confidence bonus from priority database rating
    score += (int)(det.confidence * 5.0f);

    return constrain(score, 0, 100);
}

// ============================================================
// BLE SCAN CALLBACK
// ============================================================

class MyAdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        BLEMeta meta;

        // Device name
        String bleName = advertisedDevice->haveName() ? advertisedDevice->getName().c_str() : "";

        // Manufacturer specific data → Bluetooth SIG company ID
        if (advertisedDevice->haveManufacturerData()) {
            std::string mfrData = advertisedDevice->getManufacturerData();
            if (mfrData.size() >= 2) {
                uint16_t companyId = (uint8_t)mfrData[0] | ((uint8_t)mfrData[1] << 8);
                meta.company = lookupBTCompany(companyId);
                if (meta.company.isEmpty()) {
                    char buf[8]; snprintf(buf, sizeof(buf), "BT:%04X", companyId);
                    meta.company = String(buf);
                }
            }
        }

        // Service UUIDs → device type hint
        for (int i = 0; i < (int)advertisedDevice->getServiceUUIDCount(); i++) {
            NimBLEUUID u = advertisedDevice->getServiceUUID(i);
            if (u.bitSize() == 16) {
                meta.svcHint = getBLESvcHint(u.getNative()->u16.value);
                if (!meta.svcHint.isEmpty()) break;
            }
        }

        // TX power (for distance estimate)
        meta.hasTxPower = advertisedDevice->haveTXPower();
        if (meta.hasTxPower) meta.txPower = (int8_t)advertisedDevice->getTXPower();

        // Address type
        meta.publicAddr = (advertisedDevice->getAddressType() == BLE_ADDR_PUBLIC);

        checkOUI(advertisedDevice->getAddress().toString().c_str(),
                 advertisedDevice->getRSSI(), true, bleName, &meta);
    }
};

// ============================================================
// WIFI PROMISCUOUS CALLBACK
// ============================================================

void onPromiscuousPacket(const uint8_t* mac, int8_t rssi, uint8_t channel, const char* ssid) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    // ssid is non-null (may be empty string) — pass through for AP/beacon identification
    checkOUI(String(macStr), rssi, false, String(ssid), nullptr, channel);
}

// ============================================================
// FREERTOS TASKS
// ============================================================

void ScanTask(void *pvParameters) {
    static int sdCheckCounter = 0;
    for (;;) {
        // Periodic SD health check: attempt remount if card was reinserted
        if (++sdCheckCounter >= 10) {
            sdCheckCounter = 0;
            if (!sdCardAvailable) {
                if (SD.begin(SD_CS, sdSPI)) {
                    sdCardAvailable = true;
                    ouiCache.clear();  // force re-lookup with newly mounted card
                    btCache.clear();
                    Serial.println("[SD] Card remounted — caches cleared");
                }
            }
        }

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
                // Promiscuous mode: catches probe requests, hidden APs, non-broadcasting devices
                startWiFiPromiscuous(onPromiscuousPacket);
                scanAllChannels(onPromiscuousPacket, 150);
                stopWiFiPromiscuous();
                digitalWrite(LED_G_PIN, LOW);
            }
            scanning = false;
            lastScanTime = millis();

            // Run correlation engine after each scan cycle
            runCorrelationEngine();

            Serial.printf("[SCAN] Cycle complete: BLE=%d WiFi=%d | Total=%d matched=%d | Detections=%d\n",
                          lastBLECount, lastWiFiCount, totalScanned, totalMatched, detections.size());
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void UITask(void *pvParameters) {
    Serial.println("[UI] UITask started on core " + String(xPortGetCoreID()));
    unsigned long uiLoopCount = 0;
    for (;;) {
        if (uiLoopCount < 3 || uiLoopCount % 100 == 0) {
            Serial.printf("[UI] loop #%lu screen=%d dirty=%d BOOT=%s\n",
                          uiLoopCount, (int)currentScreen, (int)displayDirty,
                          digitalRead(BOOT_BTN) == LOW ? "PRESSED" : "idle");
        }
        uiLoopCount++;
        handleTouchGestures();
        handleBootButton();
        if (config.autoBrightness) {
            static unsigned long lastLDR = 0;
            if (millis() - lastLDR > 2000) {
                int ldr = analogRead(LDR_PIN);
                setBrightness(map(ldr, 0, 4095, 50, 255));
                lastLDR = millis();
            }
        }
        batteryVoltage = readBattery();

        // Periodic refresh every 2 seconds for battery level, scan status, etc.
        // This is the ONLY timer-driven redraw; everything else is event-driven.
        static unsigned long lastPeriodicRefresh = 0;
        if (millis() - lastPeriodicRefresh >= 2000) {
            displayDirty = true;
            lastPeriodicRefresh = millis();
        }

        // Process DNS for captive portal
        if (webPortalActive) {
            dnsServer.processNextRequest();
        }

        updateDisplay();

        if (millis() - lastInteractionTime > (unsigned long)(config.sleepTimeout * 1000)) {
            enterDeepSleep();
        }

        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

// ============================================================
// TOUCH CONSTANTS
// ============================================================

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

// Settings screen: toggle rows are drawn at these Y origins (from drawSettingsScreen)
// and the hitbox extends from ROW_Y - SETTINGS_ROW_PAD to ROW_Y + SETTINGS_ROW_HEIGHT - SETTINGS_ROW_PAD.
static const int SETTINGS_ROW_Y[]     = {42, 66, 90, 114, 138, 162};
static const int SETTINGS_ROW_HEIGHT  = 24;
static const int SETTINGS_HIT_X_MIN   = 20;
static const int SETTINGS_HIT_X_MAX   = 260;

// ============================================================
// TOUCH DRIVER (XPT2046 via Paul Stoffregen library on VSPI)
//
// Fr4nkFletcher CYD_28 calibration (Display.cpp calData):
//   { 350, 3465, 188, 3431, 2 }
//   raw_x: 350–3465  raw_y: 188–3431  flags: y-inverted
//
// XPT2046_Touchscreen at rotation=1 (empirically verified with 4-corner cal):
//   p.x  →  screen X (horizontal): left≈244, right≈3742
//   p.y  →  screen Y (vertical):   top≈333,  bottom≈3348
//
// config.calX/YMin/Max loaded from NVS (defaults = device-measured values).
// ============================================================

bool readTouch(uint16_t *x, uint16_t *y) {
    if (!touchAvailable) return false;

    if (touchscreen.tirqTouched() && touchscreen.touched()) {
        TS_Point p = touchscreen.getPoint();

        // Empirically verified (4-corner calibration): p.x→screen X, p.y→screen Y.
        // Use signed long intermediates: map() can return negative values when the raw
        // reading is outside the calibrated range; assigning a negative long to uint16_t
        // wraps it to ~65000, causing constrain() to clamp to the wrong edge.
        long mx = map(p.x, config.calXMin, config.calXMax, 0, SCREEN_WIDTH);
        long my = map(p.y, config.calYMin, config.calYMax, 0, SCREEN_HEIGHT);
        *x = (uint16_t)constrain(mx, 0L, (long)(SCREEN_WIDTH  - 1));
        *y = (uint16_t)constrain(my, 0L, (long)(SCREEN_HEIGHT - 1));

        Serial.printf("TOUCH: raw(%d,%d,%d) mapped(%u,%u)\n", p.x, p.y, p.z, *x, *y);
        lastInteractionTime = millis();
        return true;
    }
    return false;
}

// ============================================================
// SETUP & LOOP
// ============================================================

void setup() {
    Serial.begin(115200);
    delay(500);  // Give serial monitor time to connect
    Serial.println("\n\n============================");
    Serial.println("UK-OUI-SPY PRO v" VERSION);
    Serial.println("============================");
    xDetectionMutex = xSemaphoreCreateMutex();

    // Pin setup
    Serial.println("[BOOT] Configuring GPIO...");
    pinMode(LED_PIN, OUTPUT);
    pinMode(LED_G_PIN, OUTPUT);
    pinMode(LED_B_PIN, OUTPUT);
    pinMode(TFT_BL, OUTPUT);
    pinMode(BAT_ADC, INPUT);
    pinMode(BOOT_BTN, INPUT_PULLUP);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);

    // Flash LED to confirm new firmware is running
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);

    Serial.println("[BOOT] initDisplay...");
    initDisplay();
    Serial.println("[BOOT] initDisplay OK");

    // ── Boot splash ──────────────────────────────────────────────────────────
    tft.fillScreen(0x0000);

    // Header bar — project name + version
    tft.fillRect(0, 0, 320, 28, COL_HEADER);
    tft.setTextSize(2);
    tft.setTextColor(0x07FF);       // cyan
    tft.setCursor(8, 6);
    tft.print("UK-OUI-SPY PRO");
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(248, 10);
    tft.print("v" VERSION);

    // Tagline
    tft.setTextColor(0x7BEF);
    tft.setCursor(8, 34);
    tft.print("Midas Electrotech  |  Surveillance Detector");

    // Divider
    tft.drawFastHLine(0, 46, 320, 0x2104);

    // Stacked status lines with coloured pill badges right-aligned
    int bootY = 52;

    // bootTag: label on left, filled rounded-rect badge on right
    // tagBg = badge background colour; textCol = badge text colour (default black)
    auto bootTag = [&](const char* label, const char* tag,
                       uint16_t tagBg, uint16_t textCol = TFT_BLACK) {
        tft.setTextSize(1);
        tft.setTextColor(0xC618);       // light grey label
        tft.setCursor(8, bootY);
        tft.print(label);
        int tagW = (int)strlen(tag) * 6 + 8;   // 6px/char + 4px each side padding
        int tagX = 310 - tagW;
        tft.fillRoundRect(tagX, bootY - 2, tagW, 12, 3, tagBg);
        tft.setTextColor(textCol);
        tft.setCursor(tagX + 4, bootY);
        tft.print(tag);
        bootY += 16;
    };

    // Generate session ID from hardware RNG
    uint32_t sid = esp_random();
    snprintf(sessionId, sizeof(sessionId), "%04X-%04X",
             (unsigned)(sid >> 16) & 0xFFFFu, (unsigned)sid & 0xFFFFu);

    Serial.println("[BOOT] initTouch...");
    initTouch();
    Serial.println("[BOOT] initTouch OK");

    Serial.println("[BOOT] loadConfig...");
    loadConfig();
    Serial.println("[BOOT] loadConfig OK");
    Serial.printf("[BOOT] Touch cal: X=%d..%d  Y=%d..%d\n",
                  config.calXMin, config.calXMax, config.calYMin, config.calYMax);

    // Skip wizard — boot straight to main screen
    config.setupComplete = true;
    currentScreen = SCREEN_MAIN;
    Serial.println("[BOOT] Wizard skipped -> SCREEN_MAIN");

    bootTag("Surveillance detector", "ARMED",    0x07E0);          // green

    Serial.println("[BOOT] initSDCard...");
    initSDCard();
    if (sdCardAvailable) {
        SD.mkdir("/sessions");
        snprintf(sessionLogPath, sizeof(sessionLogPath), "/sessions/%s.csv", sessionId);
        Serial.printf("[BOOT] Log path: %s\n", sessionLogPath);
        // Read WiFi credentials from /wifi.txt (SSID line 1, password line 2)
        if (SD.exists("/wifi.txt")) {
            File wf = SD.open("/wifi.txt", FILE_READ);
            if (wf) {
                String ssid = wf.readStringUntil('\n');
                String pass = wf.readStringUntil('\n');
                wf.close();
                ssid.trim(); pass.trim();
                if (ssid.length() > 0) {
                    strncpy(wifiSsid, ssid.c_str(), 63); wifiSsid[63] = 0;
                    strncpy(wifiPass, pass.c_str(), 63); wifiPass[63] = 0;
                    staCredentialsFound = true;
                    Serial.printf("[BOOT] WiFi creds: SSID=%s\n", wifiSsid);
                }
            }
        }
    }
    Serial.println("[BOOT] initSDCard OK");
    bootTag("Initialising SD card",
            sdCardAvailable ? "ONLINE" : "OFFLINE",
            sdCardAvailable ? 0x07E0   : 0xFD20);                 // green / orange

    Serial.printf("[BOOT] OUI database: %d entries\n", OUI_DATABASE_SIZE);

    // Load priority database (SD first, then static fallback)
    Serial.println("[BOOT] Loading priority DB...");
    if (!loadPriorityDB("/priority.json")) initializeStaticPriorityDB();
    Serial.println("[BOOT] Priority DB OK");

    Serial.println("[BOOT] initBLE...");
    initBLE();
    Serial.println("[BOOT] initBLE OK");
    bootTag("BLE scanner",
            config.enableBLE ? "ACTIVE" : "DISABLED",
            config.enableBLE ? 0x07E0   : 0x4A49);                // green / grey

    Serial.println("[BOOT] initWiFi...");
    initWiFi();
    Serial.println("[BOOT] initWiFi OK");
    bootTag("WiFi scanner",
            config.enableWiFi ? "ACTIVE" : "DISABLED",
            config.enableWiFi ? 0x07E0   : 0x4A49);               // green / grey

    // Session ID — blue badge, white text
    bootTag("Session ID", sessionId, 0x001F, TFT_WHITE);
    Serial.printf("[BOOT] Session: %s\n", sessionId);

    // NTP time sync — via STA if wifi.txt credentials found, otherwise AP MODE
    bool ntpOk = false;
    if (staCredentialsFound) {
        // Wait up to 8s for STA association
        bootTag("Connecting WiFi", wifiSsid, 0x7BEF);
        unsigned long staStart = millis();
        bool staOk = false;
        while (millis() - staStart < 8000) {
            if (WiFi.status() == WL_CONNECTED) { staOk = true; break; }
            delay(200);
        }
        if (staOk) {
            Serial.printf("[WiFi] STA connected, IP: %s\n", WiFi.localIP().toString().c_str());
            configTime(0, 0, "pool.ntp.org", "time.nist.gov");
            unsigned long ntpStart = millis();
            while (millis() - ntpStart < 5000) {
                if (time(nullptr) > 1000000000UL) { ntpOk = true; break; }
                delay(100);
            }
            bootTag("Syncing time", ntpOk ? "SYNCED" : "TIMEOUT",
                                    ntpOk ? 0x07E0   : 0xFD20);
            Serial.printf("[BOOT] NTP sync %s\n", ntpOk ? "OK" : "timed out");
        } else {
            Serial.println("[WiFi] STA connection timed out");
            bootTag("Syncing time", "TIMEOUT", 0xFD20);
        }
    } else {
        bootTag("Syncing time", "AP MODE", 0x4A49);               // grey — no credentials, expected
        Serial.println("[BOOT] NTP sync -- no wifi.txt, AP-only mode");
    }

    delay(1000);  // let the user read the boot screen before main takes over
    // ── End boot splash ───────────────────────────────────────────────────────

    lastInteractionTime = millis();
    Serial.println("[BOOT] Starting FreeRTOS tasks...");
    xTaskCreatePinnedToCore(ScanTask, "ScanTask", 8192, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(UITask, "UITask", 12288, NULL, 1, NULL, 1);
    Serial.println("[BOOT] *** SETUP COMPLETE ***");
}

void loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }

// ============================================================
// PERIPHERAL INIT
// ============================================================

void initDisplay() { tft.init(); tft.setRotation(1); tft.fillScreen(COL_BG); }

void initTouch() {
    touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    touchscreen.begin(touchscreenSPI);
    touchscreen.setRotation(1);
    touchAvailable = true;
    Serial.println("XPT2046 touch initialized (HSPI: MOSI=32 MISO=39 CLK=25 CS=33 IRQ=36)");
}

void initSDCard() {
    sdSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
    sdCardAvailable = SD.begin(SD_CS, sdSPI);
}
void initBLE() { NimBLEDevice::init("UK-OUI-SPY"); }
void initWiFi() {
    if (config.enableWebPortal) {
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(AP_SSID, config.apPassword, AP_CHANNEL, 0, AP_MAX_CONN);
        if (staCredentialsFound) {
            WiFi.begin(wifiSsid, wifiPass);  // STA connection for NTP/updates
            Serial.printf("[WiFi] STA connecting to: %s\n", wifiSsid);
        } else {
            WiFi.disconnect();
        }
        dnsServer.start(53, "*", WiFi.softAPIP());
        setupWebServer();
        webPortalActive = true;
        Serial.printf("[WEB] AP: %s  IP: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
    } else {
        WiFi.mode(WIFI_STA);
        if (staCredentialsFound) {
            WiFi.begin(wifiSsid, wifiPass);
        } else {
            WiFi.disconnect();
        }
    }
}

void scanBLE() {
    NimBLEScan* pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true);
    pBLEScan->start(2, false);
    int found = pBLEScan->getResults().getCount();
    lastBLECount = found;
    totalScanned += found;
    Serial.printf("[SCAN] BLE: %d devices found\n", found);
    pBLEScan->clearResults();
}

void scanWiFi() {
    int n = WiFi.scanNetworks(false, true, false, 100);
    lastWiFiCount = max(n, 0);
    totalScanned += max(n, 0);
    Serial.printf("[SCAN] WiFi: %d networks found\n", n);
    for (int i = 0; i < n; ++i) checkOUI(WiFi.BSSIDstr(i), WiFi.RSSI(i), false, WiFi.SSID(i));
}

// ============================================================
// OUI CHECK WITH PRIORITY ENRICHMENT
// ============================================================

void checkOUI(String macAddress, int8_t rssi, bool isBLE, String name, BLEMeta* bleMeta, uint8_t channel) {
    String mac = macAddress;
    mac.toUpperCase();
    String oui = mac.substring(0, 8);

    // Build detection
    Detection det;
    det.macAddress = mac;
    det.ssid = name;
    det.rssi = rssi;
    det.timestamp = millis();
    det.firstSeen = millis();
    det.sightings = 1;
    det.isBLE = isBLE;
    det.context = "";
    det.correlationGroup = "";
    det.channel = channel;
    if (isBLE && bleMeta) {
        det.bleCompany    = bleMeta->company;
        det.bleSvcHint    = bleMeta->svcHint;
        det.txPower       = bleMeta->txPower;
        det.hasTxPower    = bleMeta->hasTxPower;
        det.blePublicAddr = bleMeta->publicAddr;
    }

    // Check OUI database
    const OUIEntry* entry = findOUI(oui);
    if (entry) {
        totalMatched++;
        Serial.printf("[OUI] MATCH: %s -> %s\n", oui.c_str(), entry->manufacturer);
        det.manufacturer = entry->manufacturer;
        det.category = entry->category;
        det.relevance = entry->relevance;
        det.deployment = entry->deployment;
        det.priority = (det.relevance == REL_HIGH) ? 4 : (det.relevance == REL_MEDIUM) ? 3 : 2;
    } else {
        int firstByte = strtol(oui.substring(0, 2).c_str(), nullptr, 16);
        if (firstByte & 0x02) {
            // Locally-administered bit = randomised/private MAC
            // Use BT company ID as manufacturer if available (works even with random MAC)
            if (isBLE && bleMeta && !bleMeta->company.isEmpty()) {
                det.manufacturer = bleMeta->company;
            } else {
                det.manufacturer = "Randomised MAC";
            }
        } else {
            // Real OUI: try SD IEEE database, then fall back to raw OUI prefix
            String sdName = sdLookupOUI(oui);
            if (!sdName.isEmpty()) {
                det.manufacturer = sdName;
            } else if (isBLE && bleMeta && !bleMeta->company.isEmpty()) {
                det.manufacturer = bleMeta->company;  // BT company as extra fallback
            } else {
                det.manufacturer = oui;
            }
        }
        det.category = CAT_UNKNOWN;
        det.relevance = REL_LOW;
        det.priority = PRIORITY_BASELINE;
    }

    // Enrich with priority database
    auto priIt = priorityLookup.find(oui);
    if (priIt != priorityLookup.end()) {
        if (!det.manufacturer.length()) totalMatched++;  // count stage-2 only hits
        det.manufacturer = priIt->second->label;
        det.context = priIt->second->context;
        det.correlationGroup = priIt->second->correlationGroup;
        det.priority = priIt->second->priority;
        det.confidence = priIt->second->confidence;
    }

    // BLE company-based surveillance boost — catches randomised-MAC devices
    // Manufacturer-specific data company ID is NOT randomised, so DJI/Axon/FLIR
    // can be identified by BT company ID even when MAC is random.
    if (isBLE && det.category == CAT_UNKNOWN && !det.bleCompany.isEmpty()) {
        struct BLEBoost { const char* keyword; DeviceCategory cat; int priority; };
        static const BLEBoost boosts[] = {
            // ── Drones ────────────────────────────────────────────────────
            {"DJI",                  CAT_DRONE,            PRIORITY_HIGH},
            {"Skydio",               CAT_DRONE,            PRIORITY_CRITICAL},
            {"Parrot",               CAT_DRONE,            PRIORITY_MODERATE},
            {"Autel",                CAT_DRONE,            PRIORITY_HIGH},
            // ── Body-worn cameras ─────────────────────────────────────────
            {"Axon",                 CAT_BODYCAM,          PRIORITY_HIGH},
            {"Taser",                CAT_BODYCAM,          PRIORITY_HIGH},
            {"Reveal Media",         CAT_BODYCAM,          PRIORITY_MODERATE},
            {"WCCTV",                CAT_BODYCAM,          PRIORITY_MODERATE},
            {"Motorola Solutions",   CAT_BODYCAM,          PRIORITY_MODERATE},
            // ── Fixed / PTZ surveillance cameras ─────────────────────────
            {"FLIR",                 CAT_CCTV,             PRIORITY_HIGH},
            {"Hikvision",            CAT_CCTV,             PRIORITY_HIGH},
            {"Dahua",                CAT_CCTV,             PRIORITY_HIGH},
            {"Axis Comm",            CAT_CCTV,             PRIORITY_HIGH},
            {"Hanwha",               CAT_CCTV,             PRIORITY_HIGH},
            {"Bosch Security",       CAT_CCTV,             PRIORITY_HIGH},
            {"Pelco",                CAT_CCTV,             PRIORITY_HIGH},
            {"Uniview",              CAT_CCTV,             PRIORITY_HIGH},
            {"Milestone",            CAT_CCTV,             PRIORITY_MODERATE},
            {"IndigoVision",         CAT_CCTV,             PRIORITY_MODERATE},
            // ── Facial recognition / AI analytics ────────────────────────
            {"Avigilon",             CAT_FACIAL_RECOG,     PRIORITY_CRITICAL},
            {"Genetec",              CAT_FACIAL_RECOG,     PRIORITY_HIGH},
            {"NEC",                  CAT_FACIAL_RECOG,     PRIORITY_HIGH},
            {"Briefcam",             CAT_FACIAL_RECOG,     PRIORITY_CRITICAL},
            // ── ANPR / traffic ────────────────────────────────────────────
            {"Siemens",              CAT_ANPR,             PRIORITY_HIGH},
            {"Jenoptik",             CAT_ANPR,             PRIORITY_HIGH},
            {"Kapsch",               CAT_TRAFFIC,          PRIORITY_MODERATE},
        };
        for (const auto& b : boosts) {
            if (det.bleCompany.indexOf(b.keyword) >= 0) {
                det.category = b.cat;
                det.priority  = b.priority;
                Serial.printf("[BLE-BOOST] %s -> %s (company: %s)\n",
                              mac.c_str(), b.keyword, det.bleCompany.c_str());
                break;
            }
        }
    }

    det.threatScore = computeThreatScore(det);
    addDetection(det);

    // Alert based on priority
    if (det.priority >= PRIORITY_MODERATE) {
        alertLED(det.priority);
    }

    // Log to SD card
    if (config.enableLogging && sdCardAvailable) {
        // Write CSV header on first entry
        bool needHeader = !SD.exists(sessionLogPath);
        File f = SD.open(sessionLogPath, FILE_APPEND);
        if (f) {
            if (needHeader) {
                f.println("timestamp_ms,mac,protocol,manufacturer,company,ssid,category,priority,rssi,sightings");
            }
            String proto  = isBLE ? "BLE" : "WiFi";
            String catStr = getCategoryName(det.category);
            // Quote fields that may contain commas
            auto q = [](const String& s) -> String {
                if (s.indexOf(',') >= 0) return "\"" + s + "\"";
                return s;
            };
            String logData =
                String(millis()) + "," +
                mac              + "," +
                proto            + "," +
                q(det.manufacturer) + "," +
                q(det.bleCompany)   + "," +
                q(det.ssid)         + "," +
                q(catStr)           + "," +
                String(det.priority) + "," +
                String(rssi)         + "," +
                String(det.sightings);
            f.println(logData); f.flush(); f.close();
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
            if (d.ssid.isEmpty() && !det.ssid.isEmpty()) d.ssid = det.ssid;
            // Recompute threat score with updated sightings and RSSI
            d.threatScore = computeThreatScore(d);
            found = true;
            break;
        }
    }
    if (!found) {
        detections.insert(detections.begin(), det);
        if ((int)detections.size() > MAX_DETECTIONS) { detections.pop_back(); totalEvicted++; }
    }
    // Sort: highest threat score first; ties broken by priority, then most recent, then RSSI
    std::sort(detections.begin(), detections.end(), [](const Detection& a, const Detection& b) {
        if (a.threatScore != b.threatScore) return a.threatScore > b.threatScore;
        if (a.priority    != b.priority)    return a.priority    > b.priority;
        if (a.timestamp   != b.timestamp)   return a.timestamp   > b.timestamp;
        return a.rssi > b.rssi;
    });
    xSemaphoreGive(xDetectionMutex);
    displayDirty = true;  // New/updated detection → refresh display
}

// ============================================================
// WEB PORTAL
// ============================================================

void setupWebServer() {
    // Serve dashboard
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *req){
        req->send_P(200, "text/html", WEB_DASHBOARD);
    });
    // Captive portal redirects
    webServer.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *req){ req->redirect("/"); });
    webServer.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *req){ req->redirect("/"); });
    webServer.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *req){ req->redirect("/"); });

    // API: Get detections
    webServer.on("/api/detections", HTTP_GET, [](AsyncWebServerRequest *req){
        DynamicJsonDocument doc(20480);  // 50 detections * ~400 bytes each
        xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
        auto snap = detections;
        xSemaphoreGive(xDetectionMutex);
        doc["total"]    = snap.size();
        doc["capacity"] = MAX_DETECTIONS;
        doc["evicted"]  = totalEvicted;
        int highCount = 0;
        JsonArray arr = doc.createNestedArray("detections");
        for (auto &d : snap) {
            if (d.priority >= PRIORITY_HIGH) highCount++;
            JsonObject obj = arr.createNestedObject();
            obj["mac"]           = d.macAddress;
            obj["manufacturer"]  = d.manufacturer;
            obj["ssid"]          = d.ssid;
            obj["bleCompany"]    = d.bleCompany;
            obj["bleSvcHint"]    = d.bleSvcHint;
            obj["correlationGroup"] = d.correlationGroup;
            obj["context"]       = d.context;
            obj["category"]      = getCategoryName(d.category);
            obj["relevance"]     = getRelevanceName(d.relevance);
            obj["priority"]      = d.priority;
            obj["threat"]        = d.threatScore;
            obj["rssi"]          = d.rssi;
            obj["txPower"]       = d.hasTxPower ? d.txPower : 0;
            obj["hasTxPower"]    = d.hasTxPower;
            obj["isBLE"]         = d.isBLE;
            obj["publicAddr"]    = d.blePublicAddr;
            obj["sightings"]     = d.sightings;
            obj["stationary"]    = (d.sightings > 3);
            obj["firstSeen"]     = d.firstSeen;
            obj["lastSeen"]      = d.timestamp;
            obj["ageSec"]        = (millis() - d.timestamp) / 1000;
            obj["dwellSec"]      = (millis() - d.firstSeen) / 1000;
            obj["deployment"]    = getDeploymentName(d.deployment);
            obj["confidence"]    = d.confidence;
            obj["channel"]       = d.channel;
        }
        doc["highCount"] = highCount;
        String response;
        serializeJson(doc, response);
        req->send(200, "application/json", response);
    });

    // API: Get system status
    webServer.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *req){
        DynamicJsonDocument doc(1024);
        doc["firmware"] = VERSION;
        int bp = constrain(map((int)(batteryVoltage * 100), 330, 420, 0, 100), 0, 100);
        doc["battery"] = bp;
        doc["freeHeap"] = ESP.getFreeHeap() / 1024;
        doc["ouiCount"] = OUI_DATABASE_SIZE;
        doc["priorityCount"] = priorityDB.size();
        doc["sdCard"] = sdCardAvailable;
        doc["touch"] = touchAvailable;
        doc["scanning"] = scanning;
        doc["totalScanned"] = totalScanned;
        doc["totalMatched"] = totalMatched;
        doc["packets"] = totalScanned;
        unsigned long up = millis() / 1000;
        char upStr[16];
        snprintf(upStr, sizeof(upStr), "%02d:%02d:%02d",
                 (int)(up / 3600), (int)((up % 3600) / 60), (int)(up % 60));
        doc["uptime"] = upStr;
        doc["webClients"] = WiFi.softAPgetStationNum();
        doc["alerts"] = activeAlerts.size();
        String response;
        serializeJson(doc, response);
        req->send(200, "application/json", response);
    });

    // API: Get config
    webServer.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *req){
        DynamicJsonDocument doc(512);
        doc["ble"] = config.enableBLE;
        doc["wifi"] = config.enableWiFi;
        doc["promiscuous"] = config.enableWiFi;
        doc["logging"] = config.enableLogging;
        doc["autoBrightness"] = config.autoBrightness;
        doc["showBaseline"] = config.showBaseline;
        doc["webPortal"] = config.enableWebPortal;
        doc["brightness"] = config.brightness;
        doc["apPassword"] = config.apPassword;
        String response;
        serializeJson(doc, response);
        req->send(200, "application/json", response);
    });

    // API: Update config
    webServer.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *req){},
        NULL,
        [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total){
            DynamicJsonDocument doc(512);
            if (deserializeJson(doc, data, len)) {
                req->send(400, "application/json", "{\"error\":\"bad JSON\"}");
                return;
            }
            if (doc.containsKey("ble")) config.enableBLE = doc["ble"];
            if (doc.containsKey("wifi")) config.enableWiFi = doc["wifi"];
            if (doc.containsKey("logging")) config.enableLogging = doc["logging"];
            if (doc.containsKey("autoBrightness")) config.autoBrightness = doc["autoBrightness"];
            if (doc.containsKey("showBaseline")) config.showBaseline = doc["showBaseline"];
            if (doc.containsKey("brightness")) {
                config.brightness = doc["brightness"];
                setBrightness(config.brightness);
            }
            if (doc.containsKey("apPassword")) {
                String newPass = doc["apPassword"].as<String>();
                if (newPass.length() >= 8 && newPass.length() <= 19) {
                    strncpy(config.apPassword, newPass.c_str(), 19);
                    config.apPassword[19] = 0;
                    // Restart AP with new password
                    WiFi.softAP(AP_SSID, config.apPassword, AP_CHANNEL, 0, AP_MAX_CONN);
                }
            }
            saveConfig();
            displayDirty = true;
            req->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );

    // API: Get raw logs (last 50 lines)
    webServer.on("/api/logs", HTTP_GET, [](AsyncWebServerRequest *req){
        if (!sdCardAvailable || !SD.exists(sessionLogPath)) {
            req->send(200, "text/plain", "No log data available.");
            return;
        }
        File f = SD.open(sessionLogPath);
        if (!f) { req->send(500, "text/plain", "Error reading log."); return; }
        String content = "";
        int lines = 0;
        while (f.available() && lines < 50) {
            content += f.readStringUntil('\n') + "\n";
            lines++;
        }
        f.close();
        req->send(200, "text/plain", content);
    });

    // API: Download CSV
    webServer.on("/api/logs/download", HTTP_GET, [](AsyncWebServerRequest *req){
        if (!sdCardAvailable || !SD.exists("/detections.csv")) {
            req->send(404, "text/plain", "No log file found.");
            return;
        }
        req->send(SD, "/detections.csv", "text/csv", true);
    });

    webServer.begin();
    Serial.println("[WEB] Web server started");
}

// ============================================================
// DISPLAY SYSTEM
// ============================================================

void updateDisplay() {
    if (!displayDirty) return;

    displayDirty = false;

    switch (currentScreen) {
        case SCREEN_MAIN:     drawMainScreen(); break;
        case SCREEN_RADAR:    drawRadarScreen(); break;
        case SCREEN_SETTINGS: drawSettingsScreen(); break;
        case SCREEN_INFO:     drawInfoScreen(); break;
        default:              currentScreen = SCREEN_MAIN; drawMainScreen(); break;
    }
}

void drawHeader(const char* title) {
    tft.fillRect(0, 0, 320, 28, COL_HEADER);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 6);
    tft.print(title);

    // Scanning indicator + last scan counts
    tft.setTextSize(1);
    if (scanning) {
        tft.fillCircle(210, 14, 4, COL_ACCENT);
        tft.setCursor(218, 10);
        tft.setTextColor(COL_ACCENT);
        tft.print("SCAN");
    } else if (totalScanned > 0) {
        tft.setTextColor(COL_DIMTEXT);
        tft.setCursor(200, 10);
        tft.printf("B:%d W:%d", lastBLECount, lastWiFiCount);
    }

    // Web portal indicator
    if (webPortalActive) {
        tft.setTextColor(TFT_GREEN);
        tft.setCursor(255, 10);
        tft.print("WEB");
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
    drawHeader("DEVICE LIST");

    // ── Stats strip (y=28..46) ──────────────────────────────────────────────
    tft.fillRect(0, 28, 320, 18, COL_HEADER);  // same shade as title bar — clearly visible
    xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
    int bleTotal = 0, wifiTotal = 0, alertCount = 0;
    for (auto& d : detections) {
        if (d.isBLE) bleTotal++; else wifiTotal++;
        if (d.priority >= PRIORITY_HIGH) alertCount++;
    }
    int totalDevices = (int)detections.size();
    xSemaphoreGive(xDetectionMutex);

    tft.setTextSize(1);
    // BLE count (blue)
    tft.setTextColor(0x001F); tft.setCursor(5, 31);
    tft.printf("BLE:%d", bleTotal);
    // WiFi count (green)
    tft.setTextColor(0x07E0); tft.setCursor(60, 31);
    tft.printf("WiFi:%d", wifiTotal);
    // Alert count (orange if any, grey if zero)
    tft.setTextColor(alertCount > 0 ? 0xFD20 : 0x4A49);
    tft.setCursor(125, 31);
    tft.printf("ALRT:%d", alertCount);
    // Total devices (white)
    tft.setTextColor(TFT_WHITE); tft.setCursor(190, 31);
    tft.printf("TOT:%d", totalDevices);
    // SD indicator (green=mounted, grey=no SD)
    tft.setTextColor(sdCardAvailable ? 0x07E0 : 0x4A49);
    tft.setCursor(260, 31);
    tft.print(sdCardAvailable ? "SD:OK" : "SD:--");

    // Correlation alert banner (if active)
    int yStart = 47;
    if (!activeAlerts.empty()) {
        drawCorrelationBanner();
        yStart = 69;
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

    // Calculate scroll limits (42px cards in 163px list area after stats strip)
    maxScroll = max(0, (int)snapshot.size() - 3);
    scrollOffset = constrain(scrollOffset, 0, maxScroll);

    // Draw tier headers and items
    int y = yStart;
    int currentTier = -1;
    int itemIndex = 0;

    for (int i = scrollOffset; i < (int)snapshot.size() && y < 208; i++) {
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
            if (y >= 208) break;
        }

        // ── Intelligence card (3-line, 42px) ──────────────────
        uint16_t cardBg = (det.priority >= PRIORITY_CRITICAL) ? COL_CARD_HI : COL_CARD;
        tft.fillRoundRect(5, y, 310, 42, 3, cardBg);

        // Priority colour bar (6px — clearly visible)
        tft.fillRect(5, y, 6, 42, getTierColor(det.priority));

        // Protocol badge (top-right)
        uint16_t badgeCol = det.isBLE ? 0x001F : 0x07E0;
        tft.fillRoundRect(265, y + 2, 42, 12, 2, badgeCol);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(269, y + 4);
        tft.print(det.isBLE ? " BLE" : "WiFi");

        // Category badge (stacked below protocol badge — colour-coded by type)
        if (det.category != CAT_UNKNOWN) {
            uint16_t catBg; const char* catLabel; bool lightBg = false;
            switch (det.category) {
                case CAT_CCTV:                catBg = 0xC000; catLabel = "CCTV";  break;
                case CAT_ANPR:                catBg = 0xC8A0; catLabel = "ANPR";  break;
                case CAT_DRONE:               catBg = 0x4810; catLabel = "DRONE"; break;
                case CAT_BODYCAM:             catBg = 0xA800; catLabel = "BCAM";  break;
                case CAT_CLOUD_CCTV:          catBg = 0x8000; catLabel = "CLOUD"; break;
                case CAT_TRAFFIC:             catBg = 0x8400; catLabel = "TRAFF"; break;
                case CAT_FACIAL_RECOG:        catBg = 0x8010; catLabel = "FACE";  break;
                case CAT_SMART_CITY_INFRA:    catBg = 0x0412; catLabel = "CITY";  break;
                case CAT_PARKING_ENFORCEMENT: catBg = 0xA840; catLabel = "PARK";  break;
                case CAT_DASH_CAM:            catBg = 0x4208; catLabel = "DASH";  break;
                case CAT_DOORBELL_CAM:        catBg = 0x03E0; catLabel = "BELL";  break;
                default:                      catBg = 0x2945; catLabel = "UNK";   break;
            }
            tft.fillRoundRect(265, y + 16, 42, 12, 2, catBg);
            tft.setTextColor(TFT_WHITE);
            tft.setCursor(269, y + 18);
            tft.print(catLabel);
        }

        // Line 1 — Primary identifier (tier-coloured so risk is immediately visible)
        // For BLE: prefer company name over raw OUI; for WiFi: SSID if manufacturer unknown
        bool rawOUI = (det.manufacturer.length() == 8 && det.manufacturer[2] == ':');
        String primaryName = (!rawOUI || det.ssid.isEmpty()) ? det.manufacturer : det.ssid;
        if (primaryName.length() > 26) primaryName = primaryName.substring(0, 23) + "...";
        tft.setTextColor(getTierColor(det.priority));
        tft.setCursor(15, y + 4);
        tft.print(primaryName);

        // Line 2 — Device type / context + RSSI (+ distance for BLE with TX power)
        tft.setTextColor(COL_DIMTEXT);
        tft.setCursor(15, y + 16);
        // Choose best descriptor: service hint > category > SSID (if not on line 1) > BLE company
        String typeStr = "";
        if (!det.bleSvcHint.isEmpty())                              typeStr = det.bleSvcHint;
        else if (det.category != CAT_UNKNOWN)                       typeStr = getCategoryName(det.category);
        else if (!det.ssid.isEmpty() && !rawOUI)                    typeStr = det.ssid;
        else if (!det.bleCompany.isEmpty() && det.bleCompany != det.manufacturer) typeStr = det.bleCompany;
        else                                                        typeStr = "Unknown";
        if (typeStr.length() > 14) typeStr = typeStr.substring(0, 12) + "..";

        if (det.isBLE && det.hasTxPower) {
            // Estimated distance: d = 10^((TxPower - RSSI) / 20)
            float dist = pow(10.0f, ((float)det.txPower - (float)det.rssi) / 20.0f);
            tft.printf("%-14s %ddBm ~%.0fm", typeStr.c_str(), det.rssi,
                       dist < 100.0f ? dist : 99.0f);
        } else {
            tft.printf("%-14s %ddBm", typeStr.c_str(), det.rssi);
        }

        // Line 3 — MAC + address type + dwell time + sightings + confidence
        unsigned long dwellSec = (millis() - det.firstSeen) / 1000;
        String dwellStr = (dwellSec < 60)   ? String(dwellSec) + "s" :
                          (dwellSec < 3600) ? String(dwellSec / 60) + "m" :
                                              String(dwellSec / 3600) + "h";
        tft.setTextColor(0x4A49);
        tft.setCursor(15, y + 29);
        tft.print(det.macAddress);
        // Address type indicator for BLE
        if (det.isBLE) {
            tft.setTextColor(det.blePublicAddr ? 0x07E0 : 0xFD20); // green=public, orange=random
            tft.setCursor(130, y + 29);
            tft.print(det.blePublicAddr ? " PUB" : " RND");
        }
        tft.setTextColor(0x4A49);
        tft.setCursor(195, y + 29);
        tft.printf("%s", dwellStr.c_str());
        if (det.sightings > 1) { tft.setCursor(227, y + 29); tft.printf("x%d", det.sightings); }
        // Confidence % for priority DB hits with meaningful confidence
        if (det.confidence >= 0.7f) {
            tft.setTextColor(getTierColor(det.priority));
            tft.setCursor(265, y + 29);
            tft.printf("%d%%", (int)(det.confidence * 100));
        }

        y += 45;
        itemIndex++;
    }

    // Scroll indicator
    if (maxScroll > 0) {
        int listH = 208 - yStart;
        int barHeight = max(10, listH / max(1, (int)snapshot.size()));
        int barY = map(scrollOffset, 0, maxScroll, yStart, 208 - barHeight);
        tft.fillRoundRect(316, barY, 3, barHeight, 1, COL_ACCENT);
    }

    // Empty state
    if (snapshot.empty()) {
        tft.setTextSize(1);
        tft.setTextColor(COL_DIMTEXT);
        if (totalScanned > 0) {
            tft.setCursor(40, 95);
            tft.print("No surveillance devices detected.");
            tft.setCursor(45, 115);
            tft.printf("%d devices scanned, %d matched", totalScanned, totalMatched);
            tft.setTextColor(0x4208);
            tft.setCursor(50, 140);
            tft.print("This is normal in most areas.");
        } else {
            tft.setCursor(90, 110);
            tft.print("Starting scan...");
        }
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
        float dist = map(constrain(det.rssi, -100, -30), -30, -100, 5, r);

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

    // Empty state for radar
    bool hasVisibleDevices = false;
    for (auto& det : snapshot) {
        if (config.showBaseline || det.priority > PRIORITY_BASELINE) {
            hasVisibleDevices = true;
            break;
        }
    }
    if (!hasVisibleDevices) {
        tft.setTextSize(1);
        tft.setTextColor(COL_DIMTEXT);
        tft.setCursor(cx - 45, cy + 45);
        tft.print("No devices in range");
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
    drawToggle(20, 42, config.enableBLE, "BLE Scanning");
    drawToggle(20, 66, config.enableWiFi, "WiFi Scanning");
    drawToggle(20, 90,  config.enableLogging,  "SD Logging");
    drawToggle(20, 114, config.autoBrightness, "Auto-Brightness");
    drawToggle(20, 138, config.showBaseline,   "Show Baseline");
    drawToggle(20, 162, config.enableWebPortal,"Web Portal");
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
    drawRow("OUI Database:", String(OUI_DATABASE_SIZE) + " entries");
    drawRow("Priority DB:", String(priorityDB.size()) + " entries");
    drawRow("Corr. Rules:", String(correlationRules.size()) + " rules");
    drawRow("Active Alerts:", String(activeAlerts.size()));
    drawRow("Detections:", String(detections.size()) + "/" + String(MAX_DETECTIONS) +
                          (totalEvicted ? " (" + String(totalEvicted) + " dropped)" : ""));
    drawRow("Free Memory:", String(ESP.getFreeHeap() / 1024) + " KB");
    drawRow("Scanned:", String(totalScanned) + " total, " + String(totalMatched) + " matched");
    drawRow("Touch:", touchAvailable ? "OK" : "ERROR");
    drawRow("SD Card:", sdCardAvailable ? "OK" : "NOT FOUND");
    if (webPortalActive) {
        drawRow("Web Portal:", String(AP_SSID) + " (" + String(WiFi.softAPgetStationNum()) + ")");
        drawRow("AP Password:", config.apPassword);
    } else {
        drawRow("Web Portal:", "Disabled");
    }

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
        tft.print("Tap NEXT or press BOOT button.");
    } else if (wizardStep == 1) {
        tft.setCursor(20, 60); tft.print("HARDWARE CHECK");
        tft.setCursor(20, 85); tft.printf("Touch: %s", touchAvailable ? "OK" : "ERROR");
        tft.setCursor(20, 100); tft.printf("SD Card: %s", sdCardAvailable ? "DETECTED" : "NOT FOUND");
        tft.setCursor(20, 115); tft.printf("OUI DB: %d entries", OUI_DATABASE_SIZE);
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
        tft.print("Tap FINISH or press BOOT button.");
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
    if (!readTouch(&x, &y)) return;
    if (millis() - lastTouch < 150) return;  // Debounce
    lastTouch = millis();

    if (y >= 208) {
        // Navbar tap
        int newScreen = x / 80;
        if (newScreen >= 0 && newScreen <= 3) {
            currentScreen = (Screen)(newScreen + 1);
            scrollOffset = 0;
        }
        displayDirty = true;
    } else if (currentScreen == SCREEN_MAIN) {
        // Scroll: top half = scroll up, bottom half = scroll down
        if (y < 120 && scrollOffset > 0) {
            scrollOffset--;
            displayDirty = true;
        } else if (y >= 120 && y < 210 && scrollOffset < maxScroll) {
            scrollOffset++;
            displayDirty = true;
        }
    } else if (currentScreen == SCREEN_SETTINGS) {
        // Toggle taps — widen hit area to full row width for easier tapping
        if (x > SETTINGS_HIT_X_MIN && x < SETTINGS_HIT_X_MAX) {
            bool* toggles[] = {
                &config.enableBLE, &config.enableWiFi, &config.enableLogging,
                &config.autoBrightness, &config.showBaseline, &config.enableWebPortal
            };
            for (int i = 0; i < 6; i++) {
                int rowTop = SETTINGS_ROW_Y[i] - 10;
                int rowBot = SETTINGS_ROW_Y[i] + SETTINGS_ROW_HEIGHT - 5;
                if (y > rowTop && y < rowBot) {
                    *toggles[i] = !*toggles[i];
                    break;
                }
            }
            saveConfig();
            displayDirty = true;
        }
    }
}

// ============================================================
// BOOT BUTTON HANDLER
// ============================================================

void handleBootButton() {
    static unsigned long lastPress = 0;
    if (digitalRead(BOOT_BTN) != LOW) return;       // Active-low
    if (millis() - lastPress < 300) return;          // Debounce
    lastPress = millis();
    lastInteractionTime = millis();

    // Cycle through screens (wizard is always skipped now)
    Screen prev = currentScreen;
    int s = (int)currentScreen + 1;
    if (s > (int)SCREEN_INFO) s = (int)SCREEN_MAIN;
    currentScreen = (Screen)s;
    scrollOffset = 0;
    displayDirty = true;
    Serial.printf("[BOOT BTN] Screen %d -> %d\n", (int)prev, (int)currentScreen);
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


void saveConfig() {
    preferences.begin("uk-oui-spy", false);
    preferences.putBool("setup", config.setupComplete);
    preferences.putBool("ble", config.enableBLE);
    preferences.putBool("wifi", config.enableWiFi);
    preferences.putBool("log", config.enableLogging);
    preferences.putBool("auto", config.autoBrightness);
    preferences.putBool("baseline", config.showBaseline);
    preferences.putBool("webp", config.enableWebPortal);
    preferences.putInt("sleepT", config.sleepTimeout);
    preferences.putString("apPass", config.apPassword);
    // Touch calibration
    preferences.putInt("calXMin", config.calXMin);
    preferences.putInt("calXMax", config.calXMax);
    preferences.putInt("calYMin", config.calYMin);
    preferences.putInt("calYMax", config.calYMax);
    preferences.end();
}

void loadConfig() {
    preferences.begin("uk-oui-spy", true);
    config.setupComplete = preferences.getBool("setup", false);
    config.enableBLE = preferences.getBool("ble", true);
    config.enableWiFi = preferences.getBool("wifi", true);
    config.enableLogging = preferences.getBool("log", true);
    config.autoBrightness = preferences.getBool("auto", true);
    config.showBaseline = preferences.getBool("baseline", true);
    config.enableWebPortal = preferences.getBool("webp", true);
    config.sleepTimeout    = preferences.getInt("sleepT", 1800);  // 30 min default
    String ap = preferences.getString("apPass", "spypro2026");
    strncpy(config.apPassword, ap.c_str(), 19); config.apPassword[19] = 0;
    // Touch calibration — default to device-measured 4-corner values
    config.calXMin = preferences.getInt("calXMin", TOUCH_CAL_X_MIN_DEFAULT);
    config.calXMax = preferences.getInt("calXMax", TOUCH_CAL_X_MAX_DEFAULT);
    config.calYMin = preferences.getInt("calYMin", TOUCH_CAL_Y_MIN_DEFAULT);
    config.calYMax = preferences.getInt("calYMax", TOUCH_CAL_Y_MAX_DEFAULT);
    preferences.end();

    // One-time migration: stale Fr4nkFletcher NVS values → device-measured values
    if (config.calXMin == 665 && config.calXMax == 3908) {
        config.calXMin = TOUCH_CAL_X_MIN_DEFAULT;  // 244
        config.calXMax = TOUCH_CAL_X_MAX_DEFAULT;  // 3742
        config.calYMin = TOUCH_CAL_Y_MIN_DEFAULT;  // 333
        config.calYMax = TOUCH_CAL_Y_MAX_DEFAULT;  // 3348
        saveConfig();
        Serial.println("[BOOT] Cal migrated: stale NVS -> device-measured values");
    }
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
    esp_sleep_enable_ext0_wakeup((gpio_num_t)XPT2046_IRQ, 0);
    esp_deep_sleep_start();
}
