#include "oui_database.h"
#include <TFT_eSPI.h>
#include <SD.h>

std::vector<OUIEntry> dynamicDatabase;
std::unordered_map<std::string, OUIEntry*> ouiLookup;

const char* getCategoryName(DeviceCategory cat) {
    switch(cat) {
        case CAT_CCTV: return "CCTV";
        case CAT_ANPR: return "ANPR";
        case CAT_DRONE: return "Drone";
        case CAT_BODYCAM: return "Body Cam";
        case CAT_CLOUD_CCTV: return "Cloud CCTV";
        case CAT_TRAFFIC: return "Traffic";
        case CAT_DASH_CAM: return "Dash Cam";
        case CAT_DOORBELL_CAM: return "Doorbell";
        case CAT_FACIAL_RECOG: return "Face Recog";
        case CAT_PARKING_ENFORCEMENT: return "Parking";
        case CAT_SMART_CITY_INFRA: return "Smart Pole";
        default: return "Unknown";
    }
}

const char* getRelevanceName(RelevanceLevel rel) {
    switch(rel) {
        case REL_HIGH: return "HIGH";
        case REL_MEDIUM: return "MEDIUM";
        case REL_LOW: return "LOW";
        default: return "UNKNOWN";
    }
}

const char* getDeploymentName(DeploymentType dep) {
    switch(dep) {
        case DEPLOY_POLICE: return "Police";
        case DEPLOY_COUNCIL: return "Council";
        case DEPLOY_TRANSPORT: return "Transport";
        case DEPLOY_RETAIL: return "Retail";
        case DEPLOY_PRIVATE: return "Private";
        case DEPLOY_GOVERNMENT: return "Government";
        default: return "Unknown";
    }
}

uint16_t getCategoryColor(DeviceCategory cat) {
    switch(cat) {
        case CAT_CCTV: return TFT_RED;
        case CAT_ANPR: return TFT_ORANGE;
        case CAT_DRONE: return TFT_MAGENTA;
        case CAT_BODYCAM: return TFT_RED;
        case CAT_CLOUD_CCTV: return TFT_BLUE;
        case CAT_TRAFFIC: return TFT_ORANGE;
        case CAT_DASH_CAM: return TFT_CYAN;
        case CAT_DOORBELL_CAM: return TFT_CYAN;
        case CAT_FACIAL_RECOG: return TFT_PURPLE;
        case CAT_PARKING_ENFORCEMENT: return TFT_YELLOW;
        case CAT_SMART_CITY_INFRA: return TFT_YELLOW;
        default: return TFT_WHITE;
    }
}

uint16_t getRelevanceColor(RelevanceLevel rel) {
    switch(rel) {
        case REL_HIGH: return TFT_RED;
        case REL_MEDIUM: return TFT_YELLOW;
        case REL_LOW: return TFT_GREEN;
        default: return TFT_WHITE;
    }
}

void rebuildLookupTable() {
    ouiLookup.clear();
    ouiLookup.reserve(dynamicDatabase.size());
    for (auto& entry : dynamicDatabase) {
        ouiLookup[toOuiKey(entry.oui)] = &entry;
    }
}

bool loadOUIDatabaseFromSD(const char* path) {
    if (!SD.exists(path)) return false;
    File file = SD.open(path);
    if (!file) return false;

    dynamicDatabase.clear();
    if (file.available()) file.readStringUntil('\n'); // Skip header

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
            Serial.print("OUI DB: skipping malformed line: ");
            Serial.println(line);
            continue;
        }

        int catVal = fields[2].toInt();
        int relVal = fields[3].toInt();
        int depVal = fields[4].toInt();

        if (catVal < 0 || catVal > CAT_MAX || relVal < 0 || relVal > REL_MAX || depVal < 0 || depVal > DEPLOY_MAX) {
            Serial.print("OUI DB: invalid enum values in line: ");
            Serial.println(line);
            continue;
        }

        OUIEntry entry;
        entry.oui = fields[0];
        entry.manufacturer = fields[1];
        entry.category = (DeviceCategory)catVal;
        entry.relevance = (RelevanceLevel)relVal;
        entry.deployment = (DeploymentType)depVal;
        entry.notes = fields[5];
        
        dynamicDatabase.push_back(entry);
    }
    file.close();
    
    if (!dynamicDatabase.empty()) {
        rebuildLookupTable();
        return true;
    }
    return false;
}

void initializeStaticDatabase() {
    dynamicDatabase.clear();
    // Government / Council CCTV (HIGH)
    dynamicDatabase.push_back({"00:12:12", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "UK Police/Council CCTV"});
    dynamicDatabase.push_back({"00:40:8C", "Axis Communications", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Body Cams/CCTV"});
    dynamicDatabase.push_back({"A4:DA:32", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Government/Council CCTV"});
    dynamicDatabase.push_back({"3C:EF:8C", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras and NVRs"});
    dynamicDatabase.push_back({"6C:C2:17", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Security Cameras (Field Validated)"});
    dynamicDatabase.push_back({"00:18:7D", "Pelco (Motorola)", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "Police/Transport CCTV"});
    dynamicDatabase.push_back({"00:80:F0", "Bosch Security", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Council/Retail CCTV"});
    dynamicDatabase.push_back({"00:1A:3F", "Hanwha Techwin", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Samsung/Hanwha CCTV"});
    dynamicDatabase.push_back({"00:50:C2", "Milestone Systems", CAT_CCTV, REL_HIGH, DEPLOY_GOVERNMENT, "VMS Infrastructure"});
    dynamicDatabase.push_back({"E0:50:8B", "Genetec", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_GOVERNMENT, "Facial Recognition Systems"});

    // Drones (HIGH)
    dynamicDatabase.push_back({"60:60:1F", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Police Drones"});
    dynamicDatabase.push_back({"00:60:37", "Skydio", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Autonomous Drone Controller (Field Validated)"});
    dynamicDatabase.push_back({"90:9F:33", "Sky Drone", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Autonomous Drone Platform (Field Validated)"});

    // Surveillance Infrastructure (HIGH)
    dynamicDatabase.push_back({"B8:69:F4", "Ubiquiti Networks", CAT_CCTV, REL_HIGH, DEPLOY_PRIVATE, "UniFi Cameras (Field Validated)"});
    dynamicDatabase.push_back({"18:E8:29", "Ubiquiti Networks", CAT_CCTV, REL_HIGH, DEPLOY_PRIVATE, "UniFi Cameras (Field Validated)"});
    dynamicDatabase.push_back({"74:83:C2", "Ubiquiti Networks", CAT_CCTV, REL_HIGH, DEPLOY_PRIVATE, "UniFi Cameras (Field Validated)"});
    dynamicDatabase.push_back({"00:1C:B3", "Cisco Meraki", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Cloud Managed Cameras"});
    dynamicDatabase.push_back({"B0:A7:B9", "Reolink", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_PRIVATE, "WiFi Cameras (Field Validated)"});

    // ANPR (HIGH)
    dynamicDatabase.push_back({"00:0A:28", "Motorola Solutions", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "ANPR Systems"});

    // Body Cameras (HIGH)
    dynamicDatabase.push_back({"00:1E:C0", "Digital Barriers", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Police Body Cameras"});

    // Thermal (HIGH)
    dynamicDatabase.push_back({"00:0D:66", "FLIR Systems", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Thermal Cameras"});

    // Vehicle / Dash Cams (MEDIUM)
    dynamicDatabase.push_back({"D8:60:CF", "Smart Dashcam", CAT_DASH_CAM, REL_MEDIUM, DEPLOY_PRIVATE, "Delivery Fleet/Bodycam (Field Validated)"});
    dynamicDatabase.push_back({"28:87:BA", "GoPro", CAT_DASH_CAM, REL_MEDIUM, DEPLOY_PRIVATE, "Action Cameras (Field Validated)"});

    // Consumer Doorbells (LOW)
    dynamicDatabase.push_back({"74:C6:3B", "Ring (Amazon)", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Video Doorbells"});
    dynamicDatabase.push_back({"EC:71:DB", "Ring (Amazon)", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Video Doorbells"});
    dynamicDatabase.push_back({"18:B4:30", "Nest (Google)", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Cloud Cameras"});

    // Smart City Infrastructure (MEDIUM)
    dynamicDatabase.push_back({"38:AB:41", "Texas Instruments", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "Cardiff Smart Poles (Field Validated)"});
    dynamicDatabase.push_back({"AC:64:CF", "Fn-Link Technology", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "Cardiff Pole Network (Field Validated)"});

    // Consumer / Cloud CCTV (LOW)
    dynamicDatabase.push_back({"50:C7:BF", "TP-Link", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Tapo Cameras (Field Validated)"});

    // Consumer Baseline (filtered by default)
    dynamicDatabase.push_back({"74:DA:88", "Sky CPE", CAT_UNKNOWN, REL_LOW, DEPLOY_PRIVATE, "Consumer Broadband (Baseline)"});
    dynamicDatabase.push_back({"FC:F8:AE", "BT/EE Hub", CAT_UNKNOWN, REL_LOW, DEPLOY_PRIVATE, "Consumer Broadband (Baseline)"});
    dynamicDatabase.push_back({"20:8B:FB", "TP-Link", CAT_UNKNOWN, REL_LOW, DEPLOY_PRIVATE, "Consumer Networking (Baseline)"});

    rebuildLookupTable();
}
