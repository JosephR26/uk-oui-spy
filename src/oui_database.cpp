#include "oui_database.h"
#include <TFT_eSPI.h>

// ============================================================
// UK Surveillance Device OUI Database (363 entries)
// ============================================================
const OUIEntry OUI_DATABASE[] = {
    // ============================================================
    // HIKVISION - Major CCTV manufacturer, widely used by UK police/councils
    // ============================================================
    {"00:12:12", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "UK Police/Council CCTV"},
    {"28:57:BE", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"44:19:B6", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Network Cameras"},
    {"BC:AD:28", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "Transport CCTV"},
    {"54:C4:15", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "PTZ Cameras"},
    {"C4:2F:90", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Smart Cameras"},
    {"14:2D:27", "Hikvision", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "ANPR Systems"},
    {"4C:BD:8F", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Network Cameras"},
    {"68:E1:66", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP CCTV"},
    {"C0:56:E3", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "Surveillance Cameras"},
    {"D4:4B:5E", "Hikvision", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "DeepinMind ANPR"},
    {"F0:1D:BC", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Smart Cameras"},
    // ============================================================
    // AXIS COMMUNICATIONS - Premium surveillance, UK police/transport
    // ============================================================
    {"00:40:8C", "Axis Communications", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Body Cams/CCTV"},
    {"AC:CC:8E", "Axis Communications", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "Network Cameras"},
    {"B8:A4:4F", "Axis Communications", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Police Body Cameras"},
    {"00:09:2D", "Axis Communications", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "M-Series Cameras"},
    // ============================================================
    // DAHUA TECHNOLOGY - Major CCTV, UK council/retail use
    // ============================================================
    {"A4:DA:32", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Government/Council CCTV"},
    {"3C:EF:8C", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras and NVRs"},
    {"6C:C2:17", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Security Cameras (Field Validated)"},
    {"00:12:16", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"08:60:6E", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Network CCTV"},
    {"A0:BD:1D", "Dahua", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "PTZ Cameras"},
    {"78:D8:B5", "Dahua", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Traffic Cameras"},
    {"00:26:37", "Dahua", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "IP Cameras"},
    {"2C:44:05", "Dahua", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Network Cameras"},
    {"E8:CC:18", "Dahua", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "HDCVI Cameras"},
    {"F4:83:CD", "Dahua", CAT_ANPR, REL_MEDIUM, DEPLOY_POLICE, "LPR Cameras"},
    // ============================================================
    // MOTOROLA SOLUTIONS - UK Police systems (ANPR, body cams)
    // ============================================================
    {"00:0A:28", "Motorola Solutions", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "ANPR Systems"},
    {"00:23:68", "Motorola Solutions", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Police Body Cameras"},
    {"00:30:D3", "Motorola Solutions", CAT_TRAFFIC, REL_HIGH, DEPLOY_GOVERNMENT, "Traffic Systems"},
    {"00:04:56", "Motorola", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Police Equipment"},
    {"00:90:9C", "Motorola", CAT_TRAFFIC, REL_HIGH, DEPLOY_GOVERNMENT, "Traffic Management"},
    {"00:D0:BC", "Motorola", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "Public Safety ANPR"},
    // ============================================================
    // AVIGILON (Motorola) - High-end surveillance, UK police
    // ============================================================
    {"00:11:C1", "Avigilon", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "HD Surveillance"},
    {"00:05:CA", "Avigilon", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "HD Analytics"},
    {"D8:90:E8", "Avigilon", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "LPR/ANPR"},
    {"68:EB:C5", "Avigilon", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "H5A Cameras"},
    {"E4:11:5B", "Avigilon Alta", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Cloud Access Control"},
    // ============================================================
    // DJI & DRONES - Police, search & rescue
    // ============================================================
    {"60:60:1F", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Police Drones"},
    {"F0:F0:1D", "DJI", CAT_DRONE, REL_MEDIUM, DEPLOY_GOVERNMENT, "Surveillance Drones"},
    {"AC:17:02", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Enterprise Drones"},
    {"D0:53:C4", "DJI", CAT_DRONE, REL_MEDIUM, DEPLOY_GOVERNMENT, "Matrice Series"},
    {"00:60:37", "Skydio", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Autonomous Drone Controller (Field Validated)"},
    {"90:9F:33", "Sky Drone", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Autonomous Drone Platform (Field Validated)"},
    {"A0:14:3D", "Parrot", CAT_DRONE, REL_LOW, DEPLOY_PRIVATE, "Consumer Drones"},
    {"00:25:DF", "Autel Robotics", CAT_DRONE, REL_MEDIUM, DEPLOY_GOVERNMENT, "Commercial Drones"},
    {"DC:9F:DB", "Autel Robotics", CAT_DRONE, REL_MEDIUM, DEPLOY_GOVERNMENT, "EVO Series Drones"},
    {"00:26:66", "Yuneec", CAT_DRONE, REL_MEDIUM, DEPLOY_POLICE, "H520 Police Drones"},
    {"90:3A:E6", "senseFly", CAT_DRONE, REL_LOW, DEPLOY_GOVERNMENT, "Survey Drones"},
    // ============================================================
    // BODY CAMERAS - UK Police suppliers
    // ============================================================
    {"00:1E:C0", "Digital Barriers", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Police Body Cameras"},
    {"00:26:08", "Edesix", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "UK Police Body Cams"},
    {"00:1B:C5", "Reveal Media", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Body Worn Video"},
    {"00:02:55", "Axon Enterprise", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "UK Police Body Cams"},
    {"00:18:F3", "Axon Enterprise", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Axon Body Cameras"},
    {"6C:C7:EC", "Axon Enterprise", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Axon Fleet/Body Cams"},
    {"00:0C:D4", "WatchGuard Video", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Police Dash/Body Cams"},
    {"00:21:10", "Sepura", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Police Radio/BWV"},
    {"00:0E:1D", "Zepcam", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "Body Worn Cameras"},
    // ============================================================
    // HANWHA (Samsung) - Major CCTV supplier
    // ============================================================
    {"00:1A:3F", "Hanwha Techwin", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Samsung/Hanwha CCTV"},
    {"00:00:F0", "Hanwha Techwin", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Samsung CCTV"},
    {"00:09:18", "Hanwha Techwin", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "Network Cameras"},
    {"00:16:6C", "Hanwha Techwin", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Wisenet Cameras"},
    {"00:0D:F0", "Hanwha Techwin", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Wisenet Cameras"},
    {"20:13:E0", "Hanwha Techwin", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Network Cameras"},
    {"00:09:6D", "Hanwha", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "Council Cameras"},
    // ============================================================
    // BOSCH SECURITY - UK transport/government
    // ============================================================
    {"00:80:F0", "Bosch Security", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Council/Retail CCTV"},
    {"00:0E:8F", "Bosch Security", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Security Cameras"},
    {"00:1B:EE", "Bosch Security", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "ANPR/Traffic"},
    {"00:12:E0", "Bosch Security", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Autodome Cameras"},
    {"00:1A:A0", "Bosch Security", CAT_TRAFFIC, REL_HIGH, DEPLOY_POLICE, "Traffic Solutions"},
    // ============================================================
    // GENETEC - ANPR/CCTV integration platform
    // ============================================================
    {"E0:50:8B", "Genetec", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_GOVERNMENT, "Facial Recognition Systems"},
    {"00:0C:E5", "Genetec", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "ANPR/Security Platform"},
    {"00:15:C5", "Genetec", CAT_ANPR, REL_HIGH, DEPLOY_POLICE, "AutoVu ANPR"},
    // ============================================================
    // ANPR & TRAFFIC ENFORCEMENT
    // ============================================================
    {"00:03:52", "Kapsch", CAT_ANPR, REL_HIGH, DEPLOY_GOVERNMENT, "ULEZ/ANPR London"},
    {"00:21:5C", "Kapsch", CAT_TRAFFIC, REL_HIGH, DEPLOY_TRANSPORT, "Congestion Charging"},
    {"00:30:05", "SWARCO", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_TRANSPORT, "Traffic Signals/ANPR"},
    {"00:0C:A4", "Jenoptik", CAT_TRAFFIC, REL_HIGH, DEPLOY_POLICE, "Speed/ANPR Cameras"},
    {"00:07:7C", "Tattile", CAT_ANPR, REL_MEDIUM, DEPLOY_TRANSPORT, "ANPR Solutions"},
    {"00:1F:CD", "Redflex", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_GOVERNMENT, "Speed Cameras"},
    {"00:0A:E4", "Verra Mobility", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_GOVERNMENT, "Traffic Enforcement"},
    // ============================================================
    // SIEMENS - UK traffic cameras and smart city
    // ============================================================
    {"00:0E:8C", "Siemens", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_TRANSPORT, "Traffic CCTV"},
    {"00:50:7F", "Siemens", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_GOVERNMENT, "Smart City Cameras"},
    {"00:1B:1B", "Siemens", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_TRANSPORT, "Transport Systems"},
    // ============================================================
    // FACIAL RECOGNITION SYSTEMS (Cardiff/UK Police)
    // ============================================================
    // NEC Corporation - NeoFace Live used by South Wales Police (Cardiff)
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
    // AnyVision (now Oosto) - FR systems (UK retail/transport)
    {"00:1C:23", "AnyVision", CAT_FACIAL_RECOG, REL_MEDIUM, DEPLOY_RETAIL, "Retail FR Systems"},
    // Clearview AI - Controversial FR platform (UK police usage reported)
    {"00:1A:6B", "Clearview AI", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "FR Database"},
    // Idemia (formerly Morpho) - Biometrics/FR for UK police
    {"00:30:AB", "Idemia", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "Police Biometrics"},
    {"00:0E:2E", "Morpho", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "Legacy FR Systems"},
    // Auror - Retail crime intelligence (growing UK use)
    {"A4:83:E7", "Auror", CAT_FACIAL_RECOG, REL_MEDIUM, DEPLOY_RETAIL, "Retail Intelligence"},
    // ============================================================
    // UK COUNCIL PARKING & CIVIL ENFORCEMENT
    // ============================================================
    {"00:00:AA", "Conduent", CAT_PARKING_ENFORCEMENT, REL_MEDIUM, DEPLOY_COUNCIL, "Parking Enforcement"},
    {"00:08:02", "Conduent", CAT_ANPR, REL_MEDIUM, DEPLOY_COUNCIL, "PCN Cameras"},
    {"00:D0:B7", "Conduent", CAT_PARKING_ENFORCEMENT, REL_MEDIUM, DEPLOY_COUNCIL, "Civil Enforcement"},
    {"00:1E:58", "NSL Services", CAT_PARKING_ENFORCEMENT, REL_MEDIUM, DEPLOY_COUNCIL, "Parking Enforcement"},
    {"00:0F:EA", "APCOA", CAT_PARKING_ENFORCEMENT, REL_LOW, DEPLOY_COUNCIL, "Car Park ANPR"},
    {"00:30:48", "APCOA", CAT_PARKING_ENFORCEMENT, REL_LOW, DEPLOY_PRIVATE, "Parking Cameras"},
    {"00:1D:7E", "Euro Car Parks", CAT_PARKING_ENFORCEMENT, REL_LOW, DEPLOY_PRIVATE, "Private Parking"},
    {"00:26:5E", "ParkingEye", CAT_PARKING_ENFORCEMENT, REL_LOW, DEPLOY_PRIVATE, "ANPR Parking"},
    // ============================================================
    // PROFESSIONAL CCTV & VMS MANUFACTURERS
    // ============================================================
    {"00:50:C2", "Milestone Systems", CAT_CCTV, REL_HIGH, DEPLOY_GOVERNMENT, "VMS Infrastructure"},
    {"00:0C:C8", "Milestone Systems", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Police VMS"},
    {"00:18:7D", "Pelco (Motorola)", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "Police/Transport CCTV"},
    {"00:03:BE", "Pelco", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Surveillance Systems"},
    {"00:03:C5", "Mobotix", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Decentralized Cameras"},
    {"00:02:D1", "Vivotek", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "IP Surveillance"},
    {"00:13:FE", "IndigoVision", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "IP CCTV"},
    {"00:11:5B", "Dallmeier", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Panomera Cameras"},
    {"00:11:98", "ACTi", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "IP Cameras"},
    {"00:40:FA", "Exacq Technologies", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Video Management"},
    {"00:19:70", "Salient Systems", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "CompleteView VMS"},
    {"00:03:C0", "March Networks", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Transit Surveillance"},
    {"00:02:A2", "Dedicated Micros", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "UK DVR/NVR"},
    {"00:0D:8B", "360 Vision", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "UK PTZ Cameras"},
    {"00:1D:09", "Videcon", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "UK CCTV Systems"},
    {"00:0E:C6", "Wavestore", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "UK Video Management"},
    {"00:1B:67", "Qognify", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Ocularis VMS"},
    {"00:12:CF", "Tyco Security", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "victor/Illustra"},
    {"00:0D:20", "Interlogix", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "TruVision Cameras"},
    {"B4:A3:82", "Uniview", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "IPC Cameras"},
    {"00:1F:AF", "Tiandy", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "IP Cameras"},
    {"00:1E:8C", "CP Plus", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "Budget CCTV"},
    {"00:1B:63", "LTS Security", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Platinum Series"},
    {"00:11:D9", "Digital Watchdog", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "DW Spectrum"},
    {"00:1C:14", "Razberi", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "ServerSwitch"},
    // ============================================================
    // PANASONIC / SONY / CANON - Professional cameras
    // ============================================================
    {"00:80:15", "Panasonic", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Security Cameras"},
    {"00:0D:C1", "Panasonic", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "i-PRO Cameras"},
    {"00:80:64", "Panasonic", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "WV Series Cameras"},
    {"00:1D:BA", "Sony", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Network Cameras"},
    {"08:00:46", "Sony", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "IP Cameras"},
    {"50:EB:1A", "Sony", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Network Cameras"},
    {"00:00:85", "Canon", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Network Cameras"},
    {"00:04:A9", "Canon", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "VB Series Cameras"},
    // ============================================================
    // HONEYWELL - Security systems
    // ============================================================
    {"00:15:7D", "Honeywell", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "Security Systems"},
    {"00:E0:4C", "Honeywell", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Building Security"},
    {"00:06:2A", "Honeywell", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "equIP Cameras"},
    {"00:D0:06", "Honeywell", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Performance Series"},
    // ============================================================
    // THALES - UK government/transport security
    // ============================================================
    {"00:06:0D", "Thales", CAT_CCTV, REL_HIGH, DEPLOY_GOVERNMENT, "Government Security"},
    {"00:E0:63", "Thales", CAT_TRAFFIC, REL_HIGH, DEPLOY_TRANSPORT, "Transport Security"},
    // ============================================================
    // THERMAL CAMERAS - FLIR Systems
    // ============================================================
    {"00:0D:66", "FLIR Systems", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Thermal Cameras"},
    {"00:40:D0", "FLIR Systems", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Thermal Cameras"},
    {"00:05:07", "FLIR Systems", CAT_CCTV, REL_HIGH, DEPLOY_GOVERNMENT, "Security Thermal"},
    // ============================================================
    // CLOUD-MANAGED CAMERAS - Cisco Meraki / Verkada / Eagle Eye
    // ============================================================
    {"00:1C:B3", "Cisco Meraki", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Cloud Managed Cameras"},
    {"00:18:0A", "Cisco Meraki", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Cloud Cameras"},
    {"AC:17:C8", "Cisco Meraki", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "MV Cameras"},
    {"E0:55:3D", "Cisco Meraki", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Smart Cameras"},
    {"E0:1F:88", "Verkada", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Cloud CCTV"},
    {"88:DC:96", "Verkada", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Hybrid Cloud Cameras"},
    {"00:0C:84", "Eagle Eye Networks", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_RETAIL, "Cloud VMS"},
    {"9C:4E:36", "Rhombus Systems", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_RETAIL, "Cloud Cameras"},
    // ============================================================
    // UBIQUITI - UniFi Protect cameras
    // ============================================================
    {"B8:69:F4", "Ubiquiti Networks", CAT_CCTV, REL_HIGH, DEPLOY_PRIVATE, "UniFi Cameras (Field Validated)"},
    {"18:E8:29", "Ubiquiti Networks", CAT_CCTV, REL_HIGH, DEPLOY_PRIVATE, "UniFi Cameras (Field Validated)"},
    {"74:83:C2", "Ubiquiti Networks", CAT_CCTV, REL_HIGH, DEPLOY_PRIVATE, "UniFi Cameras (Field Validated)"},
    {"B4:FB:E4", "Ubiquiti Networks", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "UniFi Protect"},
    {"24:5A:4C", "Ubiquiti Networks", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "G3/G4 Cameras"},
    {"FC:EC:DA", "Ubiquiti Networks", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "UniFi Protect"},
    // ============================================================
    // UK SECURITY PROVIDERS - ADT, Securitas, Blue Security
    // ============================================================
    {"00:19:5B", "Securitas", CAT_CCTV, REL_MEDIUM, DEPLOY_PRIVATE, "Verisure CCTV"},
    {"00:13:02", "ADT Security", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Monitored CCTV"},
    {"00:1E:37", "Blue Security", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "UK CCTV Installer"},
    // ============================================================
    // CARDIFF & SOUTH WALES INFRASTRUCTURE
    // ============================================================
    {"00:0B:82", "Telent", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "Council CCTV Network"},
    {"00:30:65", "Telent", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_TRANSPORT, "Traffic CCTV"},
    {"00:40:5A", "Vicon Industries", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "Professional CCTV"},
    {"00:0C:76", "Vicon Industries", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "VAX VMS"},
    {"00:0E:D7", "Oncam", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "360 Cameras"},
    {"00:19:3E", "Oncam", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Grandeye Cameras"},
    {"00:1C:C4", "BCDVideo", CAT_CCTV, REL_MEDIUM, DEPLOY_POLICE, "Surveillance Servers"},
    {"00:12:FB", "Sunell", CAT_CCTV, REL_LOW, DEPLOY_COUNCIL, "IP Cameras"},
    {"00:12:1E", "Geovision", CAT_CCTV, REL_LOW, DEPLOY_COUNCIL, "Council CCTV"},
    {"00:1B:21", "Jacobs Engineering", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_COUNCIL, "Smart City CCTV"},
    {"00:0A:95", "Amey", CAT_CCTV, REL_LOW, DEPLOY_COUNCIL, "CCTV Infrastructure"},
    // ============================================================
    // DASH CAMS
    // ============================================================
    {"D8:60:CF", "Smart Dashcam", CAT_DASH_CAM, REL_MEDIUM, DEPLOY_PRIVATE, "Delivery Fleet/Bodycam (Field Validated)"},
    {"28:87:BA", "GoPro", CAT_DASH_CAM, REL_MEDIUM, DEPLOY_PRIVATE, "Action Cameras (Field Validated)"},
    {"00:07:AB", "Nextbase", CAT_DASH_CAM, REL_LOW, DEPLOY_PRIVATE, "Dash Cameras"},
    {"00:11:32", "BlackVue", CAT_DASH_CAM, REL_LOW, DEPLOY_PRIVATE, "Cloud Dash Cams"},
    {"00:0C:6E", "Garmin", CAT_DASH_CAM, REL_LOW, DEPLOY_PRIVATE, "Dash Cameras"},
    {"00:37:6D", "Thinkware", CAT_DASH_CAM, REL_LOW, DEPLOY_PRIVATE, "Dash Cameras"},
    {"00:26:5A", "Viofo", CAT_DASH_CAM, REL_LOW, DEPLOY_PRIVATE, "Dash Cameras"},
    // ============================================================
    // CONSUMER DOORBELLS & CLOUD CAMERAS
    // ============================================================
    {"74:C6:3B", "Ring (Amazon)", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Video Doorbells"},
    {"EC:71:DB", "Ring (Amazon)", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Video Doorbells"},
    {"88:71:E5", "Ring (Amazon)", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Security Cameras"},
    {"B0:4E:26", "Ring", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Video Doorbell Pro"},
    {"FC:92:8F", "Ring", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Stick Up Cam"},
    {"18:B4:30", "Nest (Google)", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Cloud Cameras"},
    {"64:16:66", "Nest Labs", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Video Doorbells"},
    {"1C:3E:84", "Google Nest", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Hello Doorbell"},
    {"F0:EF:86", "Google Nest", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Nest Cam IQ"},
    {"D0:73:D5", "Arlo Technologies", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Wireless Cameras"},
    {"2C:AA:8E", "Wyze Labs", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Smart Cameras"},
    {"T8:1D:7F", "Anker", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Eufy Cameras"},
    {"34:EF:B6", "Anker", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Eufy Video Doorbell"},
    {"24:0A:C4", "Anker", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Eufy Cameras"},
    {"A0:02:DC", "Amazon", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Blink Cameras"},
    {"F0:D5:BF", "Yale", CAT_DOORBELL_CAM, REL_LOW, DEPLOY_PRIVATE, "Smart Locks"},
    {"50:C7:BF", "TP-Link", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Tapo Cameras (Field Validated)"},
    {"84:D8:1B", "TP-Link", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Kasa Cameras"},
    {"34:CE:00", "Xiaomi", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Mi Cameras"},
    {"78:11:DC", "Xiaomi", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Aqara Cameras"},
    {"38:D2:CA", "Imou", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Dahua Consumer"},
    {"00:62:6E", "Amcrest", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "Dahua OEM"},
    {"9C:8E:CD", "Amcrest", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Cloud Cameras"},
    {"B0:A7:B9", "Reolink", CAT_CLOUD_CCTV, REL_MEDIUM, DEPLOY_PRIVATE, "WiFi Cameras (Field Validated)"},
    {"00:03:7F", "Reolink", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "RLC Series"},
    {"00:06:D2", "Geovision", CAT_CCTV, REL_LOW, DEPLOY_RETAIL, "DVR/NVR Systems"},
    {"00:21:23", "Lorex", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "Home Security"},
    {"00:11:8B", "Swann", CAT_CCTV, REL_LOW, DEPLOY_PRIVATE, "DIY CCTV"},
    // ============================================================
    // SMART CITY INFRASTRUCTURE (Cardiff Field-tested)
    // ============================================================
    {"38:AB:41", "Texas Instruments", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "Cardiff Smart Poles (Field Validated)"},
    {"AC:64:CF", "Fn-Link Technology", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "Cardiff Pole Network (Field Validated)"},
    {"00:12:4B", "Texas Instruments", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "IoT Wireless Module"},
    {"B0:B4:48", "Texas Instruments", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "CC2640 BLE Module"},
    {"00:80:98", "TDK Corporation", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "Cardiff Pole Sensors"},
    {"00:1D:94", "TDK Corporation", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "Industrial IoT"},
    {"00:16:A4", "Ezurio", CAT_SMART_CITY_INFRA, REL_MEDIUM, DEPLOY_COUNCIL, "Cardiff Smart Infrastructure"},
    {"24:6F:28", "Espressif", CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "ESP32 IoT Module"},
    {"30:AE:A4", "Espressif", CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "ESP32 WiFi/BLE"},
    {"00:18:DA", "u-blox", CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "Industrial IoT Module"},
    // ============================================================
    // ADDITIONAL UK-RELEVANT SURVEILLANCE OUIs
    // ============================================================
    {"E8:AB:FA", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "ColorVu Cameras"},
    {"48:0F:CF", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "AcuSense Cameras"},
    {"40:F4:FD", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "WizSense Cameras"},
    {"34:4B:50", "Dahua Technology", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "TiOC Cameras"},
    {"00:04:E2", "SMC Networks", CAT_TRAFFIC, REL_MEDIUM, DEPLOY_TRANSPORT, "Traffic Infrastructure"},
    {"DC:54:D7", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Mini Series Drones"},
    {"00:1A:07", "i-PRO (Panasonic)", CAT_CCTV, REL_MEDIUM, DEPLOY_GOVERNMENT, "X-Series Cameras"},
    {"B0:B2:DC", "Zyxel", CAT_SMART_CITY_INFRA, REL_LOW, DEPLOY_COUNCIL, "Council Network Infra"},
    // ============================================================
    // CONSUMER BASELINE (filtered by default)
    // ============================================================
    {"74:DA:88", "Sky CPE", CAT_UNKNOWN, REL_LOW, DEPLOY_PRIVATE, "Consumer Broadband (Baseline)"},
    {"FC:F8:AE", "BT/EE Hub", CAT_UNKNOWN, REL_LOW, DEPLOY_PRIVATE, "Consumer Broadband (Baseline)"},
    {"20:8B:FB", "TP-Link", CAT_UNKNOWN, REL_LOW, DEPLOY_PRIVATE, "Consumer Networking (Baseline)"},
    // ── HIKVISION (expanded) ──
    {"00:BC:99", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"04:03:12", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"04:EE:CD", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Network Cameras"},
    {"08:3B:C1", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"08:54:11", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"08:A1:89", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "Network Cameras"},
    {"08:CC:81", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Smart Cameras"},
    {"0C:75:D2", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"10:12:FB", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "CCTV Systems"},
    {"18:68:CB", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"18:80:25", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Network Cameras"},
    {"24:0F:9B", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"24:28:FD", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "PTZ Cameras"},
    {"24:32:AE", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Network Cameras"},
    {"24:48:45", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "IP Cameras"},
    {"2C:A5:9C", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Network Cameras"},
    {"34:09:62", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "CCTV Systems"},
    {"3C:1B:F8", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"40:AC:BF", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Smart Cameras"},
    {"44:47:CC", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Network Cameras"},
    {"44:A6:42", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"48:78:5B", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "PTZ Cameras"},
    {"4C:1F:86", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Network Cameras"},
    {"4C:62:DF", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "IP Cameras"},
    {"4C:F5:DC", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Smart Cameras"},
    {"50:E5:38", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Network Cameras"},
    {"54:8C:81", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"58:03:FB", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "PTZ Cameras"},
    {"58:50:ED", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "IP Cameras"},
    {"5C:34:5B", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Network Cameras"},
    {"64:DB:8B", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"68:6D:BC", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "ANPR Systems"},
    {"74:3F:C2", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "Network Cameras"},
    {"80:48:9F", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"80:7C:62", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Smart Cameras"},
    {"80:BE:AF", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Network Cameras"},
    {"80:F5:AE", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "CCTV Systems"},
    {"84:94:59", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "IP Cameras"},
    {"84:9A:40", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Network Cameras"},
    {"88:DE:39", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "IP Cameras"},
    {"8C:22:D2", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Smart Cameras"},
    {"8C:E7:48", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "Network Cameras"},
    {"94:E1:AC", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"98:8B:0A", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "CCTV Systems"},
    {"98:9D:E5", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Network Cameras"},
    {"98:DF:82", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "IP Cameras"},
    {"98:F1:12", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "PTZ Cameras"},
    {"A0:FF:0C", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"A4:14:37", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Smart Cameras"},
    {"A4:29:02", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Network Cameras"},
    {"A4:4B:D9", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "IP Cameras"},
    {"A4:A4:59", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "CCTV Systems"},
    {"A4:D5:C2", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Network Cameras"},
    {"AC:B9:2F", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"AC:CB:51", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Smart Cameras"},
    {"BC:5E:33", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"BC:9B:5E", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "CCTV Systems"},
    {"BC:BA:C2", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Network Cameras"},
    {"C0:51:7E", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"C0:6D:ED", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "PTZ Cameras"},
    {"C8:A7:02", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Smart Cameras"},
    {"D4:E8:53", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "IP Cameras"},
    {"DC:07:F8", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Network Cameras"},
    {"DC:D2:6A", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "IP Cameras"},
    {"E0:BA:AD", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "CCTV Systems"},
    {"E0:CA:3C", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Network Cameras"},
    {"E0:DF:13", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "IP Cameras"},
    {"E4:D5:8B", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Smart Cameras"},
    {"E8:A0:ED", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Network Cameras"},
    {"EC:A9:71", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "IP Cameras"},
    {"EC:C8:9C", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "CCTV Systems"},
    {"F8:4D:FC", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Network Cameras"},
    {"FC:9F:FD", "Hikvision", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    // ── DAHUA (expanded) ──
    {"08:ED:ED", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"14:A7:8B", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Network Cameras"},
    {"24:52:6A", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"38:AF:29", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "Network Cameras"},
    {"3C:E3:6B", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Smart Cameras"},
    {"4C:11:BF", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "IP Cameras"},
    {"5C:F5:1A", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Network Cameras"},
    {"64:FD:29", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "IP Cameras"},
    {"6C:1C:71", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "PTZ Cameras"},
    {"74:C9:29", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "Network Cameras"},
    {"8C:E9:B4", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"90:02:A9", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Smart Cameras"},
    {"98:F9:CC", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Network Cameras"},
    {"9C:14:63", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "IP Cameras"},
    {"B4:4C:3B", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Network Cameras"},
    {"BC:32:5F", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "CCTV Systems"},
    {"C0:39:5A", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    {"C4:AA:C4", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Network Cameras"},
    {"D4:43:0E", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Smart Cameras"},
    {"E0:2E:FE", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "PTZ Cameras"},
    {"E4:24:6C", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "IP Cameras"},
    {"F4:B1:C2", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "CCTV Systems"},
    {"FC:5F:49", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Network Cameras"},
    {"FC:B6:9D", "Dahua Technology", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "IP Cameras"},
    // ── DJI (expanded) ──
    {"04:A8:5A", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Commercial/Police Drone"},
    {"0C:9A:E6", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_PRIVATE, "Consumer Drone"},
    {"34:D2:62", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_GOVERNMENT, "Surveillance Drone"},
    {"48:1C:B9", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Police Drone"},
    {"4C:43:F6", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_PRIVATE, "Commercial Drone"},
    {"58:B8:58", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_GOVERNMENT, "Enterprise Drone"},
    {"88:29:85", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Surveillance Drone"},
    {"8C:58:23", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_PRIVATE, "Consumer Drone"},
    {"E4:7A:2C", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Police/Enterprise Drone"},
    {"9C:5A:8A", "DJI", CAT_DRONE, REL_HIGH, DEPLOY_GOVERNMENT, "DJI Baiwang Drone"},
    // ── PARROT ──
    {"00:12:1C", "Parrot", CAT_DRONE, REL_MEDIUM, DEPLOY_PRIVATE, "Consumer Drone"},
    {"00:26:7E", "Parrot", CAT_DRONE, REL_MEDIUM, DEPLOY_PRIVATE, "Commercial Drone"},
    {"90:03:B7", "Parrot", CAT_DRONE, REL_MEDIUM, DEPLOY_PRIVATE, "Drone Controller"},
    // ── SKYDIO ──
    {"38:1D:14", "Skydio", CAT_DRONE, REL_HIGH, DEPLOY_POLICE, "Autonomous Drone"},
    // ── SEPURA (UK TETRA) ──
    {"00:1E:96", "Sepura", CAT_BODYCAM, REL_HIGH, DEPLOY_POLICE, "UK Police TETRA Radio"},
    // ── HYTERA ──
    {"9C:06:6E", "Hytera", CAT_BODYCAM, REL_MEDIUM, DEPLOY_POLICE, "DMR/TETRA Radio + Bodycam"},
    // ── AVIGILON (expanded) ──
    {"00:18:85", "Avigilon", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "AI Surveillance Camera"},
    {"70:1A:D5", "Avigilon", CAT_CCTV, REL_HIGH, DEPLOY_GOVERNMENT, "AI Surveillance Camera"},
    // ── GENETEC ──
    {"00:BF:15", "Genetec", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_POLICE, "Facial Recognition VMS"},
    {"0C:BF:15", "Genetec", CAT_FACIAL_RECOG, REL_HIGH, DEPLOY_GOVERNMENT, "AutoVu ANPR / FR"},
    // ── FLIR (Teledyne) ──
    {"00:13:56", "FLIR Systems", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Thermal Camera"},
    {"00:1B:D8", "FLIR Systems", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Thermal Imaging"},
    {"00:40:7F", "FLIR Systems", CAT_CCTV, REL_HIGH, DEPLOY_GOVERNMENT, "Thermal Surveillance"},
    // ── BOSCH SECURITY ──
    {"00:04:63", "Bosch Security", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "IP Camera"},
    {"30:F0:28", "Bosch Security", CAT_CCTV, REL_HIGH, DEPLOY_COUNCIL, "Network Camera"},
    // ── HANWHA VISION (expanded) ──
    {"E4:30:22", "Hanwha Vision", CAT_CCTV, REL_HIGH, DEPLOY_RETAIL, "Network Camera"},
    // ── UNIVIEW / UNV ──
    {"48:EA:63", "Uniview", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "IP Camera"},
    {"6C:F1:7E", "Uniview", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "Network Camera"},
    {"88:26:3F", "Uniview", CAT_CCTV, REL_MEDIUM, DEPLOY_COUNCIL, "IP Camera"},
    {"C4:79:05", "Uniview", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "PTZ Camera"},
    // ── HID GLOBAL ──
    {"00:06:8E", "HID Global", CAT_SMART_CITY_INFRA, REL_HIGH, DEPLOY_GOVERNMENT, "Access Control Reader"},
    // ── ASSA ABLOY ──
    {"00:17:7A", "Assa Abloy", CAT_SMART_CITY_INFRA, REL_HIGH, DEPLOY_GOVERNMENT, "Smart Lock/Access Control"},
    // ── PELCO ──
    {"00:04:7D", "Pelco", CAT_CCTV, REL_HIGH, DEPLOY_TRANSPORT, "CCTV Camera"},
    // ── INDIGOVISION ──
    {"00:90:AA", "IndigoVision", CAT_CCTV, REL_HIGH, DEPLOY_POLICE, "Network Video (Legacy)"},
    // ── MARCH NETWORKS ──
    {"00:10:BE", "March Networks", CAT_CCTV, REL_MEDIUM, DEPLOY_RETAIL, "NVR/VMS"},
    {"00:12:81", "March Networks", CAT_CCTV, REL_MEDIUM, DEPLOY_TRANSPORT, "Network Video Recorder"},
    // ── CONSUMER SURVEILLANCE (LOW priority) ──
    {"44:A5:6E", "Arlo", CAT_CLOUD_CCTV, REL_LOW, DEPLOY_PRIVATE, "Wireless Security Camera"},
};

const size_t OUI_DATABASE_SIZE = sizeof(OUI_DATABASE) / sizeof(OUI_DATABASE[0]);

const OUIEntry* findOUI(const String& oui) {
    for (size_t i = 0; i < OUI_DATABASE_SIZE; i++) {
        if (oui.equalsIgnoreCase(OUI_DATABASE[i].oui)) {
            return &OUI_DATABASE[i];
        }
    }
    return nullptr;
}

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
        default: return "Device";
    }
}

const char* getRelevanceName(RelevanceLevel rel) {
    switch(rel) {
        case REL_HIGH: return "HIGH";
        case REL_MEDIUM: return "MEDIUM";
        case REL_LOW: return "LOW";
        default: return "LOW";
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
