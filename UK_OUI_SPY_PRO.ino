/*
 * ============================================================================
 *  UK-OUI-SPY PRO EDITION - v2.4.0
 *  Professional UK Surveillance Device Detector
 * ============================================================================
 *
 *  Hardware Target: ESP32-2432S028 (CYD - 2.8" ILI9341 TFT + CST820 Touch)
 *
 *  FEATURES:
 *    - Dual-Core FreeRTOS (Core 0: Scanning, Core 1: UI)
 *    - O(1) OUI Lookup via unordered_map
 *    - BLE + WiFi Surveillance Detection
 *    - Dynamic OUI Database (SD Card CSV) with Static Fallback
 *    - AES-128 Encrypted Secure Logging
 *    - Proximity Radar Visualization
 *    - First-Time Setup Wizard
 *    - Persistent Settings (NVS)
 *    - Auto-Brightness (LDR)
 *    - Battery Monitoring
 *    - Deep Sleep with Wake-on-Touch
 *    - Premium Multi-Page Touch UI
 *
 *  ARDUINO IDE SETUP:
 *    Board:     ESP32 Dev Module
 *    Partition:  Minimal SPIFFS (1.9MB APP / 190KB SPIFFS)
 *    Speed:     115200
 *
 *  REQUIRED LIBRARIES (Install via Library Manager):
 *    - TFT_eSPI by Bodmer (v2.5.43+)
 *    - NimBLE-Arduino by h2zero (v1.4.1+)
 *    - ArduinoJson by Benoit Blanchon (v6.21+)
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
 *    #define LOAD_GLCD
 *    #define LOAD_FONT2
 *    #define LOAD_FONT4
 *    #define LOAD_FONT6
 *    #define LOAD_FONT7
 *    #define LOAD_FONT8
 *    #define LOAD_GFXFF
 *    #define SMOOTH_FONT
 *    #define SPI_FREQUENCY  20000000
 *    #define SPI_READ_FREQUENCY 16000000
 *
 *  SD CARD:
 *    Place oui.csv in the root of the microSD card.
 *    Format: OUI,Manufacturer,Category,Relevance,Deployment,Notes
 *
 * ============================================================================
 */

#define VERSION "2.4.0-PRO"

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
#include "esp_sleep.h"
#include "mbedtls/aes.h"

// ============================================================================
//  HARDWARE PIN DEFINITIONS (ESP32-2432S028)
// ============================================================================
#define TOUCH_ADDR   0x15    // CST820 I2C Address
#define TOUCH_SDA    27      // I2C Data
#define TOUCH_SCL    22      // I2C Clock
#define TFT_BL       21      // Backlight (Active HIGH)
#define SD_CS        5       // SD Card Chip Select
#define BUZZER_PIN   25      // Passive Buzzer / DAC
#define LED_R_PIN    4       // RGB LED - Red
#define LED_G_PIN    16      // RGB LED - Green
#define LED_B_PIN    17      // RGB LED - Blue
#define LDR_PIN      34      // Light Dependent Resistor (ADC)
#define BAT_ADC      35      // Battery Voltage Divider (ADC)

// ============================================================================
//  ENUMERATIONS
// ============================================================================
enum Screen {
    SCREEN_WIZARD,
    SCREEN_MAIN,
    SCREEN_RADAR,
    SCREEN_SETTINGS,
    SCREEN_INFO
};

enum DeviceCategory {
    CAT_UNKNOWN = 0,
    CAT_CCTV = 1,
    CAT_ANPR = 2,
    CAT_DRONE = 3,
    CAT_BODYCAM = 4,
    CAT_CLOUD_CCTV = 5,
    CAT_TRAFFIC = 6,
    CAT_DASH_CAM = 7,
    CAT_DOORBELL_CAM = 8,
    CAT_FACIAL_RECOG = 9,
    CAT_PARKING_ENFORCEMENT = 10,
    CAT_SMART_CITY_INFRA = 11,
    CAT_MAX = 11
};

enum RelevanceLevel {
    REL_LOW = 0,
    REL_MEDIUM = 1,
    REL_HIGH = 2,
    REL_MAX = 2
};

enum DeploymentType {
    DEPLOY_POLICE = 0,
    DEPLOY_COUNCIL = 1,
    DEPLOY_TRANSPORT = 2,
    DEPLOY_RETAIL = 3,
    DEPLOY_PRIVATE = 4,
    DEPLOY_GOVERNMENT = 5,
    DEPLOY_MAX = 5
};

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
    bool setupComplete    = false;
    bool enableBLE        = true;
    bool enableWiFi       = true;
    bool enableLogging    = true;
    bool secureLogging    = false;
    bool autoBrightness   = true;
    int  brightness       = 255;
    int  sleepTimeout     = 300;  // seconds
    char encryptionKey[17] = "UK-OUI-SPY-2026";
};

// ============================================================================
//  GLOBAL OBJECTS
// ============================================================================
TFT_eSPI tft = TFT_eSPI();
Config config;
Preferences preferences;

std::vector<OUIEntry> dynamicDatabase;
std::unordered_map<String, OUIEntry*> ouiLookup;
std::vector<Detection> detections;

SemaphoreHandle_t xDetectionMutex;

Screen currentScreen         = SCREEN_WIZARD;
const int MAX_DETECTIONS     = 50;
unsigned long lastScanTime   = 0;
unsigned long lastInteraction = 0;
int scanInterval             = 5000;
bool scanning                = false;
bool sdCardAvailable         = false;
bool i2cAvailable            = false;
float batteryVoltage         = 0.0;
int wizardStep               = 0;

// ============================================================================
//  OUI DATABASE ENGINE
// ============================================================================
const char* getCategoryName(DeviceCategory cat) {
    switch (cat) {
        case CAT_CCTV:                return "CCTV";
        case CAT_ANPR:                return "ANPR";
        case CAT_DRONE:               return "Drone";
        case CAT_BODYCAM:             return "Body Cam";
        case CAT_CLOUD_CCTV:          return "Cloud CCTV";
        case CAT_TRAFFIC:             return "Traffic";
        case CAT_DASH_CAM:            return "Dash Cam";
        case CAT_DOORBELL_CAM:        return "Doorbell";
        case CAT_FACIAL_RECOG:        return "Face Recog";
        case CAT_PARKING_ENFORCEMENT: return "Parking";
        case CAT_SMART_CITY_INFRA:    return "Smart Pole";
        default:                      return "Unknown";
    }
}

const char* getRelevanceName(RelevanceLevel rel) {
    switch (rel) {
        case REL_HIGH:   return "HIGH";
        case REL_MEDIUM: return "MEDIUM";
        case REL_LOW:    return "LOW";
        default:         return "UNKNOWN";
    }
}

const char* getDeploymentName(DeploymentType dep) {
    switch (dep) {
        case DEPLOY_POLICE:     return "Police";
        case DEPLOY_COUNCIL:    return "Council";
        case DEPLOY_TRANSPORT:  return "Transport";
        case DEPLOY_RETAIL:     return "Retail";
        case DEPLOY_PRIVATE:    return "Private";
        case DEPLOY_GOVERNMENT: return "Government";
        default:                return "Unknown";
    }
}

uint16_t getCategoryColor(DeviceCategory cat) {
    switch (cat) {
        case CAT_CCTV:                return TFT_RED;
        case CAT_ANPR:                return TFT_ORANGE;
        case CAT_DRONE:               return TFT_MAGENTA;
        case CAT_BODYCAM:             return TFT_RED;
        case CAT_CLOUD_CCTV:          return TFT_BLUE;
        case CAT_TRAFFIC:             return TFT_ORANGE;
        case CAT_DASH_CAM:            return TFT_CYAN;
        case CAT_DOORBELL_CAM:        return TFT_CYAN;
        case CAT_FACIAL_RECOG:        return TFT_PURPLE;
        case CAT_PARKING_ENFORCEMENT: return TFT_YELLOW;
        case CAT_SMART_CITY_INFRA:    return TFT_YELLOW;
        default:                      return TFT_WHITE;
    }
}

uint16_t getRelevanceColor(RelevanceLevel rel) {
    switch (rel) {
        case REL_HIGH:   return TFT_RED;
        case REL_MEDIUM: return TFT_YELLOW;
        case REL_LOW:    return TFT_GREEN;
        default:         return TFT_WHITE;
    }
}

void rebuildLookupTable() {
    ouiLookup.clear();
    for (auto& entry : dynamicDatabase) {
        ouiLookup[entry.oui] = &entry;
    }
}

bool loadOUIDatabaseFromSD(const char* path) {
    if (!SD.exists(path)) return false;
    File file = SD.open(path);
    if (!file) return false;

    dynamicDatabase.clear();
    if (file.available()) file.readStringUntil('\n'); // Skip CSV header

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() < 10) continue;

        // CSV: OUI,Manufacturer,Category,Relevance,Deployment,Notes
        const int EXPECTED_FIELDS = 6;
        String fields[EXPECTED_FIELDS];
        int fieldIndex = 0;
        int start = 0;

        while (fieldIndex < EXPECTED_FIELDS - 1) {
            int commaPos = line.indexOf(',', start);
            if (commaPos < 0) break;
            fields[fieldIndex++] = line.substring(start, commaPos);
            start = commaPos + 1;
        }
        if (fieldIndex < EXPECTED_FIELDS) {
            fields[fieldIndex++] = line.substring(start);
        }
        if (fieldIndex < EXPECTED_FIELDS) {
            Serial.printf("[OUI] Skipping malformed line: %s\n", line.c_str());
            continue;
        }

        int catVal = fields[2].toInt();
        int relVal = fields[3].toInt();
        int depVal = fields[4].toInt();

        if (catVal < 0 || catVal > CAT_MAX ||
            relVal < 0 || relVal > REL_MAX ||
            depVal < 0 || depVal > DEPLOY_MAX) {
            Serial.printf("[OUI] Invalid enum in line: %s\n", line.c_str());
            continue;
        }

        OUIEntry entry;
        entry.oui          = fields[0];
        entry.manufacturer = fields[1];
        entry.category     = (DeviceCategory)catVal;
        entry.relevance    = (RelevanceLevel)relVal;
        entry.deployment   = (DeploymentType)depVal;
        entry.notes        = fields[5];
        dynamicDatabase.push_back(entry);
    }
    file.close();

    if (!dynamicDatabase.empty()) {
        rebuildLookupTable();
        Serial.printf("[OUI] Loaded %d entries from SD card.\n", dynamicDatabase.size());
        return true;
    }
    return false;
}

void initializeStaticDatabase() {
    dynamicDatabase.clear();
    // UK Surveillance Manufacturers - Static Fallback
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
    dynamicDatabase.push_back({"00:1C:B3", "Apple",           CAT_UNKNOWN,    REL_LOW,    DEPLOY_PRIVATE,    "Consumer Device"});
    dynamicDatabase.push_back({"B8:27:EB", "Raspberry Pi",    CAT_UNKNOWN,    REL_MEDIUM, DEPLOY_PRIVATE,    "Often used in DIY surveillance"});
    dynamicDatabase.push_back({"DC:A6:32", "Raspberry Pi",    CAT_UNKNOWN,    REL_MEDIUM, DEPLOY_PRIVATE,    "RPi 4 OUI"});
    dynamicDatabase.push_back({"E4:5F:01", "Raspberry Pi",    CAT_UNKNOWN,    REL_MEDIUM, DEPLOY_PRIVATE,    "RPi 5 OUI"});
    dynamicDatabase.push_back({"00:80:F0", "Panasonic",       CAT_CCTV,       REL_HIGH,   DEPLOY_TRANSPORT,  "Transport CCTV"});
    dynamicDatabase.push_back({"00:0E:8F", "Cisco",           CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_GOVERNMENT, "Network Infrastructure"});
    dynamicDatabase.push_back({"00:50:56", "VMware",          CAT_UNKNOWN,    REL_LOW,    DEPLOY_GOVERNMENT, "Virtual Infrastructure"});
    dynamicDatabase.push_back({"00:1B:C5", "Vivotek",         CAT_CCTV,       REL_HIGH,   DEPLOY_COUNCIL,    "IP Camera Manufacturer"});
    dynamicDatabase.push_back({"00:02:D1", "Vivotek",         CAT_CCTV,       REL_HIGH,   DEPLOY_COUNCIL,    "Vivotek Alt OUI"});
    dynamicDatabase.push_back({"00:0F:7C", "ACTi",            CAT_CCTV,       REL_HIGH,   DEPLOY_COUNCIL,    "IP Camera Manufacturer"});
    dynamicDatabase.push_back({"00:1A:3F", "Intelbras",       CAT_CCTV,       REL_MEDIUM, DEPLOY_RETAIL,     "CCTV Manufacturer"});
    dynamicDatabase.push_back({"00:1F:54", "Samsung Techwin", CAT_CCTV,       REL_HIGH,   DEPLOY_TRANSPORT,  "Hanwha/Samsung CCTV"});
    dynamicDatabase.push_back({"00:09:18", "Samsung Techwin", CAT_CCTV,       REL_HIGH,   DEPLOY_TRANSPORT,  "Samsung CCTV Alt OUI"});
    dynamicDatabase.push_back({"00:16:6C", "Samsung",         CAT_CCTV,       REL_MEDIUM, DEPLOY_RETAIL,     "Samsung Electronics"});
    dynamicDatabase.push_back({"00:04:A3", "Pelco",           CAT_CCTV,       REL_HIGH,   DEPLOY_GOVERNMENT, "Schneider Electric CCTV"});
    dynamicDatabase.push_back({"00:30:53", "Bosch Security",  CAT_CCTV,       REL_HIGH,   DEPLOY_GOVERNMENT, "Bosch Security Systems"});
    dynamicDatabase.push_back({"00:07:5F", "Bosch Security",  CAT_CCTV,       REL_HIGH,   DEPLOY_GOVERNMENT, "Bosch Alt OUI"});
    dynamicDatabase.push_back({"00:04:BF", "Milestone",       CAT_CCTV,       REL_HIGH,   DEPLOY_POLICE,     "VMS Platform"});
    dynamicDatabase.push_back({"00:1A:6B", "Uniview",         CAT_CCTV,       REL_HIGH,   DEPLOY_COUNCIL,    "Chinese CCTV Manufacturer"});
    dynamicDatabase.push_back({"24:28:FD", "Uniview",         CAT_CCTV,       REL_HIGH,   DEPLOY_COUNCIL,    "Uniview Alt OUI"});
    dynamicDatabase.push_back({"00:E0:FC", "Huawei",          CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_GOVERNMENT, "Smart City Infrastructure"});
    dynamicDatabase.push_back({"48:46:FB", "Huawei",          CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_GOVERNMENT, "Huawei Alt OUI"});
    dynamicDatabase.push_back({"70:A8:D3", "Huawei",          CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_GOVERNMENT, "Huawei Alt OUI"});
    dynamicDatabase.push_back({"00:1E:67", "Intel",           CAT_UNKNOWN,    REL_LOW,    DEPLOY_PRIVATE,    "Intel NUC / Edge Compute"});
    dynamicDatabase.push_back({"00:30:AB", "FLIR",            CAT_CCTV,       REL_HIGH,   DEPLOY_POLICE,     "Thermal Imaging"});
    dynamicDatabase.push_back({"00:40:7F", "Honeywell",       CAT_CCTV,       REL_HIGH,   DEPLOY_GOVERNMENT, "Security Systems"});
    dynamicDatabase.push_back({"00:0A:F5", "Airgo Networks",  CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "WiFi Infrastructure"});
    dynamicDatabase.push_back({"00:1C:0E", "Cisco Meraki",    CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "Cloud-Managed WiFi"});
    dynamicDatabase.push_back({"00:18:0A", "Cisco Meraki",    CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "Meraki Alt OUI"});
    dynamicDatabase.push_back({"00:26:CB", "Cisco Meraki",    CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "Meraki Alt OUI"});
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

    Wire.read(); // Status register
    if ((Wire.read() & 0x0F) == 0) return false; // No touch points

    uint8_t xH = Wire.read(); uint8_t xL = Wire.read();
    uint8_t yH = Wire.read(); uint8_t yL = Wire.read();
    uint16_t rawX = ((xH & 0x0F) << 8) | xL;
    uint16_t rawY = ((yH & 0x0F) << 8) | yL;

    // Map portrait coordinates to landscape (Rotation 1)
    *x = rawY;
    *y = 239 - rawX;

    lastInteraction = millis();
    return true;
}

// ============================================================================
//  BLE SCAN CALLBACK
// ============================================================================
void checkOUI(String macAddress, int8_t rssi, bool isBLE); // Forward declaration

class SurveillanceScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* device) {
        checkOUI(device->getAddress().toString().c_str(), device->getRSSI(), true);
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
    int n = WiFi.scanNetworks(false, true, false, 150);
    for (int i = 0; i < n; ++i) {
        checkOUI(WiFi.BSSIDstr(i), WiFi.RSSI(i), false);
    }
    WiFi.scanDelete();
}

void checkOUI(String macAddress, int8_t rssi, bool isBLE) {
    String mac = macAddress;
    mac.toUpperCase();
    String oui = mac.substring(0, 8);

    auto it = ouiLookup.find(oui);
    if (it != ouiLookup.end()) {
        Detection det;
        det.macAddress   = mac;
        det.manufacturer = it->second->manufacturer;
        det.category     = it->second->category;
        det.relevance    = it->second->relevance;
        det.rssi         = rssi;
        det.timestamp    = millis();
        det.isBLE        = isBLE;

        // Thread-safe insertion
        xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
        bool found = false;
        for (auto &d : detections) {
            if (d.macAddress == det.macAddress) {
                d.rssi = det.rssi;
                d.timestamp = millis();
                found = true;
                break;
            }
        }
        if (!found) {
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
            if (config.secureLogging) {
                // AES-128 ECB Encryption
                mbedtls_aes_context aes;
                unsigned char key[16];
                memcpy(key, config.encryptionKey, 16);
                // Encrypt in 16-byte blocks
                int len = logLine.length();
                int blocks = (len / 16) + 1;
                unsigned char* input  = (unsigned char*)calloc(blocks * 16, 1);
                unsigned char* output = (unsigned char*)calloc(blocks * 16, 1);
                memcpy(input, logLine.c_str(), len);
                mbedtls_aes_init(&aes);
                mbedtls_aes_setkey_enc(&aes, key, 128);
                for (int b = 0; b < blocks; b++) {
                    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT,
                                          input + (b * 16), output + (b * 16));
                }
                mbedtls_aes_free(&aes);
                File f = SD.open("/secure.log", FILE_APPEND);
                if (f) {
                    // Write block count header + encrypted data
                    uint8_t bc = (uint8_t)blocks;
                    f.write(&bc, 1);
                    f.write(output, blocks * 16);
                    f.close();
                }
                free(input);
                free(output);
            } else {
                File f = SD.open("/detections.csv", FILE_APPEND);
                if (f) { f.println(logLine); f.close(); }
            }
        }

        // Alert for HIGH relevance
        if (det.relevance == REL_HIGH) {
            tone(BUZZER_PIN, 2000, 150);
            digitalWrite(LED_R_PIN, HIGH);
            delay(100);
            digitalWrite(LED_R_PIN, LOW);
        }
    }
}

// ============================================================================
//  FREERTOS TASKS
// ============================================================================
void ScanTask(void *pvParameters) {
    for (;;) {
        if (config.setupComplete && (millis() - lastScanTime >= (unsigned long)scanInterval)) {
            scanning = true;
            digitalWrite(LED_G_PIN, HIGH);
            if (config.enableBLE) scanBLE();
            if (config.enableWiFi) scanWiFi();
            scanning = false;
            digitalWrite(LED_G_PIN, LOW);
            lastScanTime = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void UITask(void *pvParameters) {
    for (;;) {
        handleTouchGestures();

        // Auto-Brightness
        if (config.autoBrightness) {
            static unsigned long lastLDR = 0;
            if (millis() - lastLDR > 2000) {
                int ldr = analogRead(LDR_PIN);
                int br = map(ldr, 0, 4095, 50, 255);
                config.brightness = constrain(br, 0, 255);
                analogWrite(TFT_BL, config.brightness);
                lastLDR = millis();
            }
        }

        // Battery
        batteryVoltage = (analogRead(BAT_ADC) / 4095.0) * 2.0 * 3.3 * 1.1;

        // Render
        updateDisplay();

        // Deep Sleep on inactivity
        if (config.setupComplete &&
            (millis() - lastInteraction > (unsigned long)(config.sleepTimeout * 1000))) {
            tft.fillScreen(TFT_BLACK);
            analogWrite(TFT_BL, 0);
            digitalWrite(LED_R_PIN, LOW);
            digitalWrite(LED_G_PIN, LOW);
            digitalWrite(LED_B_PIN, LOW);
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
    tft.fillRect(0, 0, 320, 30, 0x1082);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 7);
    tft.print(title);

    // Scanning indicator
    if (scanning) tft.fillCircle(245, 15, 4, TFT_CYAN);

    // SD indicator
    if (sdCardAvailable) {
        tft.fillRect(255, 10, 8, 10, TFT_GREEN);
    }

    // Battery icon
    tft.drawRect(270, 8, 25, 12, TFT_WHITE);
    tft.fillRect(295, 11, 2, 6, TFT_WHITE);
    int batPct = constrain(map((int)(batteryVoltage * 100), 330, 420, 0, 100), 0, 100);
    int batW = map(batPct, 0, 100, 0, 21);
    uint16_t batCol = batPct < 20 ? TFT_RED : (batPct < 50 ? TFT_YELLOW : TFT_GREEN);
    tft.fillRect(272, 10, batW, 8, batCol);

    // Battery percentage
    tft.setTextSize(1);
    tft.setCursor(300, 12);
    tft.printf("%d%%", batPct);
}

void drawNavbar() {
    tft.fillRect(0, 210, 320, 30, 0x0841);
    tft.drawFastHLine(0, 210, 320, 0x2104);
    tft.setTextSize(1);
    const char* labels[] = {"LIST", "RADAR", "CONFIG", "INFO"};
    for (int i = 0; i < 4; i++) {
        bool active = (currentScreen == (Screen)(i + 1));
        if (active) {
            tft.fillRoundRect(i * 80, 212, 78, 26, 3, 0x2104);
        }
        tft.setTextColor(active ? TFT_CYAN : 0x7BEF);
        tft.setCursor(15 + (i * 80), 220);
        tft.print(labels[i]);
    }
}

void drawToggle(int x, int y, bool state, const char* label) {
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(x, y + 6);
    tft.print(label);

    // Track background
    uint16_t trackCol = state ? 0x0410 : 0x4208;
    tft.fillRoundRect(x + 160, y, 50, 22, 11, trackCol);

    // Knob
    if (state) {
        tft.fillCircle(x + 198, y + 11, 9, TFT_CYAN);
    } else {
        tft.fillCircle(x + 172, y + 11, 9, 0x7BEF);
    }
}

void drawWizardScreen() {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);

    // Branded header
    tft.fillRect(0, 0, 320, 30, 0x1082);
    tft.setTextColor(TFT_CYAN);
    tft.setTextSize(2);
    tft.setCursor(10, 7);
    tft.print("UK-OUI-SPY PRO");

    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);

    if (wizardStep == 0) {
        tft.setCursor(40, 60);
        tft.setTextSize(2);
        tft.print("WELCOME");
        tft.setTextSize(1);
        tft.setCursor(20, 95);
        tft.print("Professional Surveillance");
        tft.setCursor(20, 110);
        tft.print("Device Detection System");
        tft.setCursor(20, 140);
        tft.setTextColor(0xAD55);
        tft.print("Tap NEXT to begin setup.");
    } else if (wizardStep == 1) {
        tft.setCursor(40, 60);
        tft.setTextSize(2);
        tft.print("HARDWARE CHECK");
        tft.setTextSize(1);
        tft.setCursor(20, 95);
        tft.printf("Touch: %s", i2cAvailable ? "PASS" : "FAIL");
        tft.setCursor(20, 110);
        tft.printf("SD Card: %s", sdCardAvailable ? "PASS" : "NOT FOUND");
        tft.setCursor(20, 125);
        tft.printf("OUI DB: %d entries", dynamicDatabase.size());
        tft.setCursor(20, 140);
        tft.printf("Battery: %.2f V", batteryVoltage);
    } else if (wizardStep == 2) {
        tft.setCursor(40, 60);
        tft.setTextSize(2);
        tft.setTextColor(TFT_GREEN);
        tft.print("READY");
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(20, 95);
        tft.print("All systems operational.");
        tft.setCursor(20, 115);
        tft.print("Device is ready for field use.");
        tft.setCursor(20, 145);
        tft.setTextColor(0xAD55);
        tft.print("Tap FINISH to start scanning.");
    }

    // Button
    tft.fillRoundRect(200, 170, 100, 30, 6, TFT_CYAN);
    tft.setTextColor(TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(215, 176);
    tft.print(wizardStep < 2 ? "NEXT" : "GO");

    tft.endWrite();
}

void drawMainScreen() {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    drawHeader("DETECTIONS");

    xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
    auto snapshot = detections;
    xSemaphoreGive(xDetectionMutex);

    if (snapshot.empty()) {
        tft.setTextColor(0x7BEF);
        tft.setTextSize(1);
        tft.setCursor(80, 110);
        tft.print("Scanning for devices...");
    } else {
        int y = 35;
        for (auto &det : snapshot) {
            if (y > 195) break;

            // Card background
            tft.fillRoundRect(3, y, 314, 34, 4, 0x18C3);

            // Relevance indicator bar
            tft.fillRect(3, y, 4, 34, getRelevanceColor(det.relevance));

            // Manufacturer name
            tft.setTextColor(TFT_WHITE);
            tft.setTextSize(1);
            tft.setCursor(14, y + 4);
            tft.print(det.manufacturer);

            // BLE/WiFi badge
            tft.setTextColor(det.isBLE ? TFT_BLUE : TFT_ORANGE);
            tft.setCursor(260, y + 4);
            tft.print(det.isBLE ? "BLE" : "WiFi");

            // Category + RSSI
            tft.setTextColor(0xAD55);
            tft.setCursor(14, y + 18);
            tft.printf("%s | %s | %d dBm",
                       getCategoryName(det.category),
                       getRelevanceName(det.relevance),
                       det.rssi);

            y += 38;
        }
    }

    drawNavbar();
    tft.endWrite();
}

void drawRadarScreen() {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    drawHeader("RADAR");

    int cx = 160, cy = 118, r = 78;

    // Radar rings
    for (int i = 1; i <= 3; i++) {
        tft.drawCircle(cx, cy, (r * i) / 3, 0x18C3);
    }
    // Crosshair
    tft.drawLine(cx - r, cy, cx + r, cy, 0x18C3);
    tft.drawLine(cx, cy - r, cx, cy + r, 0x18C3);

    // Range labels
    tft.setTextColor(0x4208);
    tft.setTextSize(1);
    tft.setCursor(cx + (r / 3) + 2, cy + 2);
    tft.print("NEAR");
    tft.setCursor(cx + r - 20, cy + 2);
    tft.print("FAR");

    // Centre dot
    tft.fillCircle(cx, cy, 3, TFT_CYAN);

    xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
    auto snapshot = detections;
    xSemaphoreGive(xDetectionMutex);

    // Plot detections
    int idx = 0;
    for (auto &det : snapshot) {
        float dist = map(constrain(det.rssi, -100, -30), -100, -30, r, 8);
        // Deterministic angle based on MAC hash for stable positioning
        unsigned long hash = 0;
        for (int c = 0; c < (int)det.macAddress.length(); c++) {
            hash = hash * 31 + det.macAddress.charAt(c);
        }
        float angle = (float)(hash % 360) * PI / 180.0;
        int px = cx + (int)(dist * cos(angle));
        int py = cy + (int)(dist * sin(angle));

        uint16_t col = getRelevanceColor(det.relevance);
        tft.fillCircle(px, py, 5, col);
        tft.drawCircle(px, py, 7, col);
        idx++;
    }

    drawNavbar();
    tft.endWrite();
}

void drawSettingsScreen() {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    drawHeader("CONFIG");

    drawToggle(15, 45, config.enableBLE, "BLE Scanning");
    tft.drawFastHLine(15, 72, 290, 0x18C3);

    drawToggle(15, 78, config.enableWiFi, "WiFi Scanning");
    tft.drawFastHLine(15, 105, 290, 0x18C3);

    drawToggle(15, 111, config.enableLogging, "SD Logging");
    tft.drawFastHLine(15, 138, 290, 0x18C3);

    drawToggle(15, 144, config.secureLogging, "Encrypted Logs");
    tft.drawFastHLine(15, 171, 290, 0x18C3);

    drawToggle(15, 177, config.autoBrightness, "Auto Brightness");

    drawNavbar();
    tft.endWrite();
}

void drawInfoScreen() {
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    drawHeader("STATUS");

    tft.setTextSize(1);
    int y = 45;
    int spacing = 20;

    tft.setTextColor(0xAD55);
    tft.setCursor(15, y); tft.print("FIRMWARE");
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(130, y); tft.printf("%s", VERSION);
    y += spacing;

    tft.setTextColor(0xAD55);
    tft.setCursor(15, y); tft.print("BATTERY");
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(130, y); tft.printf("%.2f V", batteryVoltage);
    y += spacing;

    tft.setTextColor(0xAD55);
    tft.setCursor(15, y); tft.print("OUI DATABASE");
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(130, y); tft.printf("%d entries", dynamicDatabase.size());
    y += spacing;

    tft.setTextColor(0xAD55);
    tft.setCursor(15, y); tft.print("FREE MEMORY");
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(130, y); tft.printf("%d KB", ESP.getFreeHeap() / 1024);
    y += spacing;

    tft.setTextColor(0xAD55);
    tft.setCursor(15, y); tft.print("TOUCH");
    tft.setTextColor(i2cAvailable ? TFT_GREEN : TFT_RED);
    tft.setCursor(130, y); tft.print(i2cAvailable ? "OK" : "ERROR");
    y += spacing;

    tft.setTextColor(0xAD55);
    tft.setCursor(15, y); tft.print("SD CARD");
    tft.setTextColor(sdCardAvailable ? TFT_GREEN : TFT_RED);
    tft.setCursor(130, y); tft.print(sdCardAvailable ? "READY" : "NOT FOUND");
    y += spacing;

    tft.setTextColor(0xAD55);
    tft.setCursor(15, y); tft.print("UPTIME");
    tft.setTextColor(TFT_WHITE);
    unsigned long up = millis() / 1000;
    tft.setCursor(130, y); tft.printf("%02d:%02d:%02d", (int)(up/3600), (int)((up%3600)/60), (int)(up%60));

    drawNavbar();
    tft.endWrite();
}

void updateDisplay() {
    switch (currentScreen) {
        case SCREEN_WIZARD:   drawWizardScreen();   break;
        case SCREEN_MAIN:     drawMainScreen();     break;
        case SCREEN_RADAR:    drawRadarScreen();    break;
        case SCREEN_SETTINGS: drawSettingsScreen(); break;
        case SCREEN_INFO:     drawInfoScreen();     break;
    }
}

// ============================================================================
//  TOUCH HANDLER
// ============================================================================
void handleTouchGestures() {
    static unsigned long lastTouchTime = 0;
    uint16_t x, y;

    if (!readCapacitiveTouch(&x, &y)) return;

    // Debounce: ignore rapid repeated touches
    if (millis() - lastTouchTime < 250) return;
    lastTouchTime = millis();

    if (currentScreen == SCREEN_WIZARD) {
        if (x > 200 && y > 170 && y < 200) {
            wizardStep++;
            if (wizardStep > 2) {
                config.setupComplete = true;
                saveConfig();
                currentScreen = SCREEN_MAIN;
            }
        }
    } else if (y > 210) {
        // Navbar
        int idx = x / 80;
        if (idx >= 0 && idx <= 3) {
            currentScreen = (Screen)(idx + 1);
        }
    } else if (currentScreen == SCREEN_SETTINGS) {
        // Toggle hit zones
        if (x > 175 && x < 230) {
            if      (y > 45  && y < 72)  config.enableBLE      = !config.enableBLE;
            else if (y > 78  && y < 105) config.enableWiFi     = !config.enableWiFi;
            else if (y > 111 && y < 138) config.enableLogging  = !config.enableLogging;
            else if (y > 144 && y < 171) config.secureLogging  = !config.secureLogging;
            else if (y > 177 && y < 204) config.autoBrightness = !config.autoBrightness;
            saveConfig();
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
    preferences.putBool("log",    config.enableLogging);
    preferences.putBool("secure", config.secureLogging);
    preferences.putBool("autobr", config.autoBrightness);
    preferences.putInt("bright",  config.brightness);
    preferences.end();
}

void loadConfig() {
    preferences.begin("oui-spy", true);
    config.setupComplete  = preferences.getBool("setup",  false);
    config.enableBLE      = preferences.getBool("ble",    true);
    config.enableWiFi     = preferences.getBool("wifi",   true);
    config.enableLogging  = preferences.getBool("log",    true);
    config.secureLogging  = preferences.getBool("secure", false);
    config.autoBrightness = preferences.getBool("autobr", true);
    config.brightness     = preferences.getInt("bright",  255);
    preferences.end();
}

// ============================================================================
//  SETUP & LOOP
// ============================================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n[UK-OUI-SPY] Booting PRO Edition...");

    // Mutex
    xDetectionMutex = xSemaphoreCreateMutex();

    // GPIO Init
    pinMode(LED_R_PIN, OUTPUT);
    pinMode(LED_G_PIN, OUTPUT);
    pinMode(LED_B_PIN, OUTPUT);
    pinMode(TFT_BL, OUTPUT);
    pinMode(BAT_ADC, INPUT);
    digitalWrite(TFT_BL, HIGH);
    digitalWrite(LED_R_PIN, LOW);
    digitalWrite(LED_G_PIN, LOW);
    digitalWrite(LED_B_PIN, LOW);

    // I2C & Touch
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Wire.beginTransmission(TOUCH_ADDR);
    i2cAvailable = (Wire.endTransmission() == 0);
    Serial.printf("[TOUCH] CST820: %s\n", i2cAvailable ? "OK" : "NOT FOUND");

    // Display
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    Serial.println("[TFT] Display initialized.");

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
    Serial.println("[BLE] Initialized.");

    // WiFi (Station mode, disconnected for passive scanning)
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    Serial.println("[WiFi] Initialized.");

    // Battery initial read
    batteryVoltage = (analogRead(BAT_ADC) / 4095.0) * 2.0 * 3.3 * 1.1;

    // Interaction timer
    lastInteraction = millis();

    // Launch FreeRTOS tasks on separate cores
    xTaskCreatePinnedToCore(ScanTask, "ScanTask", 8192, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(UITask,   "UITask",   8192, NULL, 1, NULL, 1);

    Serial.println("[UK-OUI-SPY] Boot complete. System ready.");
}

void loop() {
    // Main loop yields to FreeRTOS tasks
    vTaskDelay(pdMS_TO_TICKS(1000));
}
