#include "oui_database.h"
#include <TFT_eSPI.h>
#include <SD.h>

std::vector<OUIEntry> dynamicDatabase;
std::unordered_map<String, OUIEntry*> ouiLookup;

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
    for (auto& entry : dynamicDatabase) {
        ouiLookup[entry.oui] = &entry;
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
    dynamicDatabase.push_back({"00:12:12", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Static Fallback"});
    dynamicDatabase.push_back({"00:40:8C", "Axis", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Static Fallback"});
    dynamicDatabase.push_back({"60:60:1F", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Static Fallback"});
    rebuildLookupTable();
}
