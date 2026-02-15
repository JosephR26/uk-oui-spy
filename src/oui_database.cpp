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
    // ============================================================
    // HIKVISION - Major CCTV manufacturer, widely used by UK police/councils
    // ============================================================
    dynamicDatabase.push_back({"00:12:12", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "UK Police/Council CCTV"});
    dynamicDatabase.push_back({"28:57:BE", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"});
    dynamicDatabase.push_back({"44:19:B6", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Network Cameras"});
    dynamicDatabase.push_back({"BC:AD:28", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "Transport CCTV"});
    dynamicDatabase.push_back({"54:C4:15", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "PTZ Cameras"});
    dynamicDatabase.push_back({"C4:2F:90", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Smart Cameras"});
    dynamicDatabase.push_back({"14:2D:27", "Hikvision", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "ANPR Systems"});
    dynamicDatabase.push_back({"4C:BD:8F", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Network Cameras"});
    dynamicDatabase.push_back({"68:E1:66", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP CCTV"});
    dynamicDatabase.push_back({"C0:56:E3", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "Surveillance Cameras"});
    dynamicDatabase.push_back({"D4:4B:5E", "Hikvision", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "DeepinMind ANPR"});
    dynamicDatabase.push_back({"F0:1D:BC", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Smart Cameras"});

    // ============================================================
    // AXIS COMMUNICATIONS - Premium surveillance, UK police/transport
    // ============================================================
    dynamicDatabase.push_back({"00:40:8C", "Axis Communications", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Body Cams/CCTV"});
    dynamicDatabase.push_back({"AC:CC:8E", "Axis Communications", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "Network Cameras"});
    dynamicDatabase.push_back({"B8:A4:4F", "Axis Communications", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Police Body Cameras"});
    dynamicDatabase.push_back({"00:09:2D", "Axis Communications", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "M-Series Cameras"});

    // ============================================================
    // DAHUA TECHNOLOGY - Major CCTV, UK council/retail use
    // ============================================================
    dynamicDatabase.push_back({"A4:DA:32", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Government/Council CCTV"});
    dynamicDatabase.push_back({"3C:EF:8C", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras and NVRs"});
    dynamicDatabase.push_back({"6C:C2:17", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Security Cameras (Field Validated)"});
    dynamicDatabase.push_back({"00:12:16", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"});
    dynamicDatabase.push_back({"08:60:6E", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Network CCTV"});
    dynamicDatabase.push_back({"A0:BD:1D", "Dahua", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "PTZ Cameras"});
    dynamicDatabase.push_back({"78:D8:B5", "Dahua", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Traffic Cameras"});
    dynamicDatabase.push_back({"00:26:37", "Dahua", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "IP Cameras"});
    dynamicDatabase.push_back({"2C:44:05", "Dahua", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Network Cameras"});
    dynamicDatabase.push_back({"E8:CC:18", "Dahua", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "HDCVI Cameras"});
    dynamicDatabase.push_back({"F4:83:CD", "Dahua", CAT_ANPR, REL_MEDIUM, DEPLOY_POLICE, "LPR Cameras"});

    // ============================================================
    // MOTOROLA SOLUTIONS - UK Police systems (ANPR, body cams)
    // ============================================================
    dynamicDatabase.push_back({"00:0A:28", "Motorola Solutions", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "ANPR Systems"});
    dynamicDatabase.push_back({"00:23:68", "Motorola Solutions", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Police Body Cameras"});
    dynamicDatabase.push_back({"00:30:D3", "Motorola Solutions", CAT_TRAFFIC, REL_HIGH, DEPLOY_GOVERNMENT, "Traffic Systems"});
    dynamicDatabase.push_back({"00:04:56", "Motorola", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Police Equipment"});
    dynamicDatabase.push_back({"00:90:9C", "Motorola", CAT_TRAFFIC, REL_HIGH, DEPLOY_GOVERNMENT, "Traffic Management"});
    dynamicDatabase.push_back({"00:D0:BC", "Motorola", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "Public Safety ANPR"});

    // ============================================================
    // AVIGILON (Motorola) - High-end surveillance, UK police
    // ============================================================
    dynamicDatabase.push_back({"00:11:C1", "Avigilon", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "HD Surveillance"});
    dynamicDatabase.push_back({"00:05:CA", "Avigilon", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "HD Analytics"});
    dynamicDatabase.push_back({"D8:90:E8", "Avigilon", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "LPR/ANPR"});
    dynamicDatabase.push_back({"68:EB:C5", "Avigilon", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "H5A Cameras"});
    dynamicDatabase.push_back({"E4:11:5B", "Avigilon Alta", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Cloud Access Control"});

    // ============================================================
    // DJI & DRONES - Police, search & rescue
    // ============================================================
    dynamicDatabase.push_back({"60:60:1F", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Police Drones"});
    dynamicDatabase.push_back({"F0:F0:1D", "DJI", CAT_DRONE, REL_MEDIUM, DEPLOY_GOVERNMENT, "Surveillance Drones"});
    dynamicDatabase.push_back({"AC:17:02", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Enterprise Drones"});
    dynamicDatabase.push_back({"D0:53:C4", "DJI", CAT_DRONE, REL_MEDIUM, DEPLOY_GOVERNMENT, "Matrice Series"});
    dynamicDatabase.push_back({"00:60:37", "Skydio", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Autonomous Drone Controller (Field Validated)"});
    dynamicDatabase.push_back({"90:9F:33", "Sky Drone", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Autonomous Drone Platform (Field Validated)"});
    dynamicDatabase.push_back({"A0:14:3D", "Parrot", CAT_DRONE, REL_LOW, DEPLOY_PRIVATE, "Consumer Drones"});
    dynamicDatabase.push_back({"00:25:DF", "Autel Robotics", CAT_DRONE, REL_MEDIUM, DEPLOY_GOVERNMENT, "Commercial Drones"});
    dynamicDatabase.push_back({"DC:9F:DB", "Autel Robotics", CAT_DRONE, REL_MEDIUM, DEPLOY_GOVERNMENT, "EVO Series Drones"});
    dynamicDatabase.push_back({"00:26:66", "Yuneec", CAT_DRONE, REL_MEDIUM, DEPLOY_POLICE, "H520 Police Drones"});
    dynamicDatabase.push_back({"90:3A:E6", "senseFly", CAT_DRONE, REL_LOW, DEPLOY_GOVERNMENT, "Survey Drones"});

    // ============================================================
    // BODY CAMERAS - UK Police suppliers
    // ============================================================
    dynamicDatabase.push_back({"00:1E:C0", "Digital Barriers", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Police Body Cameras"});
    dynamicDatabase.push_back({"00:26:08", "Edesix", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "UK Police Body Cams"});
    dynamicDatabase.push_back({"00:1B:C5", "Reveal Media", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Body Worn Video"});
    dynamicDatabase.push_back({"00:02:55", "Axon Enterprise", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "UK Police Body Cams"});
    dynamicDatabase.push_back({"00:18:F3", "Axon Enterprise", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Axon Body Cameras"});
    dynamicDatabase.push_back({"6C:C7:EC", "Axon Enterprise", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Axon Fleet/Body Cams"});
    dynamicDatabase.push_back({"00:0C:D4", "WatchGuard Video", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Police Dash/Body Cams"});
    dynamicDatabase.push_back({"00:21:10", "Sepura", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Police Radio/BWV"});
    dynamicDatabase.push_back({"00:0E:1D", "Zepcam", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Body Worn Cameras"});

    // ============================================================
    // HANWHA (Samsung) - Major CCTV supplier
    // ============================================================
    dynamicDatabase.push_back({"00:1A:3F", "Hanwha Techwin", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Samsung/Hanwha CCTV"});
    dynamicDatabase.push_back({"00:00:F0", "Hanwha Techwin", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Samsung CCTV"});
    dynamicDatabase.push_back({"00:09:18", "Hanwha Techwin", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "Network Cameras"});
    dynamicDatabase.push_back({"00:16:6C", "Hanwha Techwin", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Wisenet Cameras"});
    dynamicDatabase.push_back({"00:0D:F0", "Hanwha Techwin", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Wisenet Cameras"});
    dynamicDatabase.push_back({"20:13:E0", "Hanwha Techwin", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Network Cameras"});
    dynamicDatabase.push_back({"00:09:6D", "Hanwha", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "Council Cameras"});

    // ============================================================
    // BOSCH SECURITY - UK transport/government
    // ============================================================
    dynamicDatabase.push_back({"00:80:F0", "Bosch Security", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Council/Retail CCTV"});
    dynamicDatabase.push_back({"00:0E:8F", "Bosch Security", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Security Cameras"});
    dynamicDatabase.push_back({"00:1B:EE", "Bosch Security", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "ANPR/Traffic"});
    dynamicDatabase.push_back({"00:12:E0", "Bosch Security", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Autodome Cameras"});
    dynamicDatabase.push_back({"00:1A:A0", "Bosch Security", CAT_TRAFFIC, REL_HIGH, DEPLOY_POLICE, "Traffic Solutions"});

    // ============================================================
    // GENETEC - ANPR/CCTV integration platform
    // ============================================================
    dynamicDatabase.push_back({"E0:50:8B", "Genetec", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_GOVERNMENT, "Facial Recognition Systems"});
    dynamicDatabase.push_back({"00:0C:E5", "Genetec", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "ANPR/Security Platform"});
    dynamicDatabase.push_back({"00:15:C5", "Genetec", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "AutoVu ANPR"});

    // ============================================================
    // ANPR & TRAFFIC ENFORCEMENT
    // ============================================================
    dynamicDatabase.push_back({"00:03:52", "Kapsch", CAT_ANPR, REL_HIGH, DEPLOY_GOVERNMENT, "ULEZ/ANPR London"});
    dynamicDatabase.push_back({"00:21:5C", "Kapsch", CAT_TRAFFIC, REL_HIGH, DEPLOY_TRANSPORT, "Congestion Charging"});
    dynamicDatabase.push_back({"00:30:05", "SWARCO", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_TRANSPORT, "Traffic Signals/ANPR"});
    dynamicDatabase.push_back({"00:0C:A4", "Jenoptik", CAT_TRAFFIC, REL_HIGH, DEPLOY_POLICE, "Speed/ANPR Cameras"});
    dynamicDatabase.push_back({"00:07:7C", "Tattile", CAT_ANPR, REL_MEDIUM, DEPLOY_TRANSPORT, "ANPR Solutions"});
    dynamicDatabase.push_back({"00:1F:CD", "Redflex", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_GOVERNMENT, "Speed Cameras"});
    dynamicDatabase.push_back({"00:0A:E4", "Verra Mobility", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_GOVERNMENT, "Traffic Enforcement"});

    // ============================================================
    // SIEMENS - UK traffic cameras and smart city
    // ============================================================
    dynamicDatabase.push_back({"00:0E:8C", "Siemens", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_TRANSPORT, "Traffic CCTV"});
    dynamicDatabase.push_back({"00:50:7F", "Siemens", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_GOVERNMENT, "Smart City Cameras"});
    dynamicDatabase.push_back({"00:1B:1B", "Siemens", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_TRANSPORT, "Transport Systems"});

    // ============================================================
    // FACIAL RECOGNITION SYSTEMS (Cardiff/UK Police)
    // ============================================================
    // NEC Corporation - NeoFace Live used by South Wales Police (Cardiff)
    dynamicDatabase.push_back({"00:00:86", "NEC Corporation", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "NeoFace Live FR (Cardiff)"});
    dynamicDatabase.push_back({"00:00:D1", "NEC Corporation", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "NEC FR Systems"});
    dynamicDatabase.push_back({"00:40:66", "NEC Corporation", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "Police FR Servers"});
    dynamicDatabase.push_back({"00:1B:C0", "NEC Corporation", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "NEC Surveillance"});
    // Cognitec Systems - German FR vendor used in UK
    dynamicDatabase.push_back({"00:0E:3B", "Cognitec", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "FaceVACS FR System"});
    dynamicDatabase.push_back({"00:50:BA", "Cognitec", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_GOVERNMENT, "Facial Recognition"});
    // BriefCam - Video analytics for retrospective FR (UK police use)
    dynamicDatabase.push_back({"00:50:56", "BriefCam", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "Video Analytics/FR"});
    dynamicDatabase.push_back({"A4:5E:60", "BriefCam", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Forensic Video"});
    // AnyVision (now Oosto) - FR systems (UK retail/transport)
    dynamicDatabase.push_back({"00:1C:23", "AnyVision", CAT_FACIAL_RECOG, REL_MEDIUM, DEPLOY_RETAIL, "Retail FR Systems"});
    // Clearview AI - Controversial FR platform (UK police usage reported)
    dynamicDatabase.push_back({"00:1A:6B", "Clearview AI", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "FR Database"});
    // Idemia (formerly Morpho) - Biometrics/FR for UK police
    dynamicDatabase.push_back({"00:30:AB", "Idemia", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "Police Biometrics"});
    dynamicDatabase.push_back({"00:0E:2E", "Morpho", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "Legacy FR Systems"});
    // Auror - Retail crime intelligence (growing UK use)
    dynamicDatabase.push_back({"A4:83:E7", "Auror", CAT_FACIAL_RECOG, REL_MEDIUM, DEPLOY_RETAIL, "Retail Intelligence"});

    // ============================================================
    // UK COUNCIL PARKING & CIVIL ENFORCEMENT
    // ============================================================
    dynamicDatabase.push_back({"00:00:AA", "Conduent", CAT_PARKING_ENFORCEMENT, REL_MEDIUM, DEPLOY_COUNCIL, "Parking Enforcement"});
    dynamicDatabase.push_back({"00:08:02", "Conduent", CAT_ANPR, REL_MEDIUM, DEPLOY_COUNCIL, "PCN Cameras"});
    dynamicDatabase.push_back({"00:D0:B7", "Conduent", CAT_PARKING_ENFORCEMENT, REL_MEDIUM, DEPLOY_COUNCIL, "Civil Enforcement"});
    dynamicDatabase.push_back({"00:1E:58", "NSL Services", CAT_PARKING_ENFORCEMENT, REL_MEDIUM, DEPLOY_COUNCIL, "Parking Enforcement"});
    dynamicDatabase.push_back({"00:0F:EA", "APCOA", CAT_PARKING_ENFORCEMENT, REL_LOW, DEPLOY_COUNCIL, "Car Park ANPR"});
    dynamicDatabase.push_back({"00:30:48", "APCOA", CAT_PARKING_ENFORCEMENT, REL_LOW, DEPLOY_PRIVATE, "Parking Cameras"});
    dynamicDatabase.push_back({"00:1D:7E", "Euro Car Parks", CAT_PARKING_ENFORCEMENT, REL_LOW, DEPLOY_PRIVATE, "Private Parking"});
    dynamicDatabase.push_back({"00:26:5E", "ParkingEye", CAT_PARKING_ENFORCEMENT, REL_LOW, DEPLOY_PRIVATE, "ANPR Parking"});

    // ============================================================
    // PROFESSIONAL CCTV & VMS MANUFACTURERS
    // ============================================================
    dynamicDatabase.push_back({"00:50:C2", "Milestone Systems", CAT_CCTV, REL_HIGH, DEPLOY_GOVERNMENT, "VMS Infrastructure"});
    dynamicDatabase.push_back({"00:0C:C8", "Milestone Systems", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Police VMS"});
    dynamicDatabase.push_back({"00:18:7D", "Pelco (Motorola)", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "Police/Transport CCTV"});
    dynamicDatabase.push_back({"00:03:BE", "Pelco", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Surveillance Systems"});
    dynamicDatabase.push_back({"00:03:C5", "Mobotix", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Decentralized Cameras"});
    dynamicDatabase.push_back({"00:02:D1", "Vivotek", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "IP Surveillance"});
    dynamicDatabase.push_back({"00:13:FE", "IndigoVision", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "IP CCTV"});
    dynamicDatabase.push_back({"00:11:5B", "Dallmeier", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Panomera Cameras"});
    dynamicDatabase.push_back({"00:11:98", "ACTi", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "IP Cameras"});
    dynamicDatabase.push_back({"00:40:FA", "Exacq Technologies", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Video Management"});
    dynamicDatabase.push_back({"00:19:70", "Salient Systems", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "CompleteView VMS"});
    dynamicDatabase.push_back({"00:03:C0", "March Networks", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Transit Surveillance"});
    dynamicDatabase.push_back({"00:02:A2", "Dedicated Micros", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "UK DVR/NVR"});
    dynamicDatabase.push_back({"00:0D:8B", "360 Vision", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "UK PTZ Cameras"});
    dynamicDatabase.push_back({"00:1D:09", "Videcon", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "UK CCTV Systems"});
    dynamicDatabase.push_back({"00:0E:C6", "Wavestore", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "UK Video Management"});
    dynamicDatabase.push_back({"00:1B:67", "Qognify", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Ocularis VMS"});
    dynamicDatabase.push_back({"00:12:CF", "Tyco Security", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "victor/Illustra"});
    dynamicDatabase.push_back({"00:0D:20", "Interlogix", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "TruVision Cameras"});
    dynamicDatabase.push_back({"B4:A3:82", "Uniview", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "IPC Cameras"});
    dynamicDatabase.push_back({"00:1F:AF", "Tiandy", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "IP Cameras"});
    dynamicDatabase.push_back({"00:1E:8C", "CP Plus", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "Budget CCTV"});
    dynamicDatabase.push_back({"00:1B:63", "LTS Security", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Platinum Series"});
    dynamicDatabase.push_back({"00:11:D9", "Digital Watchdog", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "DW Spectrum"});
    dynamicDatabase.push_back({"00:1C:14", "Razberi", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "ServerSwitch"});

    // ============================================================
    // PANASONIC / SONY / CANON - Professional cameras
    // ============================================================
    dynamicDatabase.push_back({"00:80:15", "Panasonic", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Security Cameras"});
    dynamicDatabase.push_back({"00:0D:C1", "Panasonic", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "i-PRO Cameras"});
    dynamicDatabase.push_back({"00:80:64", "Panasonic", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "WV Series Cameras"});
    dynamicDatabase.push_back({"00:1D:BA", "Sony", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Network Cameras"});
    dynamicDatabase.push_back({"08:00:46", "Sony", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "IP Cameras"});
    dynamicDatabase.push_back({"50:EB:1A", "Sony", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Network Cameras"});
    dynamicDatabase.push_back({"00:00:85", "Canon", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Network Cameras"});
    dynamicDatabase.push_back({"00:04:A9", "Canon", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "VB Series Cameras"});

    // ============================================================
    // HONEYWELL - Security systems
    // ============================================================
    dynamicDatabase.push_back({"00:15:7D", "Honeywell", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Security Systems"});
    dynamicDatabase.push_back({"00:E0:4C", "Honeywell", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Building Security"});
    dynamicDatabase.push_back({"00:06:2A", "Honeywell", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "equIP Cameras"});
    dynamicDatabase.push_back({"00:D0:06", "Honeywell", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Performance Series"});

    // ============================================================
    // THALES - UK government/transport security
    // ============================================================
    dynamicDatabase.push_back({"00:06:0D", "Thales", CAT_CCTV, REL_HIGH, DEPLOY_GOVERNMENT, "Government Security"});
    dynamicDatabase.push_back({"00:E0:63", "Thales", CAT_TRAFFIC, REL_HIGH, DEPLOY_TRANSPORT, "Transport Security"});

    // ============================================================
    // THERMAL CAMERAS - FLIR Systems
    // ============================================================
    dynamicDatabase.push_back({"00:0D:66", "FLIR Systems", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Thermal Cameras"});
    dynamicDatabase.push_back({"00:40:D0", "FLIR Systems", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Thermal Cameras"});
    dynamicDatabase.push_back({"00:05:07", "FLIR Systems", CAT_CCTV, REL_HIGH, DEPLOY_GOVERNMENT, "Security Thermal"});

    // ============================================================
    // CLOUD-MANAGED CAMERAS - Cisco Meraki / Verkada / Eagle Eye
    // ============================================================
    dynamicDatabase.push_back({"00:1C:B3", "Cisco Meraki", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Cloud Managed Cameras"});
    dynamicDatabase.push_back({"00:18:0A", "Cisco Meraki", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Cloud Cameras"});
    dynamicDatabase.push_back({"AC:17:C8", "Cisco Meraki", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "MV Cameras"});
    dynamicDatabase.push_back({"E0:55:3D", "Cisco Meraki", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Smart Cameras"});
    dynamicDatabase.push_back({"E0:1F:88", "Verkada", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Cloud CCTV"});
    dynamicDatabase.push_back({"88:DC:96", "Verkada", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Hybrid Cloud Cameras"});
    dynamicDatabase.push_back({"00:0C:84", "Eagle Eye Networks", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_RETAIL, "Cloud VMS"});
    dynamicDatabase.push_back({"9C:4E:36", "Rhombus Systems", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_RETAIL, "Cloud Cameras"});

    // ============================================================
    // UBIQUITI - UniFi Protect cameras
    // ============================================================
    dynamicDatabase.push_back({"B8:69:F4", "Ubiquiti Networks", CAT_CCTV, REL_HIGH, DEPLOY_PRIVATE, "UniFi Cameras (Field Validated)"});
    dynamicDatabase.push_back({"18:E8:29", "Ubiquiti Networks", CAT_CCTV, REL_HIGH, DEPLOY_PRIVATE, "UniFi Cameras (Field Validated)"});
    dynamicDatabase.push_back({"74:83:C2", "Ubiquiti Networks", CAT_CCTV, REL_HIGH, DEPLOY_PRIVATE, "UniFi Cameras (Field Validated)"});
    dynamicDatabase.push_back({"B4:FB:E4", "Ubiquiti Networks", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "UniFi Protect"});
    dynamicDatabase.push_back({"24:5A:4C", "Ubiquiti Networks", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "G3/G4 Cameras"});
    dynamicDatabase.push_back({"FC:EC:DA", "Ubiquiti Networks", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "UniFi Protect"});

    // ============================================================
    // UK SECURITY PROVIDERS - ADT, Securitas, Blue Security
    // ============================================================
    dynamicDatabase.push_back({"00:19:5B", "Securitas", CAT_CCTV, REL_MEDIUM, DEPLOY_PRIVATE, "Verisure CCTV"});
    dynamicDatabase.push_back({"00:13:02", "ADT Security", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Monitored CCTV"});
    dynamicDatabase.push_back({"00:1E:37", "Blue Security", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "UK CCTV Installer"});

    // ============================================================
    // CARDIFF & SOUTH WALES INFRASTRUCTURE
    // ============================================================
    dynamicDatabase.push_back({"00:0B:82", "Telent", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "Council CCTV Network"});
    dynamicDatabase.push_back({"00:30:65", "Telent", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_TRANSPORT, "Traffic CCTV"});
    dynamicDatabase.push_back({"00:40:5A", "Vicon Industries", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "Professional CCTV"});
    dynamicDatabase.push_back({"00:0C:76", "Vicon Industries", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "VAX VMS"});
    dynamicDatabase.push_back({"00:0E:D7", "Oncam", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "360 Cameras"});
    dynamicDatabase.push_back({"00:19:3E", "Oncam", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Grandeye Cameras"});
    dynamicDatabase.push_back({"00:1C:C4", "BCDVideo", CAT_CCTV, REL_MEDIUM, DEPLOY_POLICE, "Surveillance Servers"});
    dynamicDatabase.push_back({"00:12:FB", "Sunell", CAT_CCTV, REL_LOW, DEPLOY_COUNCIL, "IP Cameras"});
    dynamicDatabase.push_back({"00:12:1E", "Geovision", CAT_CCTV, REL_LOW, DEPLOY_COUNCIL, "Council CCTV"});
    dynamicDatabase.push_back({"00:1B:21", "Jacobs Engineering", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_COUNCIL, "Smart City CCTV"});
    dynamicDatabase.push_back({"00:0A:95", "Amey", CAT_CCTV, REL_LOW, DEPLOY_COUNCIL, "CCTV Infrastructure"});

    // ============================================================
    // DASH CAMS
    // ============================================================
    dynamicDatabase.push_back({"D8:60:CF", "Smart Dashcam", CAT_DASH_CAM, REL_MEDIUM, DEPLOY_PRIVATE, "Delivery Fleet/Bodycam (Field Validated)"});
    dynamicDatabase.push_back({"28:87:BA", "GoPro", CAT_DASH_CAM, REL_MEDIUM, DEPLOY_PRIVATE, "Action Cameras (Field Validated)"});
    dynamicDatabase.push_back({"00:07:AB", "Nextbase", CAT_DASH_CAM, REL_LOW, DEPLOY_PRIVATE, "Dash Cameras"});
    dynamicDatabase.push_back({"00:11:32", "BlackVue", CAT_DASH_CAM, REL_LOW, DEPLOY_PRIVATE, "Cloud Dash Cams"});
    dynamicDatabase.push_back({"00:0C:6E", "Garmin", CAT_DASH_CAM, REL_LOW, DEPLOY_PRIVATE, "Dash Cameras"});
    dynamicDatabase.push_back({"00:37:6D", "Thinkware", CAT_DASH_CAM, REL_LOW, DEPLOY_PRIVATE, "Dash Cameras"});
    dynamicDatabase.push_back({"00:26:5A", "Viofo", CAT_DASH_CAM, REL_LOW, DEPLOY_PRIVATE, "Dash Cameras"});

    // ============================================================
    // CONSUMER DOORBELLS & CLOUD CAMERAS
    // ============================================================
    dynamicDatabase.push_back({"74:C6:3B", "Ring (Amazon)", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Video Doorbells"});
    dynamicDatabase.push_back({"EC:71:DB", "Ring (Amazon)", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Video Doorbells"});
    dynamicDatabase.push_back({"88:71:E5", "Ring (Amazon)", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Security Cameras"});
    dynamicDatabase.push_back({"B0:4E:26", "Ring", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Video Doorbell Pro"});
    dynamicDatabase.push_back({"FC:92:8F", "Ring", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Stick Up Cam"});
    dynamicDatabase.push_back({"18:B4:30", "Nest (Google)", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Cloud Cameras"});
    dynamicDatabase.push_back({"64:16:66", "Nest Labs", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Video Doorbells"});
    dynamicDatabase.push_back({"1C:3E:84", "Google Nest", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Hello Doorbell"});
    dynamicDatabase.push_back({"F0:EF:86", "Google Nest", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Nest Cam IQ"});
    dynamicDatabase.push_back({"D0:73:D5", "Arlo Technologies", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Wireless Cameras"});
    dynamicDatabase.push_back({"2C:AA:8E", "Wyze Labs", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Smart Cameras"});
    dynamicDatabase.push_back({"T8:1D:7F", "Anker", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Eufy Cameras"});
    dynamicDatabase.push_back({"34:EF:B6", "Anker", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Eufy Video Doorbell"});
    dynamicDatabase.push_back({"24:0A:C4", "Anker", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Eufy Cameras"});
    dynamicDatabase.push_back({"A0:02:DC", "Amazon", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Blink Cameras"});
    dynamicDatabase.push_back({"F0:D5:BF", "Yale", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Smart Locks"});
    dynamicDatabase.push_back({"50:C7:BF", "TP-Link", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Tapo Cameras (Field Validated)"});
    dynamicDatabase.push_back({"84:D8:1B", "TP-Link", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Kasa Cameras"});
    dynamicDatabase.push_back({"34:CE:00", "Xiaomi", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Mi Cameras"});
    dynamicDatabase.push_back({"78:11:DC", "Xiaomi", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Aqara Cameras"});
    dynamicDatabase.push_back({"38:D2:CA", "Imou", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Dahua Consumer"});
    dynamicDatabase.push_back({"00:62:6E", "Amcrest", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "Dahua OEM"});
    dynamicDatabase.push_back({"9C:8E:CD", "Amcrest", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Cloud Cameras"});
    dynamicDatabase.push_back({"B0:A7:B9", "Reolink", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_PRIVATE, "WiFi Cameras (Field Validated)"});
    dynamicDatabase.push_back({"00:03:7F", "Reolink", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "RLC Series"});
    dynamicDatabase.push_back({"00:06:D2", "Geovision", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "DVR/NVR Systems"});
    dynamicDatabase.push_back({"00:21:23", "Lorex", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "Home Security"});
    dynamicDatabase.push_back({"00:11:8B", "Swann", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "DIY CCTV"});

    // ============================================================
    // SMART CITY INFRASTRUCTURE (Cardiff Field-tested)
    // ============================================================
    dynamicDatabase.push_back({"38:AB:41", "Texas Instruments", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "Cardiff Smart Poles (Field Validated)"});
    dynamicDatabase.push_back({"AC:64:CF", "Fn-Link Technology", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "Cardiff Pole Network (Field Validated)"});
    dynamicDatabase.push_back({"00:12:4B", "Texas Instruments", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "IoT Wireless Module"});
    dynamicDatabase.push_back({"B0:B4:48", "Texas Instruments", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "CC2640 BLE Module"});
    dynamicDatabase.push_back({"00:80:98", "TDK Corporation", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "Cardiff Pole Sensors"});
    dynamicDatabase.push_back({"00:1D:94", "TDK Corporation", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "Industrial IoT"});
    dynamicDatabase.push_back({"00:16:A4", "Ezurio", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "Cardiff Smart Infrastructure"});
    dynamicDatabase.push_back({"24:6F:28", "Espressif", CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "ESP32 IoT Module"});
    dynamicDatabase.push_back({"30:AE:A4", "Espressif", CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "ESP32 WiFi/BLE"});
    dynamicDatabase.push_back({"00:18:DA", "u-blox", CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "Industrial IoT Module"});

    // ============================================================
    // ADDITIONAL UK-RELEVANT SURVEILLANCE OUIs
    // ============================================================
    dynamicDatabase.push_back({"E8:AB:FA", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "ColorVu Cameras"});
    dynamicDatabase.push_back({"48:0F:CF", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "AcuSense Cameras"});
    dynamicDatabase.push_back({"40:F4:FD", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "WizSense Cameras"});
    dynamicDatabase.push_back({"34:4B:50", "Dahua Technology", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "TiOC Cameras"});
    dynamicDatabase.push_back({"00:04:E2", "SMC Networks", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_TRANSPORT, "Traffic Infrastructure"});
    dynamicDatabase.push_back({"DC:54:D7", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Mini Series Drones"});
    dynamicDatabase.push_back({"00:1A:07", "i-PRO (Panasonic)", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "X-Series Cameras"});
    dynamicDatabase.push_back({"B0:B2:DC", "Zyxel", CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "Council Network Infra"});

    // ============================================================
    // CONSUMER BASELINE (filtered by default)
    // ============================================================
    dynamicDatabase.push_back({"74:DA:88", "Sky CPE", CAT_UNKNOWN, REL_LOW, DEPLOY_PRIVATE, "Consumer Broadband (Baseline)"});
    dynamicDatabase.push_back({"FC:F8:AE", "BT/EE Hub", CAT_UNKNOWN, REL_LOW, DEPLOY_PRIVATE, "Consumer Broadband (Baseline)"});
    dynamicDatabase.push_back({"20:8B:FB", "TP-Link", CAT_UNKNOWN, REL_LOW, DEPLOY_PRIVATE, "Consumer Networking (Baseline)"});

    rebuildLookupTable();
}
