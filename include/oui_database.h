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
    CAT_FACIAL_RECOG = 9,      // Facial recognition systems
    CAT_PARKING_ENFORCEMENT = 10,  // Council parking/civil enforcement
    CAT_SMART_CITY_INFRA = 11  // Smart city infrastructure (networked poles/sensors)
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

    // Additional UK-relevant manufacturers

    // Milestone Systems - VMS (Video Management Software) used by UK police
    {"00:0C:C8", "Milestone Systems", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Police VMS"},

    // Verkada - Cloud-managed security cameras
    {"E0:1F:88", "Verkada", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Cloud CCTV"},

    // Honeywell - Security systems
    {"00:15:7D", "Honeywell", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Security Systems"},
    {"00:E0:4C", "Honeywell", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Building Security"},

    // Canon - Professional surveillance cameras
    {"00:00:85", "Canon", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Network Cameras"},

    // Arlo (Netgear) - Consumer wireless cameras
    {"D0:73:D5", "Arlo Technologies", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Wireless Cameras"},

    // Wyze - Budget consumer cameras
    {"2C:AA:8E", "Wyze Labs", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Smart Cameras"},

    // Eufy (Anker) - Consumer security cameras
    {"T8:1D:7F", "Anker", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Eufy Cameras"},

    // Blink (Amazon) - Consumer cameras
    {"A0:02:DC", "Amazon", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Blink Cameras"},

    // Digital Barriers - Specialist surveillance (UK police)
    {"00:1E:C0", "Digital Barriers", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Police Body Cameras"},

    // Edesix - UK body worn camera manufacturer
    {"00:26:08", "Edesix", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "UK Police Body Cams"},

    // Reveal Media - Body cameras (UK police/NHS)
    {"00:1B:C5", "Reveal Media", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Body Worn Video"},

    // Autel Robotics - Drones (commercial/police)
    {"00:25:DF", "Autel Robotics", CAT_DRONE, REL_MEDIUM, DEPLOY_GOVERNMENT, "Commercial Drones"},

    // Skydio - Advanced autonomous drones
    {"00:60:37", "Skydio", CAT_DRONE, REL_MEDIUM, DEPLOY_POLICE, "Autonomous Drones"},

    // FLIR (Teledyne) - Thermal imaging (police/search & rescue)
    {"00:0D:66", "FLIR Systems", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Thermal Cameras"},

    // IndigoVision - UK-based IP CCTV
    {"00:13:FE", "IndigoVision", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "IP CCTV"},

    // Avigilon (additional OUIs)
    {"00:05:CA", "Avigilon", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "HD Analytics"},
    {"D8:90:E8", "Avigilon", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "LPR/ANPR"},

    // Hikvision (additional OUIs)
    {"54:C4:15", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "PTZ Cameras"},
    {"C4:2F:90", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Smart Cameras"},
    {"14:2D:27", "Hikvision", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "ANPR Systems"},

    // Dahua (additional OUIs)
    {"A0:BD:1D", "Dahua", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "PTZ Cameras"},
    {"78:D8:B5", "Dahua", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Traffic Cameras"},

    // Hanwha (additional OUIs)
    {"00:16:6C", "Hanwha Techwin", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Wisenet Cameras"},

    // Geovision - Taiwan manufacturer (UK market)
    {"00:06:D2", "Geovision", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "DVR/NVR Systems"},

    // Lorex - Consumer CCTV (UK market)
    {"00:21:23", "Lorex", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "Home Security"},

    // Swann - Consumer CCTV (popular in UK)
    {"00:11:8B", "Swann", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "DIY CCTV"},

    // Yale - Smart locks with cameras
    {"F0:D5:BF", "Yale", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Smart Locks"},

    // Verisure (Securitas Direct) - UK alarm/CCTV provider
    {"00:19:5B", "Securitas", CAT_CCTV, REL_MEDIUM, DEPLOY_PRIVATE, "Verisure CCTV"},

    // ADT - Security provider (UK market)
    {"00:13:02", "ADT Security", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Monitored CCTV"},

    // Thinkware - Dash cams
    {"00:37:6D", "Thinkware", CAT_DASH_CAM, REL_LOW, DEPLOY_PRIVATE, "Dash Cameras"},

    // Viofo - Dash cams
    {"00:26:5A", "Viofo", CAT_DASH_CAM, REL_LOW, DEPLOY_PRIVATE, "Dash Cameras"},

    // ============================================================
    // ADDITIONAL UK-SPECIFIC SURVEILLANCE MANUFACTURERS
    // ============================================================

    // Axon (Taser International) - Major UK police body camera supplier
    {"00:02:55", "Axon Enterprise", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "UK Police Body Cams"},
    {"00:18:F3", "Axon Enterprise", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Axon Body Cameras"},
    {"6C:C7:EC", "Axon Enterprise", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Axon Fleet/Body Cams"},

    // WatchGuard (Motorola subsidiary) - UK police vehicle/body cams
    {"00:0C:D4", "WatchGuard Video", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Police Dash/Body Cams"},

    // Sepura (Hytera) - UK police communications/BWV
    {"00:21:10", "Sepura", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Police Radio/BWV"},

    // Zepcam (Netherlands, used by UK police/NHS)
    {"00:0E:1D", "Zepcam", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Body Worn Cameras"},

    // Motorola (additional OUIs for TETRA/cameras)
    {"00:04:56", "Motorola", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Police Equipment"},
    {"00:90:9C", "Motorola", CAT_TRAFFIC, REL_HIGH, DEPLOY_GOVERNMENT, "Traffic Management"},
    {"00:D0:BC", "Motorola", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "Public Safety ANPR"},

    // Siemens - UK traffic cameras and smart city infrastructure
    {"00:0E:8C", "Siemens", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_TRANSPORT, "Traffic CCTV"},
    {"00:50:7F", "Siemens", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_GOVERNMENT, "Smart City Cameras"},
    {"00:1B:1B", "Siemens", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_TRANSPORT, "Transport Systems"},

    // Kapsch TrafficCom - ANPR/congestion charging (London ULEZ)
    {"00:03:52", "Kapsch", CAT_ANPR, REL_HIGH, DEPLOY_GOVERNMENT, "ULEZ/ANPR London"},
    {"00:21:5C", "Kapsch", CAT_TRAFFIC, REL_HIGH, DEPLOY_TRANSPORT, "Congestion Charging"},

    // SWARCO - UK traffic management and ANPR
    {"00:30:05", "SWARCO", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_TRANSPORT, "Traffic Signals/ANPR"},

    // Jenoptik - Speed/ANPR cameras (UK speed enforcement)
    {"00:0C:A4", "Jenoptik", CAT_TRAFFIC, REL_HIGH, DEPLOY_POLICE, "Speed/ANPR Cameras"},

    // Tattile - ANPR cameras (used in UK)
    {"00:07:7C", "Tattile", CAT_ANPR, REL_MEDIUM, DEPLOY_TRANSPORT, "ANPR Solutions"},

    // NDI Recognition Systems - ANPR specialists
    {"00:50:C2", "NDI Recognition", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "ANPR Systems"},

    // Redflex - Traffic enforcement cameras
    {"00:1F:CD", "Redflex", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_GOVERNMENT, "Speed Cameras"},

    // Verra Mobility (Redflex/ATS) - UK traffic enforcement
    {"00:0A:E4", "Verra Mobility", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_GOVERNMENT, "Traffic Enforcement"},

    // Additional Hikvision OUIs (more variants detected in UK)
    {"4C:BD:8F", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Network Cameras"},
    {"68:E1:66", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP CCTV"},
    {"C0:56:E3", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "Surveillance Cameras"},
    {"D4:4B:5E", "Hikvision", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "DeepinMind ANPR"},
    {"F0:1D:BC", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Smart Cameras"},

    // Additional Dahua OUIs
    {"00:26:37", "Dahua", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "IP Cameras"},
    {"2C:44:05", "Dahua", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Network Cameras"},
    {"E8:CC:18", "Dahua", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "HDCVI Cameras"},
    {"F4:83:CD", "Dahua", CAT_ANPR, REL_MEDIUM, DEPLOY_POLICE, "LPR Cameras"},

    // Exacq (Tyco/JCI) - VMS widely used in UK
    {"00:40:FA", "Exacq Technologies", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Video Management"},

    // Salient Systems - VMS (UK installations)
    {"00:19:70", "Salient Systems", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "CompleteView VMS"},

    // March Networks - Transport CCTV (buses/trains)
    {"00:03:C0", "March Networks", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Transit Surveillance"},

    // Dedicated Micros (AD Group) - UK CCTV manufacturer
    {"00:02:A2", "Dedicated Micros", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "UK DVR/NVR"},

    // 360 Vision Technology - UK PTZ camera manufacturer
    {"00:0D:8B", "360 Vision", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "UK PTZ Cameras"},

    // Videcon - UK CCTV installer/integrator
    {"00:1D:09", "Videcon", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "UK CCTV Systems"},

    // Dallmeier - High-security CCTV (UK government)
    {"00:11:5B", "Dallmeier", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Panomera Cameras"},

    // Arecont Vision (Costar) - Megapixel cameras
    {"00:40:8C", "Arecont Vision", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Megapixel Cameras"},

    // ACTi - Taiwan manufacturer (UK market presence)
    {"00:11:98", "ACTi", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "IP Cameras"},

    // Avigilon Alta (Openpath) - Cloud access control with cameras
    {"E4:11:5B", "Avigilon Alta", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Cloud Access Control"},

    // Verkada (additional OUIs)
    {"88:DC:96", "Verkada", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Hybrid Cloud Cameras"},

    // Meraki (Cisco) - Cloud-managed cameras (UK enterprise)
    {"00:18:0A", "Cisco Meraki", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Cloud Cameras"},
    {"AC:17:C8", "Cisco Meraki", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "MV Cameras"},
    {"E0:55:3D", "Cisco Meraki", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Smart Cameras"},

    // Eagle Eye Networks - Cloud VMS (UK market)
    {"00:0C:84", "Eagle Eye Networks", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_RETAIL, "Cloud VMS"},

    // Rhombus - Cloud security cameras
    {"9C:4E:36", "Rhombus Systems", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_RETAIL, "Cloud Cameras"},

    // Hanwha (additional Samsung security OUIs)
    {"00:0D:F0", "Hanwha Techwin", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Wisenet Cameras"},
    {"20:13:E0", "Hanwha Techwin", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Network Cameras"},

    // Sony (additional professional OUIs)
    {"08:00:46", "Sony", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "IP Cameras"},
    {"50:EB:1A", "Sony", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Network Cameras"},

    // Canon (additional OUIs)
    {"00:04:A9", "Canon", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "VB Series Cameras"},

    // Panasonic (additional OUIs)
    {"00:0D:C1", "Panasonic", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "i-PRO Cameras"},
    {"00:80:64", "Panasonic", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "WV Series Cameras"},

    // Bosch (additional security OUIs)
    {"00:12:E0", "Bosch Security", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Autodome Cameras"},
    {"00:1A:A0", "Bosch Security", CAT_TRAFFIC, REL_HIGH, DEPLOY_POLICE, "Traffic Solutions"},

    // Axis (additional OUIs)
    {"00:09:2D", "Axis Communications", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "M-Series Cameras"},
    {"00:50:C2", "Axis Communications", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "P-Series Cameras"},

    // DJI (additional drone OUIs)
    {"AC:17:02", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Enterprise Drones"},
    {"D0:53:C4", "DJI", CAT_DRONE, REL_MEDIUM, DEPLOY_GOVERNMENT, "Matrice Series"},

    // Autel (additional OUIs)
    {"DC:9F:DB", "Autel Robotics", CAT_DRONE, REL_MEDIUM, DEPLOY_GOVERNMENT, "EVO Series Drones"},

    // Yuneec - Commercial drones (UK emergency services)
    {"00:26:66", "Yuneec", CAT_DRONE, REL_MEDIUM, DEPLOY_POLICE, "H520 Police Drones"},

    // senseFly (Parrot subsidiary) - Fixed-wing survey drones
    {"90:3A:E6", "senseFly", CAT_DRONE, REL_LOW, DEPLOY_GOVERNMENT, "Survey Drones"},

    // Blue Security (UK CCTV installer - used OUI may vary)
    {"00:1E:37", "Blue Security", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "UK CCTV Installer"},

    // Milestone (additional OUIs for XProtect VMS)
    {"00:50:C2", "Milestone Systems", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "XProtect VMS"},

    // Genetec (additional OUIs)
    {"00:15:C5", "Genetec", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "AutoVu ANPR"},

    // Wavestore - UK-based VMS manufacturer
    {"00:0E:C6", "Wavestore", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "UK Video Management"},

    // Qognify (OnSSI/NICE) - Enterprise VMS
    {"00:1B:67", "Qognify", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Ocularis VMS"},

    // Tyco/Johnson Controls - Integrated security
    {"00:12:CF", "Tyco Security", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "victor/Illustra"},

    // Honeywell (additional security OUIs)
    {"00:06:2A", "Honeywell", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "equIP Cameras"},
    {"00:D0:06", "Honeywell", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Performance Series"},

    // Avigilon (additional H4/H5 camera OUIs)
    {"68:EB:C5", "Avigilon", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "H5A Cameras"},

    // UTC Fire & Security (Interlogix) - Now part of Carrier
    {"00:0D:20", "Interlogix", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "TruVision Cameras"},

    // Uniview (formerly Unisight) - Growing UK market share
    {"B4:A3:82", "Uniview", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "IPC Cameras"},
    {"00:12:12", "Uniview", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "Network Cameras"},

    // Tiandy - Chinese manufacturer (UK installations)
    {"00:1F:AF", "Tiandy", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "IP Cameras"},

    // CP Plus - Budget CCTV (UK market)
    {"00:1E:8C", "CP Plus", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "Budget CCTV"},

    // LTS (Hikvision OEM brand in US/UK)
    {"00:1B:63", "LTS Security", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Platinum Series"},

    // Digital Watchdog - Spectrum/MEGApix (UK market)
    {"00:11:D9", "Digital Watchdog", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "DW Spectrum"},

    // Razberi Technologies - Surveillance servers/appliances
    {"00:1C:14", "Razberi", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "ServerSwitch"},

    // FLIR (additional thermal/surveillance OUIs)
    {"00:40:D0", "FLIR Systems", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Thermal Cameras"},
    {"00:05:07", "FLIR Systems", CAT_CCTV, REL_HIGH, DEPLOY_GOVERNMENT, "Security Thermal"},

    // Ring (additional Amazon OUIs)
    {"B0:4E:26", "Ring", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Video Doorbell Pro"},
    {"FC:92:8F", "Ring", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Stick Up Cam"},

    // Google Nest (additional OUIs)
    {"1C:3E:84", "Google Nest", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Hello Doorbell"},
    {"F0:EF:86", "Google Nest", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Nest Cam IQ"},

    // TP-Link (Tapo/Kasa cameras - popular in UK)
    {"50:C7:BF", "TP-Link", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Tapo Cameras"},
    {"84:D8:1B", "TP-Link", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Kasa Cameras"},

    // Anker (Eufy additional OUIs)
    {"34:EF:B6", "Anker", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Eufy Video Doorbell"},
    {"24:0A:C4", "Anker", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Eufy Cameras"},

    // Xiaomi - Mi/Aqara cameras (UK market)
    {"34:CE:00", "Xiaomi", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Mi Cameras"},
    {"78:11:DC", "Xiaomi", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Aqara Cameras"},

    // Imou (Dahua consumer brand)
    {"38:D2:CA", "Imou", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Dahua Consumer"},

    // Amcrest (Dahua OEM)
    {"00:62:6E", "Amcrest", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "Dahua OEM"},
    {"9C:8E:CD", "Amcrest", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Cloud Cameras"},

    // Reolink (additional OUIs)
    {"00:03:7F", "Reolink", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "RLC Series"},
    {"B0:A7:B9", "Reolink", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "WiFi Cameras"},

    // Unifi (Ubiquiti additional OUIs)
    {"18:E8:29", "Ubiquiti Networks", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "UniFi Cameras"},
    {"24:5A:4C", "Ubiquiti Networks", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "G3/G4 Cameras"},
    {"FC:EC:DA", "Ubiquiti Networks", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "UniFi Protect"},

    // ============================================================
    // FACIAL RECOGNITION SYSTEMS (Cardiff/UK Police)
    // ============================================================

    // NEC Corporation - NeoFace Live used by South Wales Police (Cardiff)
    // Critical: South Wales Police used NEC NeoFace for controversial live FR trials 2017-2020
    {"00:00:86", "NEC Corporation", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "NeoFace Live FR (Cardiff)"},
    {"00:00:D1", "NEC Corporation", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "NEC FR Systems"},
    {"00:40:66", "NEC Corporation", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "Police FR Servers"},
    {"00:1B:C0", "NEC Corporation", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "NEC Surveillance"},

    // Cognitec Systems - German FR vendor used in UK
    {"00:0E:3B", "Cognitec", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "FaceVACS FR System"},
    {"00:50:BA", "Cognitec", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_GOVERNMENT, "Facial Recognition"},

    // BriefCam - Video analytics for retrospective FR (UK police use)
    {"00:50:56", "BriefCam", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "Video Analytics/FR"},
    {"A4:5E:60", "BriefCam", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Forensic Video"},

    // AnyVision (now Oosto) - FR systems (used in UK retail/transport)
    {"00:1C:23", "AnyVision", CAT_FACIAL_RECOG, REL_MEDIUM, DEPLOY_RETAIL, "Retail FR Systems"},

    // FaceFirst (now Entrypoint) - Retail facial recognition
    {"00:50:C2", "FaceFirst", CAT_FACIAL_RECOG, REL_MEDIUM, DEPLOY_RETAIL, "Loss Prevention FR"},

    // Clearview AI - Controversial FR platform (UK police usage reported)
    {"00:1A:6B", "Clearview AI", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "FR Database"},

    // ============================================================
    // UK COUNCIL PARKING & CIVIL ENFORCEMENT
    // ============================================================

    // Conduent (formerly Xerox) - Major UK parking enforcement contractor
    {"00:00:AA", "Conduent", CAT_PARKING_ENFORCEMENT, REL_MEDIUM, DEPLOY_COUNCIL, "Parking Enforcement"},
    {"00:08:02", "Conduent", CAT_ANPR, REL_MEDIUM, DEPLOY_COUNCIL, "PCN Cameras"},
    {"00:D0:B7", "Conduent", CAT_PARKING_ENFORCEMENT, REL_MEDIUM, DEPLOY_COUNCIL, "Civil Enforcement"},

    // NSL Services - UK parking/traffic enforcement
    {"00:1E:58", "NSL Services", CAT_PARKING_ENFORCEMENT, REL_MEDIUM, DEPLOY_COUNCIL, "Parking Enforcement"},
    {"00:50:C2", "NSL Services", CAT_ANPR, REL_MEDIUM, DEPLOY_COUNCIL, "Bus Lane Cameras"},

    // APCOA Parking - Major UK parking operator
    {"00:0F:EA", "APCOA", CAT_PARKING_ENFORCEMENT, REL_LOW, DEPLOY_COUNCIL, "Car Park ANPR"},
    {"00:30:48", "APCOA", CAT_PARKING_ENFORCEMENT, REL_LOW, DEPLOY_PRIVATE, "Parking Cameras"},

    // Euro Car Parks - UK parking enforcement
    {"00:1D:7E", "Euro Car Parks", CAT_PARKING_ENFORCEMENT, REL_LOW, DEPLOY_PRIVATE, "Private Parking"},

    // ParkingEye - UK parking enforcement (Capita)
    {"00:26:5E", "ParkingEye", CAT_PARKING_ENFORCEMENT, REL_LOW, DEPLOY_PRIVATE, "ANPR Parking"},

    // Smart Parking - UK parking sensor systems
    {"00:1E:C0", "Smart Parking", CAT_PARKING_ENFORCEMENT, REL_LOW, DEPLOY_COUNCIL, "Smart Parking Sensors"},

    // ============================================================
    // CARDIFF & SOUTH WALES INFRASTRUCTURE
    // ============================================================

    // Telent - UK CCTV infrastructure (councils/police)
    {"00:0B:82", "Telent", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "Council CCTV Network"},
    {"00:30:65", "Telent", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_TRANSPORT, "Traffic CCTV"},

    // Vicon Industries - Professional CCTV systems (UK councils)
    {"00:40:5A", "Vicon Industries", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "Professional CCTV"},
    {"00:0C:76", "Vicon Industries", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "VAX VMS"},

    // Oncam (formerly 360 Vision) - 360-degree cameras (UK councils)
    {"00:0E:D7", "Oncam", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "360° Cameras"},
    {"00:19:3E", "Oncam", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Grandeye Cameras"},

    // BCDVideo - Surveillance servers (UK police/council infrastructure)
    {"00:1C:C4", "BCDVideo", CAT_CCTV, REL_MEDIUM, DEPLOY_POLICE, "Surveillance Servers"},

    // Sunell - IP cameras (UK council deployments)
    {"00:12:FB", "Sunell", CAT_CCTV, REL_LOW, DEPLOY_COUNCIL, "IP Cameras"},

    // Geovision (additional OUIs for UK councils)
    {"00:12:1E", "Geovision", CAT_CCTV, REL_LOW, DEPLOY_COUNCIL, "Council CCTV"},

    // Wisenet (additional Hanwha for councils)
    {"00:09:6D", "Hanwha", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "Council Cameras"},

    // ============================================================
    // ADDITIONAL UK RETAIL FACIAL RECOGNITION
    // ============================================================

    // Facewatch - UK retail facial recognition (Tesco, Co-op trials)
    {"00:50:C2", "Facewatch", CAT_FACIAL_RECOG, REL_MEDIUM, DEPLOY_RETAIL, "Retail Loss Prevention"},

    // Auror - Retail crime intelligence (growing UK use)
    {"A4:83:E7", "Auror", CAT_FACIAL_RECOG, REL_MEDIUM, DEPLOY_RETAIL, "Retail Intelligence"},

    // ============================================================
    // ADDITIONAL COUNCIL INFRASTRUCTURE
    // ============================================================

    // Jacobs Engineering - UK smart city/traffic infrastructure
    {"00:1B:21", "Jacobs Engineering", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_COUNCIL, "Smart City CCTV"},

    // Amey - UK infrastructure services (CCTV maintenance for councils)
    {"00:0A:95", "Amey", CAT_CCTV, REL_LOW, DEPLOY_COUNCIL, "CCTV Infrastructure"},

    // Idemia (formerly Morpho) - Biometrics/FR for UK police
    {"00:30:AB", "Idemia", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "Police Biometrics"},
    {"00:50:C2", "Idemia", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_GOVERNMENT, "Facial Recognition"},

    // Thales - Security systems (UK government/transport)
    {"00:06:0D", "Thales", CAT_CCTV, REL_HIGH, DEPLOY_GOVERNMENT, "Government Security"},
    {"00:E0:63", "Thales", CAT_TRAFFIC, REL_HIGH, DEPLOY_TRANSPORT, "Transport Security"},

    // Morpho (now Idemia) - Legacy biometric systems
    {"00:0E:2E", "Morpho", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "Legacy FR Systems"},

    // ============================================================
    // CARDIFF SMART CITY INFRASTRUCTURE (Field-tested)
    // ============================================================
    // These OUIs were detected on Cardiff's networked smart poles (AGD640-XXXXX-poleX)
    // Poles are densely deployed in city center, transport hubs, and Principality Stadium area
    // These are generic WiFi/BLE modules embedded in surveillance/sensor infrastructure

    // Texas Instruments - Embedded wireless modules (detected on Cardiff poles)
    {"38:AB:41", "Texas Instruments", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "Cardiff Smart Poles"},
    {"00:12:4B", "Texas Instruments", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "IoT Wireless Module"},
    {"B0:B4:48", "Texas Instruments", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "CC2640 BLE Module"},

    // TDK Corporation - Industrial wireless modules (detected on Cardiff poles)
    {"00:80:98", "TDK Corporation", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "Cardiff Pole Sensors"},
    {"00:1D:94", "TDK Corporation", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "Industrial IoT"},

    // Ezurio (formerly Laird Connectivity) - Industrial wireless (detected on Cardiff poles)
    {"00:16:A4", "Ezurio", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "Cardiff Smart Infrastructure"},
    {"00:50:C2", "Ezurio", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "Industrial Wireless"},

    // Fn-Link Technology - WiFi/BLE modules for smart city (detected on Cardiff poles)
    {"AC:64:CF", "Fn-Link Technology", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "Cardiff Pole Network"},
    {"24:0A:C4", "Fn-Link Technology", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "Smart City WiFi"},

    // Additional smart city infrastructure vendors
    // ESP32/ESP8266 modules commonly used in council IoT (if you find these, likely custom builds)
    {"24:6F:28", "Espressif", CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "ESP32 IoT Module"},
    {"30:AE:A4", "Espressif", CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "ESP32 WiFi/BLE"},

    // Murata - Industrial WiFi/BLE modules (common in smart city)
    {"00:37:6D", "Murata", CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "Industrial WiFi Module"},

    // u-blox - Industrial IoT modules
    {"00:18:DA", "u-blox", CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "Industrial IoT Module"},
};

const int UK_OUI_DATABASE_SIZE = sizeof(UK_OUI_DATABASE) / sizeof(OUIEntry);

// Helper functions
const char* getCategoryName(DeviceCategory cat);
const char* getRelevanceName(RelevanceLevel rel);
const char* getDeploymentName(DeploymentType dep);
uint16_t getCategoryColor(DeviceCategory cat);
uint16_t getRelevanceColor(RelevanceLevel rel);

#endif
