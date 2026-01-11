#include "oui_database.h"
#include <TFT_eSPI.h>

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
