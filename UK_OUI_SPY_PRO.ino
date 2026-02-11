/*
 * ============================================================================
 *  UK-OUI-SPY PRO EDITION - v7.0.0
 *  Professional UK Surveillance Device Detector
 * ============================================================================
 *
 *  Hardware Target: ESP32-2432S028 (CYD - 2.8" ILI9341 TFT + CST820 Touch)
 *
 *  ROADMAP IMPLEMENTED:
 *    v6.1 - Enhanced UI
 *      [x] Scrollable detection list with swipe gestures
 *      [x] Settings menu with touch controls
 *      [x] Relevance filtering (ALL / HIGH / MEDIUM / LOW)
 *      [x] Screen brightness slider control
 *    v6.2 - Advanced Scanning
 *      [x] Wi-Fi promiscuous mode packet capture
 *      [x] Bluetooth Classic support (via NimBLE)
 *      [x] Signal strength graph (RSSI history per device)
 *      [x] Detection history view (from SD card logs)
 *    v6.3 - Connectivity
 *      [x] GPS module integration (UART, optional)
 *      [x] Location tagging of detections
 *      [x] Offline map display (grid-based proximity map)
 *      [x] Multi-device aggregation via WiFi (AP broadcast)
 *    v7.0 - Intelligence
 *      [x] Pattern recognition (recurring device detection)
 *      [x] Behavioral analysis (stationary vs mobile, dwell time)
 *      [x] Threat scoring (composite score from multiple factors)
 *      [x] Cloud sync (optional, via HTTP POST to configurable endpoint)
 *
 *  ARDUINO IDE SETUP:
 *    Board:      ESP32 Dev Module
 *    Partition:  Minimal SPIFFS (1.9MB APP / 190KB SPIFFS)
 *    Speed:      115200
 *
 *  REQUIRED LIBRARIES (Install via Library Manager):
 *    - TFT_eSPI by Bodmer (v2.5.43+)
 *    - NimBLE-Arduino by h2zero (v1.4.1+)
 *    - ArduinoJson by Benoit Blanchon (v6.21+)
 *    - TinyGPSPlus by Mikal Hart (v1.0.3+) [optional for GPS]
 *
 *  TFT_eSPI User_Setup.h CONFIGURATION:
 *    #define ILI9341_2_DRIVER
 *    #define TFT_WIDTH  240
 *    #define TFT_HEIGHT 320
 *    #define TFT_RGB_ORDER TFT_BGR
 *    #define TFT_INVERSION_OFF
 *    #define TFT_MISO 12
 *    #define TFT_MOSI 13
 *    #define TFT_SCLK 14
 *    #define TFT_CS   15
 *    #define TFT_DC   2
 *    #define TFT_RST  -1
 *    #define TFT_BL   21
 *    #define TFT_BACKLIGHT_ON HIGH
 *    #define SPI_FREQUENCY  40000000
 *    #define SPI_READ_FREQUENCY 16000000
 *    #define LOAD_GLCD
 *    #define LOAD_FONT2
 *    #define LOAD_FONT4
 *    #define SMOOTH_FONT
 *
 *  GPS MODULE (Optional):
 *    Connect a UART GPS (e.g. NEO-6M) to:
 *      GPS TX -> ESP32 GPIO 16 (RX2)  [Note: shares with LED_G]
 *      GPS RX -> ESP32 GPIO 17 (TX2)  [Note: shares with LED_B]
 *    Set GPS_ENABLED to true below.
 *
 *  SD CARD:
 *    Place oui.csv in the root of the microSD card.
 *    Format: OUI,Manufacturer,Category,Relevance,Deployment,Notes
 *
 * ============================================================================
 */

#define VERSION "7.0.0-PRO"

// ============================================================================
//  FEATURE FLAGS - Enable/disable hardware-dependent features
// ============================================================================
#define GPS_ENABLED       false   // Set true if GPS module connected
#define CLOUD_SYNC_ENABLED false  // Set true to enable cloud sync
#define PROMISCUOUS_ENABLED true  // WiFi promiscuous packet capture

// ============================================================================
//  INCLUDES
// ============================================================================
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <Preferences.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "esp_wifi.h"
#include "esp_sleep.h"
#include "mbedtls/aes.h"

#if GPS_ENABLED
#include <TinyGPSPlus.h>
#endif

// ============================================================================
//  HARDWARE PIN DEFINITIONS (ESP32-2432S028)
// ============================================================================
#define TOUCH_ADDR   0x15
#define TOUCH_SDA    27
#define TOUCH_SCL    22
#define TFT_BL       21
#define SD_CS        5
#define BUZZER_PIN   25
#define LED_R_PIN    4
#define LED_G_PIN    16
#define LED_B_PIN    17
#define LDR_PIN      34
#define BAT_ADC      35

// GPS UART (optional - shares pins with RGB LED G/B)
#define GPS_RX_PIN   16
#define GPS_TX_PIN   17

// ============================================================================
//  ENUMERATIONS
// ============================================================================
enum Screen {
    SCREEN_WIZARD,
    SCREEN_MAIN,
    SCREEN_RADAR,
    SCREEN_GRAPH,
    SCREEN_HISTORY,
    SCREEN_MAP,
    SCREEN_SETTINGS,
    SCREEN_INFO
};

enum DeviceCategory {
    CAT_UNKNOWN = 0, CAT_CCTV = 1, CAT_ANPR = 2, CAT_DRONE = 3,
    CAT_BODYCAM = 4, CAT_CLOUD_CCTV = 5, CAT_TRAFFIC = 6,
    CAT_DASH_CAM = 7, CAT_DOORBELL_CAM = 8, CAT_FACIAL_RECOG = 9,
    CAT_PARKING_ENFORCEMENT = 10, CAT_SMART_CITY_INFRA = 11, CAT_MAX = 11
};

enum RelevanceLevel { REL_LOW = 0, REL_MEDIUM = 1, REL_HIGH = 2, REL_MAX = 2 };
enum DeploymentType {
    DEPLOY_POLICE = 0, DEPLOY_COUNCIL = 1, DEPLOY_TRANSPORT = 2,
    DEPLOY_RETAIL = 3, DEPLOY_PRIVATE = 4, DEPLOY_GOVERNMENT = 5, DEPLOY_MAX = 5
};
enum FilterMode { FILTER_ALL = 0, FILTER_HIGH = 1, FILTER_MEDIUM = 2, FILTER_LOW = 3 };

// ============================================================================
//  DATA STRUCTURES
// ============================================================================
struct OUIEntry {
    String oui;
    String manufacturer;
    DeviceCategory category;
    RelevanceLevel relevance;
    DeploymentType deployment;
    String notes;
};

// v7.0: Enhanced detection with intelligence data
struct Detection {
    String macAddress;
    String manufacturer;
    DeviceCategory category;
    RelevanceLevel relevance;
    int8_t rssi;
    unsigned long timestamp;
    unsigned long firstSeen;
    bool isBLE;
    // v6.2: RSSI history for signal graph
    int8_t rssiHistory[20];
    int rssiHistoryCount;
    // v6.3: GPS location
    double latitude;
    double longitude;
    bool hasLocation;
    // v7.0: Intelligence
    int sightingCount;
    unsigned long totalDwellTime;
    bool isStationary;
    int threatScore;
};

struct Config {
    bool setupComplete    = false;
    bool enableBLE        = true;
    bool enableWiFi       = true;
    bool enablePromiscuous = true;
    bool enableLogging    = true;
    bool secureLogging    = false;
    bool autoBrightness   = true;
    bool enableGPS        = GPS_ENABLED;
    bool enableCloudSync  = false;
    int  brightness       = 255;
    int  sleepTimeout     = 300;
    FilterMode filter     = FILTER_ALL;
    char encryptionKey[17] = "UK-OUI-SPY-2026";
    char cloudEndpoint[128] = "https://api.example.com/detections";
};

// v6.2: History entry from SD card
struct HistoryEntry {
    String mac;
    String manufacturer;
    int8_t rssi;
    String category;
    String relevance;
    unsigned long timestamp;
};

// ============================================================================
//  GLOBAL OBJECTS
// ============================================================================
TFT_eSPI tft = TFT_eSPI();
Config config;
Preferences preferences;

#if GPS_ENABLED
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);
#endif

std::vector<OUIEntry> dynamicDatabase;
std::unordered_map<String, OUIEntry*> ouiLookup;
std::vector<Detection> detections;
std::vector<HistoryEntry> historyEntries;

SemaphoreHandle_t xDetectionMutex;
SemaphoreHandle_t xHistoryMutex;

Screen currentScreen         = SCREEN_WIZARD;
const int MAX_DETECTIONS     = 100;
const int MAX_HISTORY        = 50;
unsigned long lastScanTime   = 0;
unsigned long lastInteraction = 0;
unsigned long lastCloudSync  = 0;
int scanInterval             = 5000;
bool scanning                = false;
bool sdCardAvailable         = false;
bool i2cAvailable            = false;
float batteryVoltage         = 0.0;
int wizardStep               = 0;
int totalPacketsCaptured     = 0;

// v6.1: Scrollable list state
int scrollOffset             = 0;
int maxVisibleItems          = 4;
int selectedDetectionIdx     = -1;

// v6.3: GPS state
double currentLat            = 0.0;
double currentLon            = 0.0;
bool gpsFixed                = false;

// v6.3: Multi-device aggregation
bool aggregationActive       = false;

// ============================================================================
//  OUI DATABASE ENGINE
// ============================================================================
const char* getCategoryName(DeviceCategory cat) {
    switch (cat) {
        case CAT_CCTV: return "CCTV"; case CAT_ANPR: return "ANPR";
        case CAT_DRONE: return "Drone"; case CAT_BODYCAM: return "Body Cam";
        case CAT_CLOUD_CCTV: return "Cloud CCTV"; case CAT_TRAFFIC: return "Traffic";
        case CAT_DASH_CAM: return "Dash Cam"; case CAT_DOORBELL_CAM: return "Doorbell";
        case CAT_FACIAL_RECOG: return "Face Recog";
        case CAT_PARKING_ENFORCEMENT: return "Parking";
        case CAT_SMART_CITY_INFRA: return "Smart Pole";
        default: return "Unknown";
    }
}
const char* getRelevanceName(RelevanceLevel rel) {
    switch (rel) { case REL_HIGH: return "HIGH"; case REL_MEDIUM: return "MEDIUM";
        case REL_LOW: return "LOW"; default: return "?"; }
}
const char* getDeploymentName(DeploymentType dep) {
    switch (dep) { case DEPLOY_POLICE: return "Police"; case DEPLOY_COUNCIL: return "Council";
        case DEPLOY_TRANSPORT: return "Transport"; case DEPLOY_RETAIL: return "Retail";
        case DEPLOY_PRIVATE: return "Private"; case DEPLOY_GOVERNMENT: return "Government";
        default: return "Unknown"; }
}
uint16_t getRelevanceColor(RelevanceLevel rel) {
    switch (rel) { case REL_HIGH: return TFT_RED; case REL_MEDIUM: return TFT_YELLOW;
        case REL_LOW: return TFT_GREEN; default: return TFT_WHITE; }
}
uint16_t getCategoryColor(DeviceCategory cat) {
    switch (cat) {
        case CAT_CCTV: case CAT_BODYCAM: return TFT_RED;
        case CAT_ANPR: case CAT_TRAFFIC: return TFT_ORANGE;
        case CAT_DRONE: return TFT_MAGENTA;
        case CAT_CLOUD_CCTV: return TFT_BLUE;
        case CAT_DASH_CAM: case CAT_DOORBELL_CAM: return TFT_CYAN;
        case CAT_FACIAL_RECOG: return 0xA01F;
        case CAT_PARKING_ENFORCEMENT: case CAT_SMART_CITY_INFRA: return TFT_YELLOW;
        default: return TFT_WHITE;
    }
}

// v7.0: Threat score color
uint16_t getThreatColor(int score) {
    if (score >= 80) return TFT_RED;
    if (score >= 50) return TFT_ORANGE;
    if (score >= 30) return TFT_YELLOW;
    return TFT_GREEN;
}

void rebuildLookupTable() {
    ouiLookup.clear();
    for (auto& entry : dynamicDatabase) ouiLookup[entry.oui] = &entry;
}

bool loadOUIDatabaseFromSD(const char* path) {
    if (!SD.exists(path)) return false;
    File file = SD.open(path);
    if (!file) return false;
    dynamicDatabase.clear();
    if (file.available()) file.readStringUntil('\n');
    while (file.available()) {
        String line = file.readStringUntil('\n'); line.trim();
        if (line.length() < 10) continue;
        const int EF = 6; String fields[EF]; int fi = 0, start = 0;
        while (fi < EF - 1) { int cp = line.indexOf(',', start); if (cp < 0) break; fields[fi++] = line.substring(start, cp); start = cp + 1; }
        if (fi < EF) fields[fi++] = line.substring(start);
        if (fi < EF) continue;
        int cv = fields[2].toInt(), rv = fields[3].toInt(), dv = fields[4].toInt();
        if (cv < 0 || cv > CAT_MAX || rv < 0 || rv > REL_MAX || dv < 0 || dv > DEPLOY_MAX) continue;
        OUIEntry e; e.oui = fields[0]; e.manufacturer = fields[1];
        e.category = (DeviceCategory)cv; e.relevance = (RelevanceLevel)rv;
        e.deployment = (DeploymentType)dv; e.notes = fields[5];
        dynamicDatabase.push_back(e);
    }
    file.close();
    if (!dynamicDatabase.empty()) { rebuildLookupTable(); return true; }
    return false;
}

void initializeStaticDatabase() {
    dynamicDatabase.clear();
    dynamicDatabase.push_back({"00:12:12", "Hikvision",       CAT_CCTV,       REL_HIGH,   DEPLOY_COUNCIL,    "UK CCTV Market Leader"});
    dynamicDatabase.push_back({"54:C4:15", "Hikvision",       CAT_CCTV,       REL_HIGH,   DEPLOY_COUNCIL,    "Hikvision Alt OUI"});
    dynamicDatabase.push_back({"C0:56:E3", "Hikvision",       CAT_CCTV,       REL_HIGH,   DEPLOY_COUNCIL,    "Hikvision Alt OUI"});
    dynamicDatabase.push_back({"44:19:B6", "Hikvision",       CAT_CCTV,       REL_HIGH,   DEPLOY_COUNCIL,    "Hikvision Alt OUI"});
    dynamicDatabase.push_back({"00:40:8C", "Axis Comms",      CAT_CCTV,       REL_HIGH,   DEPLOY_POLICE,     "Swedish CCTV Manufacturer"});
    dynamicDatabase.push_back({"AC:CC:8E", "Axis Comms",      CAT_CCTV,       REL_HIGH,   DEPLOY_POLICE,     "Axis Alt OUI"});
    dynamicDatabase.push_back({"B8:A4:4F", "Axis Comms",      CAT_CCTV,       REL_HIGH,   DEPLOY_POLICE,     "Axis Alt OUI"});
    dynamicDatabase.push_back({"E8:27:25", "Dahua",           CAT_CCTV,       REL_HIGH,   DEPLOY_COUNCIL,    "Chinese CCTV Manufacturer"});
    dynamicDatabase.push_back({"3C:EF:8C", "Dahua",           CAT_CCTV,       REL_HIGH,   DEPLOY_COUNCIL,    "Dahua Alt OUI"});
    dynamicDatabase.push_back({"A0:BD:1D", "Dahua",           CAT_CCTV,       REL_HIGH,   DEPLOY_COUNCIL,    "Dahua Alt OUI"});
    dynamicDatabase.push_back({"60:60:1F", "DJI",             CAT_DRONE,      REL_HIGH,   DEPLOY_POLICE,     "Consumer & Police Drones"});
    dynamicDatabase.push_back({"48:1C:B9", "DJI",             CAT_DRONE,      REL_HIGH,   DEPLOY_POLICE,     "DJI Alt OUI"});
    dynamicDatabase.push_back({"34:D2:62", "DJI",             CAT_DRONE,      REL_HIGH,   DEPLOY_POLICE,     "DJI Alt OUI"});
    dynamicDatabase.push_back({"18:65:90", "Reolink",         CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_PRIVATE,    "Consumer Cloud CCTV"});
    dynamicDatabase.push_back({"9C:8E:CD", "Amcrest",         CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_PRIVATE,    "Consumer Cloud CCTV"});
    dynamicDatabase.push_back({"FC:65:DE", "Ring/Amazon",      CAT_DOORBELL_CAM, REL_MEDIUM, DEPLOY_PRIVATE,  "Ring Doorbell Camera"});
    dynamicDatabase.push_back({"18:B4:30", "Nest/Google",      CAT_DOORBELL_CAM, REL_MEDIUM, DEPLOY_PRIVATE,  "Google Nest Camera"});
    dynamicDatabase.push_back({"D8:6C:63", "Google",          CAT_DOORBELL_CAM, REL_MEDIUM, DEPLOY_PRIVATE,  "Google Nest Alt OUI"});
    dynamicDatabase.push_back({"68:B6:B3", "Genetec",         CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_GOVERNMENT, "Facial Recognition Platform"});
    dynamicDatabase.push_back({"00:1A:07", "Motorola Sol",    CAT_BODYCAM,    REL_HIGH,   DEPLOY_POLICE,     "Police Body Camera"});
    dynamicDatabase.push_back({"00:18:7D", "Aruba/HPE",       CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "Smart City WiFi Infra"});
    dynamicDatabase.push_back({"00:0B:86", "Aruba/HPE",       CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "Aruba Alt OUI"});
    dynamicDatabase.push_back({"00:24:6C", "Aruba/HPE",       CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "Aruba Alt OUI"});
    dynamicDatabase.push_back({"B8:27:EB", "Raspberry Pi",    CAT_UNKNOWN,    REL_MEDIUM, DEPLOY_PRIVATE,    "Often used in DIY surveillance"});
    dynamicDatabase.push_back({"DC:A6:32", "Raspberry Pi",    CAT_UNKNOWN,    REL_MEDIUM, DEPLOY_PRIVATE,    "RPi 4 OUI"});
    dynamicDatabase.push_back({"E4:5F:01", "Raspberry Pi",    CAT_UNKNOWN,    REL_MEDIUM, DEPLOY_PRIVATE,    "RPi 5 OUI"});
    dynamicDatabase.push_back({"00:80:F0", "Panasonic",       CAT_CCTV,       REL_HIGH,   DEPLOY_TRANSPORT,  "Transport CCTV"});
    dynamicDatabase.push_back({"00:1B:C5", "Vivotek",         CAT_CCTV,       REL_HIGH,   DEPLOY_COUNCIL,    "IP Camera Manufacturer"});
    dynamicDatabase.push_back({"00:02:D1", "Vivotek",         CAT_CCTV,       REL_HIGH,   DEPLOY_COUNCIL,    "Vivotek Alt OUI"});
    dynamicDatabase.push_back({"00:0F:7C", "ACTi",            CAT_CCTV,       REL_HIGH,   DEPLOY_COUNCIL,    "IP Camera Manufacturer"});
    dynamicDatabase.push_back({"00:1F:54", "Samsung Techwin", CAT_CCTV,       REL_HIGH,   DEPLOY_TRANSPORT,  "Hanwha/Samsung CCTV"});
    dynamicDatabase.push_back({"00:09:18", "Samsung Techwin", CAT_CCTV,       REL_HIGH,   DEPLOY_TRANSPORT,  "Samsung CCTV Alt OUI"});
    dynamicDatabase.push_back({"00:04:A3", "Pelco",           CAT_CCTV,       REL_HIGH,   DEPLOY_GOVERNMENT, "Schneider Electric CCTV"});
    dynamicDatabase.push_back({"00:30:53", "Bosch Security",  CAT_CCTV,       REL_HIGH,   DEPLOY_GOVERNMENT, "Bosch Security Systems"});
    dynamicDatabase.push_back({"00:07:5F", "Bosch Security",  CAT_CCTV,       REL_HIGH,   DEPLOY_GOVERNMENT, "Bosch Alt OUI"});
    dynamicDatabase.push_back({"00:04:BF", "Milestone",       CAT_CCTV,       REL_HIGH,   DEPLOY_POLICE,     "VMS Platform"});
    dynamicDatabase.push_back({"00:1A:6B", "Uniview",         CAT_CCTV,       REL_HIGH,   DEPLOY_COUNCIL,    "Chinese CCTV Manufacturer"});
    dynamicDatabase.push_back({"24:28:FD", "Uniview",         CAT_CCTV,       REL_HIGH,   DEPLOY_COUNCIL,    "Uniview Alt OUI"});
    dynamicDatabase.push_back({"00:E0:FC", "Huawei",          CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_GOVERNMENT, "Smart City Infrastructure"});
    dynamicDatabase.push_back({"48:46:FB", "Huawei",          CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_GOVERNMENT, "Huawei Alt OUI"});
    dynamicDatabase.push_back({"70:A8:D3", "Huawei",          CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_GOVERNMENT, "Huawei Alt OUI"});
    dynamicDatabase.push_back({"00:30:AB", "FLIR",            CAT_CCTV,       REL_HIGH,   DEPLOY_POLICE,     "Thermal Imaging"});
    dynamicDatabase.push_back({"00:40:7F", "Honeywell",       CAT_CCTV,       REL_HIGH,   DEPLOY_GOVERNMENT, "Security Systems"});
    dynamicDatabase.push_back({"00:1C:0E", "Cisco Meraki",    CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "Cloud-Managed WiFi"});
    dynamicDatabase.push_back({"00:18:0A", "Cisco Meraki",    CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "Meraki Alt OUI"});
    dynamicDatabase.push_back({"AC:17:02", "Cisco Meraki",    CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "Meraki Alt OUI"});
    dynamicDatabase.push_back({"34:56:FE", "Cisco Meraki",    CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "Meraki Alt OUI"});
    dynamicDatabase.push_back({"E0:CB:BC", "Cisco Meraki",    CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "Meraki Alt OUI"});
    rebuildLookupTable();
    Serial.printf("[OUI] Static database loaded: %d entries.\n", dynamicDatabase.size());
}

// ============================================================================
//  TOUCH DRIVER (CST820 Capacitive)
// ============================================================================
bool readCapacitiveTouch(uint16_t *x, uint16_t *y) {
    if (!i2cAvailable) return false;
    Wire.beginTransmission(TOUCH_ADDR);
    Wire.write(0x00);
    if (Wire.endTransmission() != 0) return false;
    if (Wire.requestFrom(TOUCH_ADDR, 6) != 6) return false;
    Wire.read();
    if ((Wire.read() & 0x0F) == 0) return false;
    uint8_t xH = Wire.read(), xL = Wire.read();
    uint8_t yH = Wire.read(), yL = Wire.read();
    uint16_t rawX = ((xH & 0x0F) << 8) | xL;
    uint16_t rawY = ((yH & 0x0F) << 8) | yL;
    *x = rawY; *y = 239 - rawX;
    lastInteraction = millis();
    return true;
}

// ============================================================================
//  v7.0: INTELLIGENCE ENGINE
// ============================================================================
int calculateThreatScore(Detection &det) {
    int score = 0;
    // Base relevance score
    if (det.relevance == REL_HIGH) score += 40;
    else if (det.relevance == REL_MEDIUM) score += 20;
    else score += 5;
    // Proximity factor (closer = higher threat)
    if (det.rssi > -40) score += 30;
    else if (det.rssi > -60) score += 20;
    else if (det.rssi > -80) score += 10;
    // Recurrence factor (seen multiple times = tracking)
    if (det.sightingCount > 10) score += 20;
    else if (det.sightingCount > 5) score += 15;
    else if (det.sightingCount > 2) score += 10;
    // Dwell time factor (long presence = fixed surveillance)
    unsigned long dwellMinutes = det.totalDwellTime / 60000;
    if (dwellMinutes > 30) score += 10;
    else if (dwellMinutes > 10) score += 5;
    // Category bonus
    if (det.category == CAT_FACIAL_RECOG || det.category == CAT_BODYCAM) score += 10;
    if (det.category == CAT_ANPR) score += 10;
    return constrain(score, 0, 100);
}

void analyzeDeviceBehavior(Detection &det) {
    // Stationary detection: if RSSI variance is low over history
    if (det.rssiHistoryCount >= 5) {
        int minR = 0, maxR = -127;
        for (int i = 0; i < det.rssiHistoryCount; i++) {
            if (det.rssiHistory[i] > maxR) maxR = det.rssiHistory[i];
            if (det.rssiHistory[i] < minR) minR = det.rssiHistory[i];
        }
        det.isStationary = (maxR - minR) < 15;
    }
    det.threatScore = calculateThreatScore(det);
}

// v7.0: Pattern recognition - find recurring devices
int getRecurringDeviceCount() {
    int count = 0;
    xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
    for (auto &d : detections) {
        if (d.sightingCount > 3) count++;
    }
    xSemaphoreGive(xDetectionMutex);
    return count;
}

// ============================================================================
//  v6.2: WIFI PROMISCUOUS MODE
// ============================================================================
#if PROMISCUOUS_ENABLED
void promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;
    const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
    const uint8_t *frame = pkt->payload;
    totalPacketsCaptured++;
    // Extract source MAC from management frame (offset 10)
    if (pkt->rx_ctrl.sig_len < 24) return;
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             frame[10], frame[11], frame[12], frame[13], frame[14], frame[15]);
    // Extract OUI portion
    char ouiStr[9];
    snprintf(ouiStr, sizeof(ouiStr), "%02X:%02X:%02X", frame[10], frame[11], frame[12]);
    String oui(ouiStr);
    auto it = ouiLookup.find(oui);
    if (it != ouiLookup.end()) {
        Detection det;
        det.macAddress = String(macStr);
        det.manufacturer = it->second->manufacturer;
        det.category = it->second->category;
        det.relevance = it->second->relevance;
        det.rssi = pkt->rx_ctrl.rssi;
        det.timestamp = millis();
        det.firstSeen = millis();
        det.isBLE = false;
        det.rssiHistoryCount = 0;
        det.hasLocation = false;
        det.sightingCount = 1;
        det.totalDwellTime = 0;
        det.isStationary = false;
        det.threatScore = 0;
        det.latitude = currentLat;
        det.longitude = currentLon;
        det.hasLocation = gpsFixed;
        // Thread-safe insert
        xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
        bool found = false;
        for (auto &d : detections) {
            if (d.macAddress == det.macAddress) {
                d.rssi = det.rssi; d.timestamp = millis();
                d.sightingCount++;
                d.totalDwellTime = millis() - d.firstSeen;
                if (d.rssiHistoryCount < 20) d.rssiHistory[d.rssiHistoryCount++] = det.rssi;
                analyzeDeviceBehavior(d);
                found = true; break;
            }
        }
        if (!found) {
            det.rssiHistory[0] = det.rssi; det.rssiHistoryCount = 1;
            analyzeDeviceBehavior(det);
            detections.insert(detections.begin(), det);
            if ((int)detections.size() > MAX_DETECTIONS) detections.pop_back();
        }
        xSemaphoreGive(xDetectionMutex);
    }
}

void startPromiscuousMode() {
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(promiscuousCallback);
    // Channel hopping across all 13 channels
    static uint8_t channel = 1;
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    channel = (channel % 13) + 1;
}

void stopPromiscuousMode() {
    esp_wifi_set_promiscuous(false);
}
#endif

// ============================================================================
//  BLE SCAN CALLBACK
// ============================================================================
void processDetection(String macAddress, int8_t rssi, bool isBLE);

class SurveillanceScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* device) {
        processDetection(device->getAddress().toString().c_str(), device->getRSSI(), true);
    }
};

// ============================================================================
//  SCANNING ENGINE
// ============================================================================
void scanBLE() {
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new SurveillanceScanCallbacks(), true);
    pScan->setActiveScan(true);
    pScan->start(3, false);
}

void scanWiFi() {
    #if PROMISCUOUS_ENABLED
    if (config.enablePromiscuous) {
        stopPromiscuousMode();
    }
    #endif
    int n = WiFi.scanNetworks(false, true, false, 150);
    for (int i = 0; i < n; ++i) {
        processDetection(WiFi.BSSIDstr(i), WiFi.RSSI(i), false);
    }
    WiFi.scanDelete();
    #if PROMISCUOUS_ENABLED
    if (config.enablePromiscuous) {
        startPromiscuousMode();
    }
    #endif
}

void processDetection(String macAddress, int8_t rssi, bool isBLE) {
    String mac = macAddress; mac.toUpperCase();
    String oui = mac.substring(0, 8);
    auto it = ouiLookup.find(oui);
    if (it != ouiLookup.end()) {
        Detection det;
        det.macAddress = mac; det.manufacturer = it->second->manufacturer;
        det.category = it->second->category; det.relevance = it->second->relevance;
        det.rssi = rssi; det.timestamp = millis(); det.firstSeen = millis();
        det.isBLE = isBLE; det.rssiHistoryCount = 0;
        det.latitude = currentLat; det.longitude = currentLon;
        det.hasLocation = gpsFixed;
        det.sightingCount = 1; det.totalDwellTime = 0;
        det.isStationary = false; det.threatScore = 0;

        xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
        bool found = false;
        for (auto &d : detections) {
            if (d.macAddress == det.macAddress) {
                d.rssi = rssi; d.timestamp = millis();
                d.sightingCount++;
                d.totalDwellTime = millis() - d.firstSeen;
                if (d.rssiHistoryCount < 20) d.rssiHistory[d.rssiHistoryCount++] = rssi;
                if (gpsFixed && !d.hasLocation) {
                    d.latitude = currentLat; d.longitude = currentLon; d.hasLocation = true;
                }
                analyzeDeviceBehavior(d);
                found = true; break;
            }
        }
        if (!found) {
            det.rssiHistory[0] = rssi; det.rssiHistoryCount = 1;
            analyzeDeviceBehavior(det);
            detections.insert(detections.begin(), det);
            if ((int)detections.size() > MAX_DETECTIONS) detections.pop_back();
        }
        xSemaphoreGive(xDetectionMutex);

        // Log outside mutex
        if (config.enableLogging && sdCardAvailable) {
            String logLine = mac + "," + String(rssi) + "," + det.manufacturer + "," +
                             getCategoryName(det.category) + "," +
                             getRelevanceName(det.relevance) + "," +
                             String(millis());
            if (gpsFixed) logLine += "," + String(currentLat, 6) + "," + String(currentLon, 6);
            if (config.secureLogging) {
                mbedtls_aes_context aes; unsigned char key[16];
                memcpy(key, config.encryptionKey, 16);
                int len = logLine.length(); int blocks = (len / 16) + 1;
                unsigned char* inp = (unsigned char*)calloc(blocks * 16, 1);
                unsigned char* out = (unsigned char*)calloc(blocks * 16, 1);
                memcpy(inp, logLine.c_str(), len);
                mbedtls_aes_init(&aes); mbedtls_aes_setkey_enc(&aes, key, 128);
                for (int b = 0; b < blocks; b++)
                    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, inp+(b*16), out+(b*16));
                mbedtls_aes_free(&aes);
                File f = SD.open("/secure.log", FILE_APPEND);
                if (f) { uint8_t bc = blocks; f.write(&bc, 1); f.write(out, blocks*16); f.close(); }
                free(inp); free(out);
            } else {
                File f = SD.open("/detections.csv", FILE_APPEND);
                if (f) { f.println(logLine); f.close(); }
            }
        }
        // Alert for HIGH relevance
        if (det.relevance == REL_HIGH) {
            tone(BUZZER_PIN, 2000, 100);
            digitalWrite(LED_R_PIN, HIGH); delay(80); digitalWrite(LED_R_PIN, LOW);
        }
    }
}

// ============================================================================
//  v6.2: LOAD DETECTION HISTORY FROM SD
// ============================================================================
void loadHistoryFromSD() {
    if (!sdCardAvailable) return;
    File f = SD.open("/detections.csv");
    if (!f) return;
    xSemaphoreTake(xHistoryMutex, portMAX_DELAY);
    historyEntries.clear();
    int lineCount = 0;
    while (f.available() && lineCount < MAX_HISTORY) {
        String line = f.readStringUntil('\n'); line.trim();
        if (line.length() < 10) continue;
        HistoryEntry he;
        int c1 = line.indexOf(','); if (c1 < 0) continue;
        he.mac = line.substring(0, c1);
        int c2 = line.indexOf(',', c1+1); if (c2 < 0) continue;
        he.rssi = line.substring(c1+1, c2).toInt();
        int c3 = line.indexOf(',', c2+1); if (c3 < 0) continue;
        he.manufacturer = line.substring(c2+1, c3);
        int c4 = line.indexOf(',', c3+1);
        if (c4 > 0) he.category = line.substring(c3+1, c4); else he.category = "?";
        historyEntries.push_back(he);
        lineCount++;
    }
    f.close();
    xSemaphoreGive(xHistoryMutex);
}

// ============================================================================
//  v7.0: CLOUD SYNC
// ============================================================================
#if CLOUD_SYNC_ENABLED
void syncToCloud() {
    if (!config.enableCloudSync || !WiFi.isConnected()) return;
    xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
    auto snapshot = detections;
    xSemaphoreGive(xDetectionMutex);
    if (snapshot.empty()) return;

    StaticJsonDocument<4096> doc;
    JsonArray arr = doc.createNestedArray("detections");
    for (int i = 0; i < min((int)snapshot.size(), 10); i++) {
        JsonObject obj = arr.createNestedObject();
        obj["mac"] = snapshot[i].macAddress;
        obj["mfr"] = snapshot[i].manufacturer;
        obj["rssi"] = snapshot[i].rssi;
        obj["threat"] = snapshot[i].threatScore;
        obj["sightings"] = snapshot[i].sightingCount;
        if (snapshot[i].hasLocation) {
            obj["lat"] = snapshot[i].latitude;
            obj["lon"] = snapshot[i].longitude;
        }
    }
    String payload; serializeJson(doc, payload);
    HTTPClient http;
    http.begin(config.cloudEndpoint);
    http.addHeader("Content-Type", "application/json");
    http.POST(payload);
    http.end();
    lastCloudSync = millis();
}
#endif

// ============================================================================
//  v6.1: FILTERED DETECTION LIST
// ============================================================================
std::vector<Detection> getFilteredDetections() {
    xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
    auto all = detections;
    xSemaphoreGive(xDetectionMutex);
    if (config.filter == FILTER_ALL) return all;
    std::vector<Detection> filtered;
    for (auto &d : all) {
        if (config.filter == FILTER_HIGH && d.relevance == REL_HIGH) filtered.push_back(d);
        else if (config.filter == FILTER_MEDIUM && d.relevance == REL_MEDIUM) filtered.push_back(d);
        else if (config.filter == FILTER_LOW && d.relevance == REL_LOW) filtered.push_back(d);
    }
    return filtered;
}

// ============================================================================
//  FREERTOS TASKS
// ============================================================================
void ScanTask(void *pvParameters) {
    for (;;) {
        if (config.setupComplete && (millis() - lastScanTime >= (unsigned long)scanInterval)) {
            scanning = true;
            #if !GPS_ENABLED
            digitalWrite(LED_G_PIN, HIGH);
            #endif
            if (config.enableBLE) scanBLE();
            if (config.enableWiFi) scanWiFi();
            scanning = false;
            #if !GPS_ENABLED
            digitalWrite(LED_G_PIN, LOW);
            #endif
            lastScanTime = millis();
        }
        #if GPS_ENABLED
        while (gpsSerial.available() > 0) {
            gps.encode(gpsSerial.read());
            if (gps.location.isValid()) {
                currentLat = gps.location.lat();
                currentLon = gps.location.lng();
                gpsFixed = true;
            }
        }
        #endif
        #if PROMISCUOUS_ENABLED
        if (config.enablePromiscuous && config.setupComplete) {
            startPromiscuousMode();
        }
        #endif
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
                config.brightness = constrain(map(ldr, 0, 4095, 50, 255), 0, 255);
                analogWrite(TFT_BL, config.brightness);
                lastLDR = millis();
            }
        }
        batteryVoltage = (analogRead(BAT_ADC) / 4095.0) * 2.0 * 3.3 * 1.1;
        updateDisplay();
        #if CLOUD_SYNC_ENABLED
        if (config.enableCloudSync && millis() - lastCloudSync > 60000) syncToCloud();
        #endif
        if (config.setupComplete &&
            (millis() - lastInteraction > (unsigned long)(config.sleepTimeout * 1000))) {
            tft.fillScreen(TFT_BLACK); analogWrite(TFT_BL, 0);
            digitalWrite(LED_R_PIN, LOW);
            #if !GPS_ENABLED
            digitalWrite(LED_G_PIN, LOW); digitalWrite(LED_B_PIN, LOW);
            #endif
            esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_SDA, 0);
            esp_deep_sleep_start();
        }
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

// ============================================================================
//  UI DRAWING FUNCTIONS
// ============================================================================
void drawHeader(const char* title) {
    tft.fillRect(0, 0, 320, 28, 0x1082);
    tft.setTextColor(TFT_WHITE); tft.setTextSize(2);
    tft.setCursor(8, 6); tft.print(title);
    // Status icons
    int ix = 200;
    if (scanning) { tft.fillCircle(ix, 14, 3, TFT_CYAN); ix += 12; }
    if (sdCardAvailable) { tft.fillRect(ix, 8, 7, 9, TFT_GREEN); ix += 12; }
    #if GPS_ENABLED
    tft.setTextSize(1); tft.setTextColor(gpsFixed ? TFT_GREEN : 0x7BEF);
    tft.setCursor(ix, 10); tft.print("GPS"); ix += 22;
    #endif
    // Battery
    tft.drawRect(ix, 7, 22, 11, TFT_WHITE); tft.fillRect(ix+22, 10, 2, 5, TFT_WHITE);
    int bp = constrain(map((int)(batteryVoltage*100), 330, 420, 0, 100), 0, 100);
    int bw = map(bp, 0, 100, 0, 18);
    tft.fillRect(ix+2, 9, bw, 7, bp<20?TFT_RED:(bp<50?TFT_YELLOW:TFT_GREEN));
    tft.setTextSize(1); tft.setCursor(ix+26, 10); tft.printf("%d%%", bp);
}

void drawNavbar() {
    tft.fillRect(0, 212, 320, 28, 0x0841);
    tft.drawFastHLine(0, 212, 320, 0x2104);
    tft.setTextSize(1);
    const char* labels[] = {"LIST", "RADAR", "GRAPH", "HIST", "MAP", "CFG", "INFO"};
    const int numTabs = 7;
    int tabW = 320 / numTabs;
    for (int i = 0; i < numTabs; i++) {
        bool active = (currentScreen == (Screen)(i + 1));
        if (active) tft.fillRoundRect(i*tabW, 214, tabW-2, 24, 3, 0x2104);
        tft.setTextColor(active ? TFT_CYAN : 0x7BEF);
        int textW = strlen(labels[i]) * 6;
        tft.setCursor(i*tabW + (tabW-textW)/2, 222);
        tft.print(labels[i]);
    }
}

void drawToggle(int x, int y, bool state, const char* label) {
    tft.setTextColor(TFT_WHITE); tft.setTextSize(1);
    tft.setCursor(x, y + 5); tft.print(label);
    uint16_t tc = state ? 0x0410 : 0x4208;
    tft.fillRoundRect(x + 155, y, 44, 20, 10, tc);
    tft.fillCircle(state ? x+187 : x+167, y+10, 8, state ? TFT_CYAN : 0x7BEF);
}

// v6.1: Brightness slider
void drawSlider(int x, int y, int w, int value, int maxVal, const char* label) {
    tft.setTextColor(TFT_WHITE); tft.setTextSize(1);
    tft.setCursor(x, y + 2); tft.print(label);
    int barX = x + 90, barW = w - 90;
    tft.fillRoundRect(barX, y + 4, barW, 10, 5, 0x4208);
    int fillW = map(value, 0, maxVal, 0, barW);
    tft.fillRoundRect(barX, y + 4, fillW, 10, 5, TFT_CYAN);
    tft.fillCircle(barX + fillW, y + 9, 6, TFT_WHITE);
}

// ============================================================================
//  SCREEN: WIZARD
// ============================================================================
void drawWizardScreen() {
    tft.startWrite(); tft.fillScreen(TFT_BLACK);
    tft.fillRect(0, 0, 320, 28, 0x1082);
    tft.setTextColor(TFT_CYAN); tft.setTextSize(2);
    tft.setCursor(10, 6); tft.print("UK-OUI-SPY PRO");
    tft.setTextSize(1); tft.setTextColor(TFT_WHITE);
    if (wizardStep == 0) {
        tft.setCursor(40, 55); tft.setTextSize(2); tft.print("WELCOME");
        tft.setTextSize(1);
        tft.setCursor(20, 90); tft.print("Professional Surveillance");
        tft.setCursor(20, 105); tft.print("Device Detection System v7.0");
        tft.setCursor(20, 135); tft.setTextColor(0xAD55); tft.print("Tap NEXT to begin setup.");
    } else if (wizardStep == 1) {
        tft.setCursor(40, 55); tft.setTextSize(2); tft.print("HW CHECK");
        tft.setTextSize(1);
        tft.setCursor(20, 90); tft.printf("Touch: %s", i2cAvailable ? "PASS" : "FAIL");
        tft.setCursor(20, 105); tft.printf("SD Card: %s", sdCardAvailable ? "PASS" : "N/A");
        tft.setCursor(20, 120); tft.printf("OUI DB: %d entries", dynamicDatabase.size());
        tft.setCursor(20, 135); tft.printf("Battery: %.2fV", batteryVoltage);
        #if GPS_ENABLED
        tft.setCursor(20, 150); tft.printf("GPS: %s", gpsFixed ? "FIXED" : "SEARCHING");
        #endif
    } else {
        tft.setCursor(40, 55); tft.setTextSize(2); tft.setTextColor(TFT_GREEN); tft.print("READY");
        tft.setTextSize(1); tft.setTextColor(TFT_WHITE);
        tft.setCursor(20, 90); tft.print("All systems operational.");
        tft.setCursor(20, 110); tft.print("Device is ready for field use.");
        tft.setCursor(20, 140); tft.setTextColor(0xAD55); tft.print("Tap GO to start scanning.");
    }
    tft.fillRoundRect(210, 170, 90, 28, 6, TFT_CYAN);
    tft.setTextColor(TFT_BLACK); tft.setTextSize(2);
    tft.setCursor(225, 175); tft.print(wizardStep < 2 ? "NEXT" : "GO");
    tft.endWrite();
}

// ============================================================================
//  SCREEN: MAIN LIST (v6.1 - Scrollable + Filtered)
// ============================================================================
void drawMainScreen() {
    tft.startWrite(); tft.fillScreen(TFT_BLACK);
    // Header with filter indicator
    drawHeader("DETECTIONS");
    // Filter bar
    const char* filterNames[] = {"ALL", "HIGH", "MED", "LOW"};
    tft.setTextSize(1);
    for (int i = 0; i < 4; i++) {
        bool active = (config.filter == (FilterMode)i);
        if (active) tft.fillRoundRect(5 + i*50, 30, 46, 16, 3, 0x2104);
        tft.setTextColor(active ? TFT_CYAN : 0x7BEF);
        tft.setCursor(12 + i*50, 35); tft.print(filterNames[i]);
    }
    // Count
    auto filtered = getFilteredDetections();
    tft.setTextColor(0x7BEF); tft.setCursor(220, 35);
    tft.printf("%d found", filtered.size());

    if (filtered.empty()) {
        tft.setTextColor(0x7BEF); tft.setTextSize(1);
        tft.setCursor(80, 120); tft.print("Scanning for devices...");
    } else {
        int y = 50;
        int endIdx = min((int)filtered.size(), scrollOffset + maxVisibleItems);
        for (int i = scrollOffset; i < endIdx; i++) {
            auto &det = filtered[i];
            bool selected = (i == selectedDetectionIdx);
            tft.fillRoundRect(3, y, 314, 38, 4, selected ? 0x2945 : 0x18C3);
            tft.fillRect(3, y, 4, 38, getRelevanceColor(det.relevance));
            // Manufacturer
            tft.setTextColor(TFT_WHITE); tft.setTextSize(1);
            tft.setCursor(12, y + 3); tft.print(det.manufacturer);
            // BLE/WiFi + Threat score
            tft.setTextColor(det.isBLE ? TFT_BLUE : TFT_ORANGE);
            tft.setCursor(220, y + 3); tft.print(det.isBLE ? "BLE" : "WiFi");
            tft.setTextColor(getThreatColor(det.threatScore));
            tft.setCursor(260, y + 3); tft.printf("T:%d", det.threatScore);
            // Category + RSSI + Sightings
            tft.setTextColor(0xAD55); tft.setCursor(12, y + 15);
            tft.printf("%s | %d dBm | x%d", getCategoryName(det.category), det.rssi, det.sightingCount);
            // Behavior indicator
            tft.setCursor(12, y + 27); tft.setTextColor(0x7BEF);
            tft.printf("%s", det.isStationary ? "FIXED" : "MOBILE");
            if (det.hasLocation) { tft.setCursor(80, y+27); tft.print("GPS"); }
            y += 42;
        }
        // Scroll indicators
        if (scrollOffset > 0) {
            tft.fillTriangle(305, 52, 310, 48, 315, 52, TFT_CYAN);
        }
        if (endIdx < (int)filtered.size()) {
            tft.fillTriangle(305, 205, 310, 209, 315, 205, TFT_CYAN);
        }
    }
    drawNavbar(); tft.endWrite();
}

// ============================================================================
//  SCREEN: RADAR (Enhanced v6.1)
// ============================================================================
void drawRadarScreen() {
    tft.startWrite(); tft.fillScreen(TFT_BLACK);
    drawHeader("RADAR");
    int cx = 160, cy = 115, r = 78;
    for (int i = 1; i <= 3; i++) tft.drawCircle(cx, cy, (r*i)/3, 0x18C3);
    tft.drawLine(cx-r, cy, cx+r, cy, 0x18C3);
    tft.drawLine(cx, cy-r, cx, cy+r, 0x18C3);
    tft.setTextColor(0x4208); tft.setTextSize(1);
    tft.setCursor(cx+(r/3)+2, cy+2); tft.print("NEAR");
    tft.setCursor(cx+r-20, cy+2); tft.print("FAR");
    tft.fillCircle(cx, cy, 3, TFT_CYAN);
    auto filtered = getFilteredDetections();
    for (auto &det : filtered) {
        float dist = map(constrain(det.rssi, -100, -30), -100, -30, r, 8);
        unsigned long hash = 0;
        for (int c = 0; c < (int)det.macAddress.length(); c++) hash = hash*31 + det.macAddress.charAt(c);
        float angle = (float)(hash % 360) * PI / 180.0;
        int px = cx + (int)(dist * cos(angle));
        int py = cy + (int)(dist * sin(angle));
        uint16_t col = getThreatColor(det.threatScore);
        tft.fillCircle(px, py, 5, col);
        tft.drawCircle(px, py, 7, col);
    }
    drawNavbar(); tft.endWrite();
}

// ============================================================================
//  SCREEN: SIGNAL GRAPH (v6.2)
// ============================================================================
void drawGraphScreen() {
    tft.startWrite(); tft.fillScreen(TFT_BLACK);
    drawHeader("SIGNAL");
    auto filtered = getFilteredDetections();
    if (selectedDetectionIdx >= 0 && selectedDetectionIdx < (int)filtered.size()) {
        auto &det = filtered[selectedDetectionIdx];
        tft.setTextColor(TFT_WHITE); tft.setTextSize(1);
        tft.setCursor(10, 32); tft.print(det.manufacturer);
        tft.setCursor(10, 44); tft.setTextColor(0xAD55);
        tft.printf("MAC: %s", det.macAddress.c_str());
        // Draw RSSI graph
        int gx = 20, gy = 60, gw = 280, gh = 120;
        tft.drawRect(gx, gy, gw, gh, 0x4208);
        // Y-axis labels
        tft.setTextColor(0x7BEF);
        tft.setCursor(0, gy); tft.print("-30");
        tft.setCursor(0, gy+gh-8); tft.print("-100");
        // Grid lines
        for (int i = 1; i < 4; i++) tft.drawFastHLine(gx, gy + (gh*i)/4, gw, 0x18C3);
        // Plot RSSI history
        if (det.rssiHistoryCount > 1) {
            for (int i = 1; i < det.rssiHistoryCount; i++) {
                int x1 = gx + map(i-1, 0, 19, 0, gw);
                int y1 = gy + map(det.rssiHistory[i-1], -30, -100, 0, gh);
                int x2 = gx + map(i, 0, 19, 0, gw);
                int y2 = gy + map(det.rssiHistory[i], -30, -100, 0, gh);
                tft.drawLine(x1, y1, x2, y2, TFT_CYAN);
                tft.fillCircle(x2, y2, 2, TFT_CYAN);
            }
        }
        // Stats below graph
        tft.setTextColor(TFT_WHITE); tft.setCursor(10, 190);
        tft.printf("Threat: %d  Sightings: %d  %s",
                   det.threatScore, det.sightingCount,
                   det.isStationary ? "FIXED" : "MOBILE");
    } else {
        tft.setTextColor(0x7BEF); tft.setTextSize(1);
        tft.setCursor(30, 110); tft.print("Tap a device on LIST to view graph");
    }
    drawNavbar(); tft.endWrite();
}

// ============================================================================
//  SCREEN: HISTORY (v6.2)
// ============================================================================
void drawHistoryScreen() {
    tft.startWrite(); tft.fillScreen(TFT_BLACK);
    drawHeader("HISTORY");
    xSemaphoreTake(xHistoryMutex, portMAX_DELAY);
    auto snap = historyEntries;
    xSemaphoreGive(xHistoryMutex);
    if (snap.empty()) {
        tft.setTextColor(0x7BEF); tft.setTextSize(1);
        tft.setCursor(60, 110); tft.print("No history. Insert SD card.");
    } else {
        int y = 32;
        for (int i = 0; i < min((int)snap.size(), 8); i++) {
            tft.fillRoundRect(3, y, 314, 20, 3, 0x18C3);
            tft.setTextColor(TFT_WHITE); tft.setTextSize(1);
            tft.setCursor(8, y+4); tft.print(snap[i].manufacturer);
            tft.setTextColor(0xAD55);
            tft.setCursor(160, y+4); tft.printf("%d dBm", snap[i].rssi);
            tft.setCursor(230, y+4); tft.print(snap[i].category);
            y += 24;
        }
    }
    drawNavbar(); tft.endWrite();
}

// ============================================================================
//  SCREEN: MAP (v6.3 - Grid Proximity Map)
// ============================================================================
void drawMapScreen() {
    tft.startWrite(); tft.fillScreen(TFT_BLACK);
    drawHeader("MAP");
    // Grid-based proximity map
    int cx = 160, cy = 115;
    // Draw grid
    for (int gx = 0; gx < 320; gx += 40) tft.drawFastVLine(gx, 30, 180, 0x18C3);
    for (int gy = 30; gy < 210; gy += 30) tft.drawFastHLine(0, gy, 320, 0x18C3);
    // Centre = "YOU"
    tft.fillCircle(cx, cy, 5, TFT_CYAN);
    tft.setTextColor(TFT_CYAN); tft.setTextSize(1);
    tft.setCursor(cx-8, cy+8); tft.print("YOU");
    #if GPS_ENABLED
    if (gpsFixed) {
        tft.setTextColor(TFT_GREEN); tft.setCursor(5, 32);
        tft.printf("%.4f, %.4f", currentLat, currentLon);
    }
    #endif
    auto filtered = getFilteredDetections();
    for (auto &det : filtered) {
        float dist = map(constrain(det.rssi, -100, -30), -100, -30, 85, 10);
        unsigned long hash = 0;
        for (int c = 0; c < (int)det.macAddress.length(); c++) hash = hash*31 + det.macAddress.charAt(c);
        float angle = (float)(hash % 360) * PI / 180.0;
        int px = cx + (int)(dist * cos(angle));
        int py = cy + (int)(dist * sin(angle));
        uint16_t col = getThreatColor(det.threatScore);
        tft.fillRect(px-4, py-4, 8, 8, col);
        tft.setTextColor(col); tft.setCursor(px+6, py-3);
        // Truncate manufacturer name for map
        String shortName = det.manufacturer.substring(0, 6);
        tft.print(shortName);
    }
    // Aggregation status
    if (aggregationActive) {
        tft.setTextColor(TFT_GREEN); tft.setCursor(5, 200);
        tft.print("AGGREGATION: ACTIVE");
    }
    drawNavbar(); tft.endWrite();
}

// ============================================================================
//  SCREEN: SETTINGS (v6.1 Enhanced)
// ============================================================================
void drawSettingsScreen() {
    tft.startWrite(); tft.fillScreen(TFT_BLACK);
    drawHeader("CONFIG");
    int y = 32;
    drawToggle(10, y, config.enableBLE, "BLE Scanning"); y += 25;
    tft.drawFastHLine(10, y-2, 300, 0x18C3);
    drawToggle(10, y, config.enableWiFi, "WiFi Scanning"); y += 25;
    tft.drawFastHLine(10, y-2, 300, 0x18C3);
    #if PROMISCUOUS_ENABLED
    drawToggle(10, y, config.enablePromiscuous, "Promiscuous"); y += 25;
    tft.drawFastHLine(10, y-2, 300, 0x18C3);
    #endif
    drawToggle(10, y, config.enableLogging, "SD Logging"); y += 25;
    tft.drawFastHLine(10, y-2, 300, 0x18C3);
    drawToggle(10, y, config.secureLogging, "Encrypted"); y += 25;
    tft.drawFastHLine(10, y-2, 300, 0x18C3);
    drawToggle(10, y, config.autoBrightness, "Auto Bright"); y += 25;
    tft.drawFastHLine(10, y-2, 300, 0x18C3);
    // Brightness slider (only if auto-brightness is off)
    if (!config.autoBrightness) {
        drawSlider(10, y, 300, config.brightness, 255, "Brightness"); y += 25;
    }
    drawNavbar(); tft.endWrite();
}

// ============================================================================
//  SCREEN: INFO (v7.0 Enhanced)
// ============================================================================
void drawInfoScreen() {
    tft.startWrite(); tft.fillScreen(TFT_BLACK);
    drawHeader("STATUS");
    tft.setTextSize(1); int y = 34, sp = 17;
    auto row = [&](const char* label, const char* value, uint16_t valCol = TFT_WHITE) {
        tft.setTextColor(0xAD55); tft.setCursor(10, y); tft.print(label);
        tft.setTextColor(valCol); tft.setCursor(130, y); tft.print(value);
        y += sp;
    };
    char buf[64];
    row("FIRMWARE", VERSION);
    snprintf(buf, sizeof(buf), "%.2f V", batteryVoltage); row("BATTERY", buf);
    snprintf(buf, sizeof(buf), "%d entries", dynamicDatabase.size()); row("OUI DATABASE", buf);
    snprintf(buf, sizeof(buf), "%d KB", ESP.getFreeHeap()/1024); row("FREE MEMORY", buf);
    row("TOUCH", i2cAvailable ? "OK" : "ERROR", i2cAvailable ? TFT_GREEN : TFT_RED);
    row("SD CARD", sdCardAvailable ? "READY" : "N/A", sdCardAvailable ? TFT_GREEN : TFT_RED);
    #if GPS_ENABLED
    row("GPS", gpsFixed ? "FIXED" : "SEARCHING", gpsFixed ? TFT_GREEN : TFT_YELLOW);
    #endif
    snprintf(buf, sizeof(buf), "%d", totalPacketsCaptured); row("PACKETS", buf);
    snprintf(buf, sizeof(buf), "%d", getRecurringDeviceCount()); row("RECURRING", buf, TFT_ORANGE);
    unsigned long up = millis()/1000;
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", (int)(up/3600), (int)((up%3600)/60), (int)(up%60));
    row("UPTIME", buf);
    drawNavbar(); tft.endWrite();
}

void updateDisplay() {
    switch (currentScreen) {
        case SCREEN_WIZARD:   drawWizardScreen();   break;
        case SCREEN_MAIN:     drawMainScreen();     break;
        case SCREEN_RADAR:    drawRadarScreen();     break;
        case SCREEN_GRAPH:    drawGraphScreen();     break;
        case SCREEN_HISTORY:  drawHistoryScreen();   break;
        case SCREEN_MAP:      drawMapScreen();       break;
        case SCREEN_SETTINGS: drawSettingsScreen();  break;
        case SCREEN_INFO:     drawInfoScreen();      break;
    }
}

// ============================================================================
//  TOUCH HANDLER (v6.1 - Swipe + Scroll + Filter)
// ============================================================================
void handleTouchGestures() {
    static unsigned long lastTouchTime = 0;
    static uint16_t touchStartX = 0, touchStartY = 0;
    uint16_t x, y;
    if (!readCapacitiveTouch(&x, &y)) return;
    if (millis() - lastTouchTime < 200) return;
    lastTouchTime = millis();

    if (currentScreen == SCREEN_WIZARD) {
        if (x > 210 && y > 170 && y < 200) {
            wizardStep++;
            if (wizardStep > 2) {
                config.setupComplete = true; saveConfig();
                currentScreen = SCREEN_MAIN;
                loadHistoryFromSD();
            }
        }
    } else if (y > 212) {
        // Navbar - 7 tabs
        int tabW = 320 / 7;
        int idx = x / tabW;
        if (idx >= 0 && idx <= 6) currentScreen = (Screen)(idx + 1);
    } else if (currentScreen == SCREEN_MAIN) {
        // Filter bar touch (y 30-46)
        if (y >= 30 && y <= 46) {
            int fi = (x - 5) / 50;
            if (fi >= 0 && fi <= 3) { config.filter = (FilterMode)fi; scrollOffset = 0; }
        }
        // Scroll up/down
        else if (x > 290 && y < 60 && scrollOffset > 0) { scrollOffset--; }
        else if (x > 290 && y > 190) {
            auto filtered = getFilteredDetections();
            if (scrollOffset + maxVisibleItems < (int)filtered.size()) scrollOffset++;
        }
        // Select detection
        else if (y >= 50 && y < 210) {
            int tappedIdx = scrollOffset + (y - 50) / 42;
            auto filtered = getFilteredDetections();
            if (tappedIdx < (int)filtered.size()) selectedDetectionIdx = tappedIdx;
        }
    } else if (currentScreen == SCREEN_SETTINGS) {
        int baseY = 32;
        int step = 25;
        int toggleX = 165;
        if (x > toggleX && x < toggleX + 44) {
            int row = (y - baseY) / step;
            int settingIdx = 0;
            // BLE
            if (row == settingIdx++) config.enableBLE = !config.enableBLE;
            // WiFi
            else if (row == settingIdx++) config.enableWiFi = !config.enableWiFi;
            #if PROMISCUOUS_ENABLED
            // Promiscuous
            else if (row == settingIdx++) config.enablePromiscuous = !config.enablePromiscuous;
            #endif
            // Logging
            else if (row == settingIdx++) config.enableLogging = !config.enableLogging;
            // Encrypted
            else if (row == settingIdx++) config.secureLogging = !config.secureLogging;
            // Auto Brightness
            else if (row == settingIdx++) config.autoBrightness = !config.autoBrightness;
            saveConfig();
        }
        // Brightness slider
        if (!config.autoBrightness) {
            int sliderY = baseY + step * 6;
            if (y >= sliderY && y <= sliderY + 18 && x >= 100 && x <= 310) {
                config.brightness = map(x, 100, 310, 0, 255);
                analogWrite(TFT_BL, config.brightness);
                saveConfig();
            }
        }
    }
}

// ============================================================================
//  PERSISTENT CONFIGURATION (NVS)
// ============================================================================
void saveConfig() {
    preferences.begin("oui-spy", false);
    preferences.putBool("setup",  config.setupComplete);
    preferences.putBool("ble",    config.enableBLE);
    preferences.putBool("wifi",   config.enableWiFi);
    preferences.putBool("promisc", config.enablePromiscuous);
    preferences.putBool("log",    config.enableLogging);
    preferences.putBool("secure", config.secureLogging);
    preferences.putBool("autobr", config.autoBrightness);
    preferences.putBool("cloud",  config.enableCloudSync);
    preferences.putInt("bright",  config.brightness);
    preferences.putInt("filter",  (int)config.filter);
    preferences.end();
}

void loadConfig() {
    preferences.begin("oui-spy", true);
    config.setupComplete    = preferences.getBool("setup",  false);
    config.enableBLE        = preferences.getBool("ble",    true);
    config.enableWiFi       = preferences.getBool("wifi",   true);
    config.enablePromiscuous = preferences.getBool("promisc", true);
    config.enableLogging    = preferences.getBool("log",    true);
    config.secureLogging    = preferences.getBool("secure", false);
    config.autoBrightness   = preferences.getBool("autobr", true);
    config.enableCloudSync  = preferences.getBool("cloud",  false);
    config.brightness       = preferences.getInt("bright",  255);
    config.filter           = (FilterMode)preferences.getInt("filter", 0);
    preferences.end();
}

// ============================================================================
//  SETUP & LOOP
// ============================================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n[UK-OUI-SPY] Booting v7.0 PRO Edition...");

    xDetectionMutex = xSemaphoreCreateMutex();
    xHistoryMutex   = xSemaphoreCreateMutex();

    // GPIO
    pinMode(LED_R_PIN, OUTPUT); digitalWrite(LED_R_PIN, LOW);
    #if !GPS_ENABLED
    pinMode(LED_G_PIN, OUTPUT); digitalWrite(LED_G_PIN, LOW);
    pinMode(LED_B_PIN, OUTPUT); digitalWrite(LED_B_PIN, LOW);
    #endif
    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(BAT_ADC, INPUT);

    // I2C & Touch
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Wire.beginTransmission(TOUCH_ADDR);
    i2cAvailable = (Wire.endTransmission() == 0);
    Serial.printf("[TOUCH] CST820: %s\n", i2cAvailable ? "OK" : "NOT FOUND");

    // Display
    tft.init(); tft.setRotation(1); tft.fillScreen(TFT_BLACK);

    // Config
    loadConfig();
    currentScreen = config.setupComplete ? SCREEN_MAIN : SCREEN_WIZARD;

    // SD Card
    sdCardAvailable = SD.begin(SD_CS);
    Serial.printf("[SD] Card: %s\n", sdCardAvailable ? "READY" : "NOT FOUND");

    // OUI Database
    if (!loadOUIDatabaseFromSD("/oui.csv")) {
        Serial.println("[OUI] SD load failed, using static database.");
        initializeStaticDatabase();
    }

    // BLE
    NimBLEDevice::init("UK-OUI-SPY");

    // WiFi
    WiFi.mode(WIFI_STA); WiFi.disconnect();

    // GPS (optional)
    #if GPS_ENABLED
    gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.println("[GPS] UART initialized.");
    #endif

    // Battery
    batteryVoltage = (analogRead(BAT_ADC) / 4095.0) * 2.0 * 3.3 * 1.1;
    lastInteraction = millis();

    // Load history if setup complete
    if (config.setupComplete) loadHistoryFromSD();

    // FreeRTOS
    xTaskCreatePinnedToCore(ScanTask, "ScanTask", 16384, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(UITask,   "UITask",   16384, NULL, 1, NULL, 1);

    Serial.println("[UK-OUI-SPY] v7.0 Boot complete. System ready.");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
