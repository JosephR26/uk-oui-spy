#include "oui_database.h"
#include <TFT_eSPI.h>
#include <SD.h>

std::vector<OUIEntry> dynamicDatabase;

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
        int c1 = line.indexOf(',');
        int c2 = line.indexOf(',', c1 + 1);
        int c3 = line.indexOf(',', c2 + 1);
        int c4 = line.indexOf(',', c3 + 1);
        int c5 = line.indexOf(',', c4 + 1);

        if (c5 == -1) continue;

        OUIEntry entry;
        entry.oui = line.substring(0, c1);
        entry.manufacturer = line.substring(c1 + 1, c2);
        entry.category = (DeviceCategory)line.substring(c2 + 1, c3).toInt();
        entry.relevance = (RelevanceLevel)line.substring(c3 + 1, c4).toInt();
        entry.deployment = (DeploymentType)line.substring(c4 + 1, c5).toInt();
        entry.notes = line.substring(c5 + 1);
        
        dynamicDatabase.push_back(entry);
    }
    file.close();
    return !dynamicDatabase.empty();
}

void initializeStaticDatabase() {
    // Fallback static entries if SD fails
    dynamicDatabase.push_back({"00:12:12", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Static Fallback"});
    dynamicDatabase.push_back({"00:40:8C", "Axis", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Static Fallback"});
    dynamicDatabase.push_back({"60:60:1F", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Static Fallback"});
}
