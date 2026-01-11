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
    CAT_DOORBELL_CAM = 8
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

// UK Surveillance Device OUI Database
// This is a curated list of known surveillance device manufacturers
const OUIEntry UK_OUI_DATABASE[] = {
    // Hikvision - Major CCTV manufacturer, widely used by UK police/councils
    {"00:12:12", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "UK Police/Council CCTV"},
    {"28:57:BE", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"44:19:B6", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Network Cameras"},
    {"BC:AD:28", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "Transport CCTV"},

    // Axis Communications - Premium surveillance, UK police/transport
    {"00:40:8C", "Axis Communications", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Body Cams/CCTV"},
    {"AC:CC:8E", "Axis Communications", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "Network Cameras"},
    {"B8:A4:4F", "Axis Communications", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Police Body Cameras"},

    // Dahua - Major CCTV, UK council/retail use
    {"00:12:16", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"08:60:6E", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Network CCTV"},
    {"6C:C2:17", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_PRIVATE, "Security Cameras"},

    // Motorola Solutions - UK Police systems (ANPR, body cams)
    {"00:0A:28", "Motorola Solutions", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "ANPR Systems"},
    {"00:23:68", "Motorola Solutions", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Police Body Cameras"},
    {"00:30:D3", "Motorola Solutions", CAT_TRAFFIC, REL_HIGH, DEPLOY_GOVERNMENT, "Traffic Systems"},

    // DJI - Drones (police, search & rescue)
    {"60:60:1F", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Police Drones"},
    {"F0:F0:1D", "DJI", CAT_DRONE, REL_MEDIUM, DEPLOY_GOVERNMENT, "Surveillance Drones"},

    // Avigilon (Motorola) - High-end surveillance, UK police
    {"00:11:C1", "Avigilon", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "HD Surveillance"},

    // Hanwha (Samsung) - Major CCTV supplier
    {"00:00:F0", "Hanwha Techwin", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Samsung CCTV"},
    {"00:09:18", "Hanwha Techwin", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "Network Cameras"},

    // Bosch Security - UK transport/government
    {"00:0E:8F", "Bosch Security", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Security Cameras"},
    {"00:1B:EE", "Bosch Security", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "ANPR/Traffic"},

    // Genetec - Software platform for ANPR/CCTV integration
    {"00:0C:E5", "Genetec", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "ANPR/Security Platform"},

    // Mobotix - High-security IP cameras
    {"00:03:C5", "Mobotix", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Decentralized Cameras"},

    // Pelco - Professional surveillance
    {"00:03:BE", "Pelco", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Surveillance Systems"},

    // Ring (Amazon) - Doorbell cameras, cloud CCTV
    {"74:C6:3B", "Ring (Amazon)", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Video Doorbells"},
    {"88:71:E5", "Ring (Amazon)", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Security Cameras"},

    // Nest (Google) - Consumer cloud cameras
    {"18:B4:30", "Nest Labs", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Cloud Cameras"},
    {"64:16:66", "Nest Labs", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Video Doorbells"},

    // Reolink - Consumer/SMB CCTV
    {"EC:71:DB", "Reolink", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "IP Cameras"},

    // Vivotek - Network cameras
    {"00:02:D1", "Vivotek", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "IP Surveillance"},

    // Panasonic - Professional security
    {"00:80:15", "Panasonic", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Security Cameras"},

    // Sony - Professional cameras
    {"00:1D:BA", "Sony", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Network Cameras"},

    // Ubiquiti - UniFi Protect cameras (growing in SMB/retail)
    {"74:83:C2", "Ubiquiti Networks", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "UniFi Cameras"},
    {"B4:FB:E4", "Ubiquiti Networks", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "UniFi Protect"},

    // Parrot - Consumer/commercial drones
    {"A0:14:3D", "Parrot", CAT_DRONE, REL_LOW, DEPLOY_PRIVATE, "Consumer Drones"},

    // GoPro - Action cameras (sometimes used as dash/body cams)
    {"28:87:BA", "GoPro", CAT_DASH_CAM, REL_LOW, DEPLOY_PRIVATE, "Action Cameras"},

    // Nextbase - UK dash cam market leader
    {"00:07:AB", "Nextbase", CAT_DASH_CAM, REL_LOW, DEPLOY_PRIVATE, "Dash Cameras"},

    // BlackVue - Premium dash cams with cloud
    {"00:11:32", "BlackVue", CAT_DASH_CAM, REL_LOW, DEPLOY_PRIVATE, "Cloud Dash Cams"},

    // Garmin - Dash cams
    {"00:0C:6E", "Garmin", CAT_DASH_CAM, REL_LOW, DEPLOY_PRIVATE, "Dash Cameras"},
};

const int UK_OUI_DATABASE_SIZE = sizeof(UK_OUI_DATABASE) / sizeof(OUIEntry);

// Helper functions
const char* getCategoryName(DeviceCategory cat);
const char* getRelevanceName(RelevanceLevel rel);
const char* getDeploymentName(DeploymentType dep);
uint16_t getCategoryColor(DeviceCategory cat);
uint16_t getRelevanceColor(RelevanceLevel rel);

#endif
