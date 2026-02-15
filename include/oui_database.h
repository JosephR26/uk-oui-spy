#ifndef OUI_DATABASE_H
#define OUI_DATABASE_H

#include <Arduino.h>

// Device categories
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
    CAT_SMART_CITY_INFRA = 11
};

// Relevance levels
enum RelevanceLevel {
    REL_LOW = 0,
    REL_MEDIUM = 1,
    REL_HIGH = 2
};

// Typical UK deployment
enum DeploymentType {
    DEPLOY_POLICE = 0,
    DEPLOY_COUNCIL = 1,
    DEPLOY_TRANSPORT = 2,
    DEPLOY_RETAIL = 3,
    DEPLOY_PRIVATE = 4,
    DEPLOY_GOVERNMENT = 5
};

// OUI Database Entry
struct OUIEntry {
    const char* oui;              // First 3 bytes of MAC (e.g., "A4:DA:32")
    const char* manufacturer;
    DeviceCategory category;
    RelevanceLevel relevance;
    DeploymentType deployment;
    const char* notes;
};

// Static OUI database
extern const OUIEntry OUI_DATABASE[];
extern const size_t OUI_DATABASE_SIZE;

// Lookup an OUI prefix — returns nullptr if not found
const OUIEntry* findOUI(const String& oui);

// Helper functions
const char* getCategoryName(DeviceCategory cat);
const char* getRelevanceName(RelevanceLevel rel);
const char* getDeploymentName(DeploymentType dep);
uint16_t getCategoryColor(DeviceCategory cat);
uint16_t getRelevanceColor(RelevanceLevel rel);

#endif
