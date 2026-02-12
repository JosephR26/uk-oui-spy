/*
 * ============================================================================
 *  UK-OUI-SPY PRO EDITION - v7.1.0
 *  Professional UK Surveillance Device Detector
 * ============================================================================
 *
 *  Hardware Target: ESP32-2432S028 (CYD - 2.8" ILI9341 TFT + CST820 Touch)
 *
 *  CHANGELOG v7.1:
 *    [x] Embedded Web UI Portal (WiFi AP + Captive Portal)
 *    [x] REST API: /api/detections, /api/config, /api/status, /api/logs
 *    [x] Premium responsive dashboard with live auto-refresh
 *    [x] Radar visualization in browser (Canvas-based)
 *    [x] Remote configuration via web interface
 *    [x] CSV log download from browser
 *    [x] WiFi AP/STA dual mode for simultaneous scanning + serving
 *
 *  REQUIRED LIBRARIES (Install via Library Manager):
 *    - TFT_eSPI by Bodmer (v2.5.43+)
 *    - NimBLE-Arduino by h2zero (v1.4.1+)
 *    - ArduinoJson by Benoit Blanchon (v6.21+)
 *    - ESPAsyncWebServer by me-no-dev (v1.2.3+)
 *    - AsyncTCP by me-no-dev (v1.1.1+)
 *    - TinyGPSPlus by Mikal Hart (v1.0.3+) [optional for GPS]
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
 *    #define SPI_FREQUENCY  40000000
 *    #define SPI_READ_FREQUENCY 16000000
 *    #define LOAD_GLCD
 *    #define LOAD_FONT2
 *    #define LOAD_FONT4
 *    #define SMOOTH_FONT
 *
 *  WEB PORTAL:
 *    Connect to WiFi AP "OUI-SPY-PRO" (password: "spypro2026")
 *    Open browser to http://192.168.4.1
 *
 * ============================================================================
 */

#define VERSION "7.1.0-PRO"

// ============================================================================
//  FEATURE FLAGS
// ============================================================================
#define GPS_ENABLED         false
#define CLOUD_SYNC_ENABLED  false
#define PROMISCUOUS_ENABLED true
#define WEB_PORTAL_ENABLED  true

// ============================================================================
//  WEB PORTAL CONFIGURATION
// ============================================================================
#define AP_SSID     "OUI-SPY-PRO"
#define AP_PASS     "spypro2026"
#define AP_CHANNEL  6
#define AP_MAX_CONN 4

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
#include <ArduinoJson.h>
#include "esp_wifi.h"
#include "esp_sleep.h"
#include "mbedtls/aes.h"

#if WEB_PORTAL_ENABLED
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#endif

#if GPS_ENABLED
#include <TinyGPSPlus.h>
#endif

#if CLOUD_SYNC_ENABLED
#include <HTTPClient.h>
#endif

// ============================================================================
//  HARDWARE PIN DEFINITIONS (ESP32-2432S028)
// ============================================================================
#define TOUCH_ADDR   0x15
#define TFT_BL       21
#define SD_CS        5

#ifdef BOARD_CYD_2USB
// 2-USB CYD variant: capacitive touch (CST820), SDA=33, SCL=32, RST=GPIO 25
#define TOUCH_SDA   33
#define TOUCH_SCL   32
#define TOUCH_RST   25
#else
// Default / original CYD variant (resistive touch)
#define TOUCH_SDA   27
#define TOUCH_SCL   22
#endif

#define LED_R_PIN    4
#define LED_G_PIN    16
#define LED_B_PIN    17
#define LDR_PIN      34
#define BAT_ADC      35
#define GPS_RX_PIN   16
#define GPS_TX_PIN   17

// ============================================================================
//  ENUMERATIONS
// ============================================================================
enum Screen {
    SCREEN_WIZARD, SCREEN_MAIN, SCREEN_RADAR, SCREEN_GRAPH,
    SCREEN_HISTORY, SCREEN_MAP, SCREEN_SETTINGS, SCREEN_INFO
};
enum DeviceCategory {
    CAT_UNKNOWN=0, CAT_CCTV=1, CAT_ANPR=2, CAT_DRONE=3, CAT_BODYCAM=4,
    CAT_CLOUD_CCTV=5, CAT_TRAFFIC=6, CAT_DASH_CAM=7, CAT_DOORBELL_CAM=8,
    CAT_FACIAL_RECOG=9, CAT_PARKING_ENFORCEMENT=10, CAT_SMART_CITY_INFRA=11, CAT_MAX=11
};
enum RelevanceLevel { REL_LOW=0, REL_MEDIUM=1, REL_HIGH=2, REL_MAX=2 };
enum DeploymentType {
    DEPLOY_POLICE=0, DEPLOY_COUNCIL=1, DEPLOY_TRANSPORT=2,
    DEPLOY_RETAIL=3, DEPLOY_PRIVATE=4, DEPLOY_GOVERNMENT=5, DEPLOY_MAX=5
};
enum FilterMode { FILTER_ALL=0, FILTER_HIGH=1, FILTER_MEDIUM=2, FILTER_LOW=3 };

// ============================================================================
//  DATA STRUCTURES
// ============================================================================
struct OUIEntry {
    String oui, manufacturer, notes;
    DeviceCategory category; RelevanceLevel relevance; DeploymentType deployment;
};
struct Detection {
    String macAddress, manufacturer;
    DeviceCategory category; RelevanceLevel relevance;
    int8_t rssi; unsigned long timestamp, firstSeen; bool isBLE;
    int8_t rssiHistory[20]; int rssiHistoryCount;
    double latitude, longitude; bool hasLocation;
    int sightingCount; unsigned long totalDwellTime; bool isStationary; int threatScore;
};
struct Config {
    bool setupComplete=false, enableBLE=true, enableWiFi=true, enablePromiscuous=true;
    bool enableLogging=true, secureLogging=false, autoBrightness=true;
    bool enableGPS=GPS_ENABLED, enableCloudSync=false, enableWebPortal=true;
    int brightness=255, sleepTimeout=300;
    FilterMode filter=FILTER_ALL;
    char encryptionKey[17]="UK-OUI-SPY-2026";
    char cloudEndpoint[128]="https://api.example.com/detections";
};
struct HistoryEntry {
    String mac, manufacturer, category, relevance; int8_t rssi; unsigned long timestamp;
};

// ============================================================================
//  GLOBAL OBJECTS
// ============================================================================
TFT_eSPI tft = TFT_eSPI();
Config config;
Preferences preferences;

#if GPS_ENABLED
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);
#endif

#if WEB_PORTAL_ENABLED
AsyncWebServer webServer(80);
DNSServer dnsServer;
int webClientsConnected = 0;
#endif

std::vector<OUIEntry> dynamicDatabase;
std::unordered_map<String, OUIEntry*> ouiLookup;
std::vector<Detection> detections;
std::vector<HistoryEntry> historyEntries;

SemaphoreHandle_t xDetectionMutex;
SemaphoreHandle_t xHistoryMutex;

Screen currentScreen = SCREEN_WIZARD;
const int MAX_DETECTIONS=100, MAX_HISTORY=50;
unsigned long lastScanTime=0, lastInteraction=0, lastCloudSync=0;
int scanInterval=5000;
bool scanning=false, sdCardAvailable=false, i2cAvailable=false;
float batteryVoltage=0.0;
int wizardStep=0, totalPacketsCaptured=0;
int scrollOffset=0, maxVisibleItems=4, selectedDetectionIdx=-1;
double currentLat=0.0, currentLon=0.0;
bool gpsFixed=false, aggregationActive=false;

// ============================================================================
//  OUI DATABASE ENGINE
// ============================================================================
const char* getCategoryName(DeviceCategory c) {
    const char* n[]={"Unknown","CCTV","ANPR","Drone","Body Cam","Cloud CCTV","Traffic","Dash Cam","Doorbell","Face Recog","Parking","Smart Pole"};
    return (c<=CAT_MAX)?n[c]:"Unknown";
}
const char* getRelevanceName(RelevanceLevel r) {
    const char* n[]={"LOW","MEDIUM","HIGH"}; return (r<=REL_MAX)?n[r]:"?";
}
const char* getDeploymentName(DeploymentType d) {
    const char* n[]={"Police","Council","Transport","Retail","Private","Government"}; return (d<=DEPLOY_MAX)?n[d]:"Unknown";
}
uint16_t getRelevanceColor(RelevanceLevel r) {
    switch(r){case REL_HIGH:return TFT_RED;case REL_MEDIUM:return TFT_YELLOW;default:return TFT_GREEN;}
}
uint16_t getThreatColor(int s) {
    if(s>=80)return TFT_RED;if(s>=50)return TFT_ORANGE;if(s>=30)return TFT_YELLOW;return TFT_GREEN;
}

void rebuildLookupTable() {
    ouiLookup.clear();
    for(auto& e:dynamicDatabase) ouiLookup[e.oui]=&e;
}

bool loadOUIDatabaseFromSD(const char* path) {
    if(!SD.exists(path))return false;
    File file=SD.open(path); if(!file)return false;
    dynamicDatabase.clear();
    if(file.available())file.readStringUntil('\n');
    while(file.available()){
        String line=file.readStringUntil('\n'); line.trim();
        if(line.length()<10)continue;
        const int EF=6; String f[EF]; int fi=0,start=0;
        while(fi<EF-1){int cp=line.indexOf(',',start);if(cp<0)break;f[fi++]=line.substring(start,cp);start=cp+1;}
        if(fi<EF)f[fi++]=line.substring(start);
        if(fi<EF)continue;
        int cv=f[2].toInt(),rv=f[3].toInt(),dv=f[4].toInt();
        if(cv<0||cv>CAT_MAX||rv<0||rv>REL_MAX||dv<0||dv>DEPLOY_MAX)continue;
        OUIEntry e;e.oui=f[0];e.manufacturer=f[1];
        e.category=(DeviceCategory)cv;e.relevance=(RelevanceLevel)rv;
        e.deployment=(DeploymentType)dv;e.notes=f[5];
        dynamicDatabase.push_back(e);
    }
    file.close();
    if(!dynamicDatabase.empty()){rebuildLookupTable();return true;}
    return false;
}

void initializeStaticDatabase() {
    dynamicDatabase.clear();
    auto add=[](const char*o,const char*m,DeviceCategory c,RelevanceLevel r,DeploymentType d,const char*n){
        OUIEntry e;e.oui=o;e.manufacturer=m;e.category=c;e.relevance=r;e.deployment=d;e.notes=n;
        dynamicDatabase.push_back(e);
    };
    add("00:12:12","Hikvision",CAT_CCTV,REL_HIGH,DEPLOY_COUNCIL,"UK CCTV Market Leader");
    add("54:C4:15","Hikvision",CAT_CCTV,REL_HIGH,DEPLOY_COUNCIL,"Hikvision Alt OUI");
    add("C0:56:E3","Hikvision",CAT_CCTV,REL_HIGH,DEPLOY_COUNCIL,"Hikvision Alt OUI");
    add("44:19:B6","Hikvision",CAT_CCTV,REL_HIGH,DEPLOY_COUNCIL,"Hikvision Alt OUI");
    add("00:40:8C","Axis Comms",CAT_CCTV,REL_HIGH,DEPLOY_POLICE,"Swedish CCTV Manufacturer");
    add("AC:CC:8E","Axis Comms",CAT_CCTV,REL_HIGH,DEPLOY_POLICE,"Axis Alt OUI");
    add("B8:A4:4F","Axis Comms",CAT_CCTV,REL_HIGH,DEPLOY_POLICE,"Axis Alt OUI");
    add("E8:27:25","Dahua",CAT_CCTV,REL_HIGH,DEPLOY_COUNCIL,"Chinese CCTV Manufacturer");
    add("3C:EF:8C","Dahua",CAT_CCTV,REL_HIGH,DEPLOY_COUNCIL,"Dahua Alt OUI");
    add("A0:BD:1D","Dahua",CAT_CCTV,REL_HIGH,DEPLOY_COUNCIL,"Dahua Alt OUI");
    add("60:60:1F","DJI",CAT_DRONE,REL_HIGH,DEPLOY_POLICE,"Consumer & Police Drones");
    add("48:1C:B9","DJI",CAT_DRONE,REL_HIGH,DEPLOY_POLICE,"DJI Alt OUI");
    add("34:D2:62","DJI",CAT_DRONE,REL_HIGH,DEPLOY_POLICE,"DJI Alt OUI");
    add("18:65:90","Reolink",CAT_CLOUD_CCTV,REL_MEDIUM,DEPLOY_PRIVATE,"Consumer Cloud CCTV");
    add("9C:8E:CD","Amcrest",CAT_CLOUD_CCTV,REL_MEDIUM,DEPLOY_PRIVATE,"Consumer Cloud CCTV");
    add("FC:65:DE","Ring/Amazon",CAT_DOORBELL_CAM,REL_MEDIUM,DEPLOY_PRIVATE,"Ring Doorbell Camera");
    add("18:B4:30","Nest/Google",CAT_DOORBELL_CAM,REL_MEDIUM,DEPLOY_PRIVATE,"Google Nest Camera");
    add("D8:6C:63","Google",CAT_DOORBELL_CAM,REL_MEDIUM,DEPLOY_PRIVATE,"Google Nest Alt OUI");
    add("68:B6:B3","Genetec",CAT_FACIAL_RECOG,REL_HIGH,DEPLOY_GOVERNMENT,"Facial Recognition Platform");
    add("00:1A:07","Motorola Sol",CAT_BODYCAM,REL_HIGH,DEPLOY_POLICE,"Police Body Camera");
    add("00:18:7D","Aruba/HPE",CAT_SMART_CITY_INFRA,REL_LOW,DEPLOY_COUNCIL,"Smart City WiFi Infra");
    add("B8:27:EB","Raspberry Pi",CAT_UNKNOWN,REL_MEDIUM,DEPLOY_PRIVATE,"Often used in DIY surveillance");
    add("DC:A6:32","Raspberry Pi",CAT_UNKNOWN,REL_MEDIUM,DEPLOY_PRIVATE,"RPi 4 OUI");
    add("E4:5F:01","Raspberry Pi",CAT_UNKNOWN,REL_MEDIUM,DEPLOY_PRIVATE,"RPi 5 OUI");
    add("00:80:F0","Panasonic",CAT_CCTV,REL_HIGH,DEPLOY_TRANSPORT,"Transport CCTV");
    add("00:1B:C5","Vivotek",CAT_CCTV,REL_HIGH,DEPLOY_COUNCIL,"IP Camera Manufacturer");
    add("00:02:D1","Vivotek",CAT_CCTV,REL_HIGH,DEPLOY_COUNCIL,"Vivotek Alt OUI");
    add("00:0F:7C","ACTi",CAT_CCTV,REL_HIGH,DEPLOY_COUNCIL,"IP Camera Manufacturer");
    add("00:1F:54","Samsung Techwin",CAT_CCTV,REL_HIGH,DEPLOY_TRANSPORT,"Hanwha/Samsung CCTV");
    add("00:04:A3","Pelco",CAT_CCTV,REL_HIGH,DEPLOY_GOVERNMENT,"Schneider Electric CCTV");
    add("00:30:53","Bosch Security",CAT_CCTV,REL_HIGH,DEPLOY_GOVERNMENT,"Bosch Security Systems");
    add("00:07:5F","Bosch Security",CAT_CCTV,REL_HIGH,DEPLOY_GOVERNMENT,"Bosch Alt OUI");
    add("00:04:BF","Milestone",CAT_CCTV,REL_HIGH,DEPLOY_POLICE,"VMS Platform");
    add("00:1A:6B","Uniview",CAT_CCTV,REL_HIGH,DEPLOY_COUNCIL,"Chinese CCTV Manufacturer");
    add("24:28:FD","Uniview",CAT_CCTV,REL_HIGH,DEPLOY_COUNCIL,"Uniview Alt OUI");
    add("00:E0:FC","Huawei",CAT_SMART_CITY_INFRA,REL_MEDIUM,DEPLOY_GOVERNMENT,"Smart City Infrastructure");
    add("48:46:FB","Huawei",CAT_SMART_CITY_INFRA,REL_MEDIUM,DEPLOY_GOVERNMENT,"Huawei Alt OUI");
    add("00:30:AB","FLIR",CAT_CCTV,REL_HIGH,DEPLOY_POLICE,"Thermal Imaging");
    add("00:40:7F","Honeywell",CAT_CCTV,REL_HIGH,DEPLOY_GOVERNMENT,"Security Systems");
    add("00:1C:0E","Cisco Meraki",CAT_SMART_CITY_INFRA,REL_LOW,DEPLOY_COUNCIL,"Cloud-Managed WiFi");
    add("AC:17:02","Cisco Meraki",CAT_SMART_CITY_INFRA,REL_LOW,DEPLOY_COUNCIL,"Meraki Alt OUI");
    add("34:56:FE","Cisco Meraki",CAT_SMART_CITY_INFRA,REL_LOW,DEPLOY_COUNCIL,"Meraki Alt OUI");
    add("E0:CB:BC","Cisco Meraki",CAT_SMART_CITY_INFRA,REL_LOW,DEPLOY_COUNCIL,"Meraki Alt OUI");
    rebuildLookupTable();
}

// ============================================================================
//  TOUCH DRIVER (CST820)
// ============================================================================
bool readCapacitiveTouch(uint16_t *x, uint16_t *y) {
    if(!i2cAvailable)return false;
    Wire.beginTransmission(TOUCH_ADDR);Wire.write(0x00);
    if(Wire.endTransmission()!=0)return false;
    if(Wire.requestFrom(TOUCH_ADDR,6)!=6)return false;
    Wire.read();if((Wire.read()&0x0F)==0)return false;
    uint8_t xH=Wire.read(),xL=Wire.read(),yH=Wire.read(),yL=Wire.read();
    uint16_t rawX=((xH&0x0F)<<8)|xL, rawY=((yH&0x0F)<<8)|yL;
    *x=rawY; *y=239-rawX;
    lastInteraction=millis();
    return true;
}

// ============================================================================
//  INTELLIGENCE ENGINE (v7.0)
// ============================================================================
int calculateThreatScore(Detection &d) {
    int s=0;
    if(d.relevance==REL_HIGH)s+=40;else if(d.relevance==REL_MEDIUM)s+=20;else s+=5;
    if(d.rssi>-40)s+=30;else if(d.rssi>-60)s+=20;else if(d.rssi>-80)s+=10;
    if(d.sightingCount>10)s+=20;else if(d.sightingCount>5)s+=15;else if(d.sightingCount>2)s+=10;
    unsigned long dm=d.totalDwellTime/60000;
    if(dm>30)s+=10;else if(dm>10)s+=5;
    if(d.category==CAT_FACIAL_RECOG||d.category==CAT_BODYCAM||d.category==CAT_ANPR)s+=10;
    return constrain(s,0,100);
}
void analyzeDeviceBehavior(Detection &d) {
    if(d.rssiHistoryCount>=5){
        int mn=0,mx=-127;
        for(int i=0;i<d.rssiHistoryCount;i++){if(d.rssiHistory[i]>mx)mx=d.rssiHistory[i];if(d.rssiHistory[i]<mn)mn=d.rssiHistory[i];}
        d.isStationary=(mx-mn)<15;
    }
    d.threatScore=calculateThreatScore(d);
}
int getRecurringDeviceCount(){
    int c=0;xSemaphoreTake(xDetectionMutex,portMAX_DELAY);
    for(auto&d:detections)if(d.sightingCount>3)c++;
    xSemaphoreGive(xDetectionMutex);return c;
}

// ============================================================================
//  PROMISCUOUS MODE (v6.2)
// ============================================================================
#if PROMISCUOUS_ENABLED
void processDetection(String macAddress, int8_t rssi, bool isBLE); // fwd
void promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if(type!=WIFI_PKT_MGMT)return;
    const wifi_promiscuous_pkt_t*pkt=(wifi_promiscuous_pkt_t*)buf;
    const uint8_t*frame=pkt->payload;
    totalPacketsCaptured++;
    if(pkt->rx_ctrl.sig_len<24)return;
    char macStr[18];
    snprintf(macStr,sizeof(macStr),"%02X:%02X:%02X:%02X:%02X:%02X",frame[10],frame[11],frame[12],frame[13],frame[14],frame[15]);
    char ouiStr[9];
    snprintf(ouiStr,sizeof(ouiStr),"%02X:%02X:%02X",frame[10],frame[11],frame[12]);
    String oui(ouiStr);
    auto it=ouiLookup.find(oui);
    if(it!=ouiLookup.end()){
        processDetection(String(macStr),pkt->rx_ctrl.rssi,false);
    }
}
void startPromiscuousMode(){
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(promiscuousCallback);
    static uint8_t ch=1;
    esp_wifi_set_channel(ch,WIFI_SECOND_CHAN_NONE);
    ch=(ch%13)+1;
}
void stopPromiscuousMode(){esp_wifi_set_promiscuous(false);}
#endif

// ============================================================================
//  BLE SCAN CALLBACK
// ============================================================================
void processDetection(String macAddress, int8_t rssi, bool isBLE);
class SurveillanceScanCallbacks:public NimBLEAdvertisedDeviceCallbacks{
    void onResult(NimBLEAdvertisedDevice*device){
        processDetection(device->getAddress().toString().c_str(),device->getRSSI(),true);
    }
};

// ============================================================================
//  SCANNING ENGINE
// ============================================================================
void scanBLE(){
    NimBLEScan*pScan=NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new SurveillanceScanCallbacks(),true);
    pScan->setActiveScan(true);pScan->start(3,false);
}
void scanWiFi(){
    #if PROMISCUOUS_ENABLED
    if(config.enablePromiscuous)stopPromiscuousMode();
    #endif
    int n=WiFi.scanNetworks(false,true,false,150);
    for(int i=0;i<n;++i)processDetection(WiFi.BSSIDstr(i),WiFi.RSSI(i),false);
    WiFi.scanDelete();
    #if PROMISCUOUS_ENABLED
    if(config.enablePromiscuous)startPromiscuousMode();
    #endif
}

void processDetection(String macAddress, int8_t rssi, bool isBLE) {
    String mac=macAddress;mac.toUpperCase();
    String oui=mac.substring(0,8);
    auto it=ouiLookup.find(oui);
    if(it==ouiLookup.end())return;
    Detection det;
    det.macAddress=mac;det.manufacturer=it->second->manufacturer;
    det.category=it->second->category;det.relevance=it->second->relevance;
    det.rssi=rssi;det.timestamp=millis();det.firstSeen=millis();
    det.isBLE=isBLE;det.rssiHistoryCount=0;
    det.latitude=currentLat;det.longitude=currentLon;det.hasLocation=gpsFixed;
    det.sightingCount=1;det.totalDwellTime=0;det.isStationary=false;det.threatScore=0;

    xSemaphoreTake(xDetectionMutex,portMAX_DELAY);
    bool found=false;
    for(auto&d:detections){
        if(d.macAddress==det.macAddress){
            d.rssi=rssi;d.timestamp=millis();d.sightingCount++;
            d.totalDwellTime=millis()-d.firstSeen;
            if(d.rssiHistoryCount<20)d.rssiHistory[d.rssiHistoryCount++]=rssi;
            if(gpsFixed&&!d.hasLocation){d.latitude=currentLat;d.longitude=currentLon;d.hasLocation=true;}
            analyzeDeviceBehavior(d);found=true;break;
        }
    }
    if(!found){
        det.rssiHistory[0]=rssi;det.rssiHistoryCount=1;
        analyzeDeviceBehavior(det);
        detections.insert(detections.begin(),det);
        if((int)detections.size()>MAX_DETECTIONS)detections.pop_back();
    }
    xSemaphoreGive(xDetectionMutex);

    if(config.enableLogging&&sdCardAvailable){
        String logLine=mac+","+String(rssi)+","+det.manufacturer+","+getCategoryName(det.category)+","+getRelevanceName(det.relevance)+","+String(millis());
        if(gpsFixed)logLine+=","+String(currentLat,6)+","+String(currentLon,6);
        if(config.secureLogging){
            mbedtls_aes_context aes;unsigned char key[16];memcpy(key,config.encryptionKey,16);
            int len=logLine.length();int blocks=(len/16)+1;
            unsigned char*inp=(unsigned char*)calloc(blocks*16,1);
            unsigned char*out=(unsigned char*)calloc(blocks*16,1);
            memcpy(inp,logLine.c_str(),len);
            mbedtls_aes_init(&aes);mbedtls_aes_setkey_enc(&aes,key,128);
            for(int b=0;b<blocks;b++)mbedtls_aes_crypt_ecb(&aes,MBEDTLS_AES_ENCRYPT,inp+(b*16),out+(b*16));
            mbedtls_aes_free(&aes);
            File f=SD.open("/secure.log",FILE_APPEND);
            if(f){uint8_t bc=blocks;f.write(&bc,1);f.write(out,blocks*16);f.close();}
            free(inp);free(out);
        }else{
            File f=SD.open("/detections.csv",FILE_APPEND);
            if(f){f.println(logLine);f.close();}
        }
    }
    if(det.relevance==REL_HIGH){
        digitalWrite(LED_R_PIN,HIGH);delay(80);digitalWrite(LED_R_PIN,LOW);
    }
}

// ============================================================================
//  HISTORY LOADER (v6.2)
// ============================================================================
void loadHistoryFromSD(){
    if(!sdCardAvailable)return;
    File f=SD.open("/detections.csv");if(!f)return;
    xSemaphoreTake(xHistoryMutex,portMAX_DELAY);
    historyEntries.clear();int lc=0;
    while(f.available()&&lc<MAX_HISTORY){
        String line=f.readStringUntil('\n');line.trim();if(line.length()<10)continue;
        HistoryEntry he;
        int c1=line.indexOf(',');if(c1<0)continue;he.mac=line.substring(0,c1);
        int c2=line.indexOf(',',c1+1);if(c2<0)continue;he.rssi=line.substring(c1+1,c2).toInt();
        int c3=line.indexOf(',',c2+1);if(c3<0)continue;he.manufacturer=line.substring(c2+1,c3);
        int c4=line.indexOf(',',c3+1);he.category=(c4>0)?line.substring(c3+1,c4):"?";
        historyEntries.push_back(he);lc++;
    }
    f.close();xSemaphoreGive(xHistoryMutex);
}

// ============================================================================
//  FILTERED DETECTIONS (v6.1)
// ============================================================================
std::vector<Detection> getFilteredDetections(){
    xSemaphoreTake(xDetectionMutex,portMAX_DELAY);
    auto all=detections;
    xSemaphoreGive(xDetectionMutex);
    if(config.filter==FILTER_ALL)return all;
    std::vector<Detection> filtered;
    for(auto&d:all){
        if(config.filter==FILTER_HIGH&&d.relevance==REL_HIGH)filtered.push_back(d);
        else if(config.filter==FILTER_MEDIUM&&d.relevance==REL_MEDIUM)filtered.push_back(d);
        else if(config.filter==FILTER_LOW&&d.relevance==REL_LOW)filtered.push_back(d);
    }
    return filtered;
}

// ============================================================================
//  WEB PORTAL - HTML DASHBOARD (stored in PROGMEM)
// ============================================================================
#if WEB_PORTAL_ENABLED
const char WEB_DASHBOARD[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>UK-OUI-SPY PRO</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
:root{--bg:#0a0e17;--card:#111827;--border:#1e293b;--cyan:#06b6d4;--red:#ef4444;--orange:#f97316;--yellow:#eab308;--green:#22c55e;--text:#e2e8f0;--muted:#64748b}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:var(--bg);color:var(--text);min-height:100vh}
.header{background:linear-gradient(135deg,#0f172a,#1e1b4b);padding:16px 20px;border-bottom:1px solid var(--border);display:flex;justify-content:space-between;align-items:center}
.header h1{font-size:18px;color:var(--cyan);font-weight:700;letter-spacing:1px}
.header .status{display:flex;gap:8px;align-items:center}
.badge{padding:3px 8px;border-radius:12px;font-size:11px;font-weight:600}
.badge-scan{background:rgba(6,182,212,0.15);color:var(--cyan)}
.badge-web{background:rgba(34,197,94,0.15);color:var(--green)}
.nav{display:flex;background:#111827;border-bottom:1px solid var(--border);overflow-x:auto}
.nav a{padding:12px 16px;color:var(--muted);text-decoration:none;font-size:13px;font-weight:500;white-space:nowrap;border-bottom:2px solid transparent;transition:all .2s}
.nav a.active,.nav a:hover{color:var(--cyan);border-bottom-color:var(--cyan)}
.content{padding:16px;max-width:960px;margin:0 auto}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:12px;margin-bottom:20px}
.stat{background:var(--card);border:1px solid var(--border);border-radius:10px;padding:14px;text-align:center}
.stat .val{font-size:24px;font-weight:700;color:var(--cyan)}
.stat .lbl{font-size:11px;color:var(--muted);margin-top:4px;text-transform:uppercase;letter-spacing:.5px}
.card{background:var(--card);border:1px solid var(--border);border-radius:10px;padding:16px;margin-bottom:12px}
.card h3{font-size:14px;color:var(--muted);margin-bottom:12px;text-transform:uppercase;letter-spacing:.5px}
.det-row{display:flex;align-items:center;padding:10px;border-radius:8px;margin-bottom:6px;background:rgba(255,255,255,0.02);border-left:3px solid var(--green);transition:background .2s}
.det-row:hover{background:rgba(255,255,255,0.05)}
.det-row.high{border-left-color:var(--red)}
.det-row.medium{border-left-color:var(--yellow)}
.det-info{flex:1}
.det-mfr{font-weight:600;font-size:14px}
.det-meta{font-size:12px;color:var(--muted);margin-top:2px}
.det-right{text-align:right}
.det-threat{font-size:20px;font-weight:700}
.det-rssi{font-size:11px;color:var(--muted)}
.threat-high{color:var(--red)}.threat-med{color:var(--orange)}.threat-low{color:var(--green)}
.toggle-row{display:flex;justify-content:space-between;align-items:center;padding:12px 0;border-bottom:1px solid var(--border)}
.toggle-row:last-child{border-bottom:none}
.toggle-label{font-size:14px}
.toggle{position:relative;width:44px;height:24px;cursor:pointer}
.toggle input{opacity:0;width:0;height:0}
.toggle .slider{position:absolute;inset:0;background:#374151;border-radius:12px;transition:.3s}
.toggle .slider:before{content:'';position:absolute;height:18px;width:18px;left:3px;bottom:3px;background:#fff;border-radius:50%;transition:.3s}
.toggle input:checked+.slider{background:var(--cyan)}
.toggle input:checked+.slider:before{transform:translateX(20px)}
canvas{width:100%;border-radius:10px;background:var(--card);border:1px solid var(--border)}
.filter-bar{display:flex;gap:8px;margin-bottom:16px;flex-wrap:wrap}
.filter-btn{padding:6px 14px;border-radius:16px;border:1px solid var(--border);background:transparent;color:var(--muted);font-size:12px;cursor:pointer;transition:all .2s}
.filter-btn.active{background:var(--cyan);color:#000;border-color:var(--cyan)}
.btn{padding:8px 20px;border-radius:8px;border:none;font-size:13px;font-weight:600;cursor:pointer;transition:all .2s}
.btn-primary{background:var(--cyan);color:#000}.btn-primary:hover{opacity:.8}
.btn-danger{background:var(--red);color:#fff}.btn-danger:hover{opacity:.8}
.footer{text-align:center;padding:20px;color:var(--muted);font-size:11px;border-top:1px solid var(--border);margin-top:20px}
.page{display:none}.page.active{display:block}
@media(max-width:600px){.grid{grid-template-columns:repeat(2,1fr)}.header h1{font-size:15px}}
</style>
</head>
<body>
<div class="header">
<h1>UK-OUI-SPY PRO</h1>
<div class="status">
<span class="badge badge-scan" id="scanBadge">SCANNING</span>
<span class="badge badge-web" id="webBadge">WEB LIVE</span>
</div>
</div>
<nav class="nav">
<a href="#" class="active" onclick="showPage('dashboard',this)">Dashboard</a>
<a href="#" onclick="showPage('detections',this)">Detections</a>
<a href="#" onclick="showPage('radar',this)">Radar</a>
<a href="#" onclick="showPage('config',this)">Settings</a>
<a href="#" onclick="showPage('logs',this)">Logs</a>
<a href="#" onclick="showPage('about',this)">About</a>
</nav>
<div class="content">

<!-- DASHBOARD -->
<div id="dashboard" class="page active">
<div class="grid">
<div class="stat"><div class="val" id="sTotal">0</div><div class="lbl">Detections</div></div>
<div class="stat"><div class="val" id="sHigh" style="color:var(--red)">0</div><div class="lbl">High Threat</div></div>
<div class="stat"><div class="val" id="sPackets">0</div><div class="lbl">Packets</div></div>
<div class="stat"><div class="val" id="sBattery">--%</div><div class="lbl">Battery</div></div>
<div class="stat"><div class="val" id="sMemory">--</div><div class="lbl">Free Memory</div></div>
<div class="stat"><div class="val" id="sUptime">--</div><div class="lbl">Uptime</div></div>
</div>
<div class="card">
<h3>Recent Detections</h3>
<div id="recentList"></div>
</div>
</div>

<!-- DETECTIONS -->
<div id="detections" class="page">
<div class="filter-bar">
<button class="filter-btn active" onclick="setFilter('all',this)">All</button>
<button class="filter-btn" onclick="setFilter('high',this)">High</button>
<button class="filter-btn" onclick="setFilter('medium',this)">Medium</button>
<button class="filter-btn" onclick="setFilter('low',this)">Low</button>
</div>
<div id="detList"></div>
</div>

<!-- RADAR -->
<div id="radar" class="page">
<canvas id="radarCanvas" width="600" height="600"></canvas>
</div>

<!-- CONFIG -->
<div id="config" class="page">
<div class="card">
<h3>Scanning</h3>
<div class="toggle-row"><span class="toggle-label">BLE Scanning</span><label class="toggle"><input type="checkbox" id="cfgBLE" onchange="updateConfig()"><span class="slider"></span></label></div>
<div class="toggle-row"><span class="toggle-label">WiFi Scanning</span><label class="toggle"><input type="checkbox" id="cfgWiFi" onchange="updateConfig()"><span class="slider"></span></label></div>
<div class="toggle-row"><span class="toggle-label">Promiscuous Mode</span><label class="toggle"><input type="checkbox" id="cfgPromisc" onchange="updateConfig()"><span class="slider"></span></label></div>
</div>
<div class="card">
<h3>Logging</h3>
<div class="toggle-row"><span class="toggle-label">SD Card Logging</span><label class="toggle"><input type="checkbox" id="cfgLog" onchange="updateConfig()"><span class="slider"></span></label></div>
<div class="toggle-row"><span class="toggle-label">Encrypted Logs</span><label class="toggle"><input type="checkbox" id="cfgSecure" onchange="updateConfig()"><span class="slider"></span></label></div>
</div>
<div class="card">
<h3>Display</h3>
<div class="toggle-row"><span class="toggle-label">Auto Brightness</span><label class="toggle"><input type="checkbox" id="cfgAutoBr" onchange="updateConfig()"><span class="slider"></span></label></div>
</div>
</div>

<!-- LOGS -->
<div id="logs" class="page">
<div class="card">
<h3>Detection Log</h3>
<p style="color:var(--muted);font-size:13px;margin-bottom:12px">Download the raw CSV detection log from the SD card.</p>
<a href="/api/logs/download" class="btn btn-primary">Download CSV</a>
</div>
<div class="card">
<h3>Recent Log Entries</h3>
<div id="logEntries" style="font-family:monospace;font-size:12px;color:var(--muted);max-height:400px;overflow-y:auto"></div>
</div>
</div>

<!-- ABOUT -->
<div id="about" class="page">
<div class="card">
<h3>Device Information</h3>
<table style="width:100%;font-size:13px">
<tr><td style="color:var(--muted);padding:6px 0">Firmware</td><td id="aFW" style="text-align:right;padding:6px 0">--</td></tr>
<tr><td style="color:var(--muted);padding:6px 0">OUI Database</td><td id="aDB" style="text-align:right;padding:6px 0">--</td></tr>
<tr><td style="color:var(--muted);padding:6px 0">SD Card</td><td id="aSD" style="text-align:right;padding:6px 0">--</td></tr>
<tr><td style="color:var(--muted);padding:6px 0">Touch</td><td id="aTouch" style="text-align:right;padding:6px 0">--</td></tr>
<tr><td style="color:var(--muted);padding:6px 0">WiFi Clients</td><td id="aClients" style="text-align:right;padding:6px 0">--</td></tr>
</table>
</div>
</div>

</div>
<div class="footer">UK-OUI-SPY PRO &middot; Professional Surveillance Detection System</div>

<script>
let allDetections=[],currentFilter='all';
function showPage(id,el){
  document.querySelectorAll('.page').forEach(p=>p.classList.remove('active'));
  document.getElementById(id).classList.add('active');
  document.querySelectorAll('.nav a').forEach(a=>a.classList.remove('active'));
  if(el)el.classList.add('active');
  if(id==='radar')drawRadar();
  if(id==='logs')loadLogs();
}
function setFilter(f,el){
  currentFilter=f;
  document.querySelectorAll('.filter-btn').forEach(b=>b.classList.remove('active'));
  el.classList.add('active');
  renderDetections();
}
function threatClass(s){return s>=60?'threat-high':s>=30?'threat-med':'threat-low'}
function relClass(r){return r==='HIGH'?'high':r==='MEDIUM'?'medium':'low'}
function renderDetRow(d){
  return `<div class="det-row ${relClass(d.relevance)}">
    <div class="det-info"><div class="det-mfr">${d.manufacturer}</div>
    <div class="det-meta">${d.category} &middot; ${d.isBLE?'BLE':'WiFi'} &middot; ${d.rssi} dBm &middot; x${d.sightings} &middot; ${d.stationary?'FIXED':'MOBILE'}</div></div>
    <div class="det-right"><div class="det-threat ${threatClass(d.threat)}">${d.threat}</div><div class="det-rssi">${d.mac.substring(0,8)}</div></div></div>`;
}
function renderDetections(){
  let f=currentFilter==='all'?allDetections:allDetections.filter(d=>d.relevance.toLowerCase()===currentFilter);
  document.getElementById('detList').innerHTML=f.length?f.map(renderDetRow).join(''):'<p style="color:var(--muted);text-align:center;padding:40px">No detections yet...</p>';
}
function drawRadar(){
  const c=document.getElementById('radarCanvas'),ctx=c.getContext('2d');
  const w=c.width,h=c.height,cx=w/2,cy=h/2,r=Math.min(cx,cy)-30;
  ctx.clearRect(0,0,w,h);ctx.fillStyle='#111827';ctx.fillRect(0,0,w,h);
  ctx.strokeStyle='#1e293b';ctx.lineWidth=1;
  for(let i=1;i<=4;i++){ctx.beginPath();ctx.arc(cx,cy,r*i/4,0,Math.PI*2);ctx.stroke();}
  ctx.beginPath();ctx.moveTo(cx-r,cy);ctx.lineTo(cx+r,cy);ctx.moveTo(cx,cy-r);ctx.lineTo(cx,cy+r);ctx.stroke();
  ctx.fillStyle='#06b6d4';ctx.beginPath();ctx.arc(cx,cy,4,0,Math.PI*2);ctx.fill();
  ctx.fillStyle='#64748b';ctx.font='11px sans-serif';ctx.fillText('YOU',cx+8,cy-8);
  allDetections.forEach(d=>{
    let dist=((Math.min(Math.max(d.rssi,-100),-30)+100)/70)*r;
    let hash=0;for(let i=0;i<d.mac.length;i++)hash=hash*31+d.mac.charCodeAt(i);
    let angle=(hash%360)*Math.PI/180;
    let px=cx+dist*Math.cos(angle),py=cy+dist*Math.sin(angle);
    let col=d.threat>=60?'#ef4444':d.threat>=30?'#f97316':'#22c55e';
    ctx.fillStyle=col;ctx.beginPath();ctx.arc(px,py,6,0,Math.PI*2);ctx.fill();
    ctx.strokeStyle=col;ctx.lineWidth=1;ctx.beginPath();ctx.arc(px,py,9,0,Math.PI*2);ctx.stroke();
    ctx.fillStyle='#e2e8f0';ctx.font='10px sans-serif';ctx.fillText(d.manufacturer.substring(0,8),px+12,py+4);
  });
}
async function fetchData(){
  try{
    const[det,st]=await Promise.all([fetch('/api/detections').then(r=>r.json()),fetch('/api/status').then(r=>r.json())]);
    allDetections=det.detections||[];
    document.getElementById('sTotal').textContent=det.total||0;
    document.getElementById('sHigh').textContent=det.highCount||0;
    document.getElementById('sPackets').textContent=st.packets||0;
    document.getElementById('sBattery').textContent=(st.battery||'--')+'%';
    document.getElementById('sMemory').textContent=(st.freeHeap||'--')+' KB';
    document.getElementById('sUptime').textContent=st.uptime||'--';
    document.getElementById('aFW').textContent=st.firmware||'--';
    document.getElementById('aDB').textContent=(st.ouiCount||'--')+' entries';
    document.getElementById('aSD').textContent=st.sdCard?'Ready':'Not Found';
    document.getElementById('aTouch').textContent=st.touch?'OK':'Error';
    document.getElementById('aClients').textContent=st.webClients||0;
    document.getElementById('recentList').innerHTML=allDetections.slice(0,5).map(renderDetRow).join('')||'<p style="color:var(--muted);padding:20px;text-align:center">Scanning...</p>';
    renderDetections();
    if(document.getElementById('radar').classList.contains('active'))drawRadar();
  }catch(e){console.error('Fetch error:',e);}
}
async function loadConfig(){
  try{
    const c=await fetch('/api/config').then(r=>r.json());
    document.getElementById('cfgBLE').checked=c.ble;
    document.getElementById('cfgWiFi').checked=c.wifi;
    document.getElementById('cfgPromisc').checked=c.promiscuous;
    document.getElementById('cfgLog').checked=c.logging;
    document.getElementById('cfgSecure').checked=c.secure;
    document.getElementById('cfgAutoBr').checked=c.autoBrightness;
  }catch(e){}
}
async function updateConfig(){
  const body={ble:document.getElementById('cfgBLE').checked,wifi:document.getElementById('cfgWiFi').checked,
    promiscuous:document.getElementById('cfgPromisc').checked,logging:document.getElementById('cfgLog').checked,
    secure:document.getElementById('cfgSecure').checked,autoBrightness:document.getElementById('cfgAutoBr').checked};
  try{await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});}catch(e){}
}
async function loadLogs(){
  try{
    const data=await fetch('/api/logs').then(r=>r.text());
    document.getElementById('logEntries').innerText=data||'No log data available.';
  }catch(e){document.getElementById('logEntries').innerText='Error loading logs.';}
}
fetchData();loadConfig();setInterval(fetchData,3000);
</script>
</body>
</html>
)rawliteral";

// ============================================================================
//  WEB PORTAL - REST API ENDPOINTS
// ============================================================================
void setupWebServer() {
    // Serve dashboard
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *req){
        req->send_P(200, "text/html", WEB_DASHBOARD);
    });
    // Captive portal redirect
    webServer.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *req){ req->redirect("/"); });
    webServer.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *req){ req->redirect("/"); });
    webServer.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *req){ req->redirect("/"); });

    // API: Get detections
    webServer.on("/api/detections", HTTP_GET, [](AsyncWebServerRequest *req){
        DynamicJsonDocument doc(8192);
        xSemaphoreTake(xDetectionMutex, portMAX_DELAY);
        auto snap = detections;
        xSemaphoreGive(xDetectionMutex);
        doc["total"] = snap.size();
        int highCount = 0;
        JsonArray arr = doc.createNestedArray("detections");
        for (auto &d : snap) {
            if (d.relevance == REL_HIGH) highCount++;
            JsonObject obj = arr.createNestedObject();
            obj["mac"] = d.macAddress;
            obj["manufacturer"] = d.manufacturer;
            obj["category"] = getCategoryName(d.category);
            obj["relevance"] = getRelevanceName(d.relevance);
            obj["rssi"] = d.rssi;
            obj["isBLE"] = d.isBLE;
            obj["threat"] = d.threatScore;
            obj["sightings"] = d.sightingCount;
            obj["stationary"] = d.isStationary;
            if (d.hasLocation) {
                obj["lat"] = d.latitude;
                obj["lon"] = d.longitude;
            }
        }
        doc["highCount"] = highCount;
        String response;
        serializeJson(doc, response);
        req->send(200, "application/json", response);
    });

    // API: Get system status
    webServer.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *req){
        DynamicJsonDocument doc(1024);
        doc["firmware"] = VERSION;
        int bp = constrain(map((int)(batteryVoltage*100), 330, 420, 0, 100), 0, 100);
        doc["battery"] = bp;
        doc["freeHeap"] = ESP.getFreeHeap() / 1024;
        doc["ouiCount"] = dynamicDatabase.size();
        doc["sdCard"] = sdCardAvailable;
        doc["touch"] = i2cAvailable;
        doc["packets"] = totalPacketsCaptured;
        doc["scanning"] = scanning;
        doc["recurring"] = getRecurringDeviceCount();
        unsigned long up = millis() / 1000;
        char upStr[16];
        snprintf(upStr, sizeof(upStr), "%02d:%02d:%02d", (int)(up/3600), (int)((up%3600)/60), (int)(up%60));
        doc["uptime"] = upStr;
        doc["webClients"] = WiFi.softAPgetStationNum();
        #if GPS_ENABLED
        doc["gps"] = gpsFixed;
        doc["lat"] = currentLat;
        doc["lon"] = currentLon;
        #endif
        String response;
        serializeJson(doc, response);
        req->send(200, "application/json", response);
    });

    // API: Get config
    webServer.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *req){
        DynamicJsonDocument doc(512);
        doc["ble"] = config.enableBLE;
        doc["wifi"] = config.enableWiFi;
        doc["promiscuous"] = config.enablePromiscuous;
        doc["logging"] = config.enableLogging;
        doc["secure"] = config.secureLogging;
        doc["autoBrightness"] = config.autoBrightness;
        doc["brightness"] = config.brightness;
        String response;
        serializeJson(doc, response);
        req->send(200, "application/json", response);
    });

    // API: Update config
    webServer.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *req){},
        NULL,
        [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total){
            DynamicJsonDocument doc(512);
            deserializeJson(doc, data, len);
            if (doc.containsKey("ble")) config.enableBLE = doc["ble"];
            if (doc.containsKey("wifi")) config.enableWiFi = doc["wifi"];
            if (doc.containsKey("promiscuous")) config.enablePromiscuous = doc["promiscuous"];
            if (doc.containsKey("logging")) config.enableLogging = doc["logging"];
            if (doc.containsKey("secure")) config.secureLogging = doc["secure"];
            if (doc.containsKey("autoBrightness")) config.autoBrightness = doc["autoBrightness"];
            if (doc.containsKey("brightness")) {
                config.brightness = doc["brightness"];
                analogWrite(TFT_BL, config.brightness);
            }
            saveConfig();
            req->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );

    // API: Get raw logs
    webServer.on("/api/logs", HTTP_GET, [](AsyncWebServerRequest *req){
        if (!sdCardAvailable || !SD.exists("/detections.csv")) {
            req->send(200, "text/plain", "No log data available.");
            return;
        }
        File f = SD.open("/detections.csv");
        if (!f) { req->send(500, "text/plain", "Error reading log."); return; }
        String content = "";
        int lines = 0;
        // Read last 50 lines
        while (f.available() && lines < 50) {
            content += f.readStringUntil('\n') + "\n";
            lines++;
        }
        f.close();
        req->send(200, "text/plain", content);
    });

    // API: Download CSV
    webServer.on("/api/logs/download", HTTP_GET, [](AsyncWebServerRequest *req){
        if (!sdCardAvailable || !SD.exists("/detections.csv")) {
            req->send(404, "text/plain", "No log file found.");
            return;
        }
        req->send(SD, "/detections.csv", "text/csv", true);
    });

    webServer.begin();
    Serial.println("[WEB] Portal started at http://192.168.4.1");
}
#endif

// ============================================================================
//  FREERTOS TASKS
// ============================================================================
void ScanTask(void *pvParameters) {
    for (;;) {
        if (config.setupComplete && (millis()-lastScanTime>=(unsigned long)scanInterval)) {
            scanning = true;
            #if !GPS_ENABLED
            digitalWrite(LED_G_PIN, HIGH);
            #endif
            if (config.enableBLE) scanBLE();
            if (config.enableWiFi) scanWiFi();
            scanning = false;
            #if !GPS_ENABLED
            digitalWrite(LED_G_PIN, LOW);
            #endif
            lastScanTime = millis();
        }
        #if GPS_ENABLED
        while (gpsSerial.available()>0) {
            gps.encode(gpsSerial.read());
            if (gps.location.isValid()) { currentLat=gps.location.lat(); currentLon=gps.location.lng(); gpsFixed=true; }
        }
        #endif
        #if PROMISCUOUS_ENABLED
        if (config.enablePromiscuous && config.setupComplete) startPromiscuousMode();
        #endif
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void UITask(void *pvParameters) {
    for (;;) {
        handleTouchGestures();
        if (config.autoBrightness) {
            static unsigned long lastLDR=0;
            if (millis()-lastLDR>2000) {
                config.brightness=constrain(map(analogRead(LDR_PIN),0,4095,50,255),0,255);
                analogWrite(TFT_BL,config.brightness); lastLDR=millis();
            }
        }
        batteryVoltage=(analogRead(BAT_ADC)/4095.0)*2.0*3.3*1.1;
        updateDisplay();
        #if WEB_PORTAL_ENABLED
        dnsServer.processNextRequest();
        #endif
        if (config.setupComplete && (millis()-lastInteraction>(unsigned long)(config.sleepTimeout*1000))) {
            tft.fillScreen(TFT_BLACK); analogWrite(TFT_BL,0);
            digitalWrite(LED_R_PIN,LOW);
            #if !GPS_ENABLED
            digitalWrite(LED_G_PIN,LOW); digitalWrite(LED_B_PIN,LOW);
            #endif
            esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_SDA,0);
            esp_deep_sleep_start();
        }
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

// ============================================================================
//  UI DRAWING FUNCTIONS (TFT)
// ============================================================================
void drawHeader(const char* title) {
    tft.fillRect(0,0,320,28,0x1082);
    tft.setTextColor(TFT_WHITE);tft.setTextSize(2);tft.setCursor(8,6);tft.print(title);
    int ix=200;
    if(scanning){tft.fillCircle(ix,14,3,TFT_CYAN);ix+=12;}
    if(sdCardAvailable){tft.fillRect(ix,8,7,9,TFT_GREEN);ix+=12;}
    #if WEB_PORTAL_ENABLED
    if(config.enableWebPortal){
        tft.setTextSize(1);tft.setTextColor(TFT_GREEN);tft.setCursor(ix,10);tft.print("WEB");ix+=22;
    }
    #endif
    #if GPS_ENABLED
    tft.setTextSize(1);tft.setTextColor(gpsFixed?TFT_GREEN:0x7BEF);tft.setCursor(ix,10);tft.print("GPS");ix+=22;
    #endif
    tft.drawRect(ix,7,22,11,TFT_WHITE);tft.fillRect(ix+22,10,2,5,TFT_WHITE);
    int bp=constrain(map((int)(batteryVoltage*100),330,420,0,100),0,100);
    int bw=map(bp,0,100,0,18);
    tft.fillRect(ix+2,9,bw,7,bp<20?TFT_RED:(bp<50?TFT_YELLOW:TFT_GREEN));
    tft.setTextSize(1);tft.setCursor(ix+26,10);tft.printf("%d%%",bp);
}
void drawNavbar() {
    tft.fillRect(0,212,320,28,0x0841);tft.drawFastHLine(0,212,320,0x2104);tft.setTextSize(1);
    const char*labels[]={"LIST","RADAR","GRAPH","HIST","MAP","CFG","INFO"};
    int tabW=320/7;
    for(int i=0;i<7;i++){
        bool active=(currentScreen==(Screen)(i+1));
        if(active)tft.fillRoundRect(i*tabW,214,tabW-2,24,3,0x2104);
        tft.setTextColor(active?TFT_CYAN:0x7BEF);
        tft.setCursor(i*tabW+(tabW-strlen(labels[i])*6)/2,222);tft.print(labels[i]);
    }
}
void drawToggle(int x,int y,bool state,const char*label){
    tft.setTextColor(TFT_WHITE);tft.setTextSize(1);tft.setCursor(x,y+5);tft.print(label);
    tft.fillRoundRect(x+155,y,44,20,10,state?0x0410:0x4208);
    tft.fillCircle(state?x+187:x+167,y+10,8,state?TFT_CYAN:0x7BEF);
}
void drawSlider(int x,int y,int w,int v,int mx,const char*l){
    tft.setTextColor(TFT_WHITE);tft.setTextSize(1);tft.setCursor(x,y+2);tft.print(l);
    int bx=x+90,bw=w-90;tft.fillRoundRect(bx,y+4,bw,10,5,0x4208);
    int fw=map(v,0,mx,0,bw);tft.fillRoundRect(bx,y+4,fw,10,5,TFT_CYAN);
    tft.fillCircle(bx+fw,y+9,6,TFT_WHITE);
}

void drawWizardScreen(){
    tft.startWrite();tft.fillScreen(TFT_BLACK);
    tft.fillRect(0,0,320,28,0x1082);tft.setTextColor(TFT_CYAN);tft.setTextSize(2);
    tft.setCursor(10,6);tft.print("UK-OUI-SPY PRO");
    tft.setTextSize(1);tft.setTextColor(TFT_WHITE);
    if(wizardStep==0){
        tft.setCursor(40,55);tft.setTextSize(2);tft.print("WELCOME");tft.setTextSize(1);
        tft.setCursor(20,90);tft.print("Professional Surveillance");
        tft.setCursor(20,105);tft.print("Device Detection System v7.1");
        tft.setCursor(20,135);tft.setTextColor(0xAD55);tft.print("Tap NEXT to begin setup.");
    }else if(wizardStep==1){
        tft.setCursor(40,55);tft.setTextSize(2);tft.print("HW CHECK");tft.setTextSize(1);
        tft.setCursor(20,90);tft.printf("Touch: %s",i2cAvailable?"PASS":"FAIL");
        tft.setCursor(20,105);tft.printf("SD Card: %s",sdCardAvailable?"PASS":"N/A");
        tft.setCursor(20,120);tft.printf("OUI DB: %d entries",dynamicDatabase.size());
        tft.setCursor(20,135);tft.printf("Battery: %.2fV",batteryVoltage);
        #if WEB_PORTAL_ENABLED
        tft.setCursor(20,150);tft.setTextColor(TFT_GREEN);tft.print("Web: 192.168.4.1");
        #endif
    }else{
        tft.setCursor(40,55);tft.setTextSize(2);tft.setTextColor(TFT_GREEN);tft.print("READY");
        tft.setTextSize(1);tft.setTextColor(TFT_WHITE);
        tft.setCursor(20,90);tft.print("All systems operational.");
        tft.setCursor(20,110);tft.print("Device is ready for field use.");
        #if WEB_PORTAL_ENABLED
        tft.setCursor(20,130);tft.setTextColor(TFT_CYAN);
        tft.printf("Web Portal: %s", AP_SSID);
        #endif
        tft.setCursor(20,150);tft.setTextColor(0xAD55);tft.print("Tap GO to start scanning.");
    }
    tft.fillRoundRect(210,170,90,28,6,TFT_CYAN);tft.setTextColor(TFT_BLACK);tft.setTextSize(2);
    tft.setCursor(225,175);tft.print(wizardStep<2?"NEXT":"GO");
    tft.endWrite();
}

void drawMainScreen(){
    tft.startWrite();tft.fillScreen(TFT_BLACK);drawHeader("DETECTIONS");
    const char*fn[]={"ALL","HIGH","MED","LOW"};tft.setTextSize(1);
    for(int i=0;i<4;i++){
        bool a=(config.filter==(FilterMode)i);
        if(a)tft.fillRoundRect(5+i*50,30,46,16,3,0x2104);
        tft.setTextColor(a?TFT_CYAN:0x7BEF);tft.setCursor(12+i*50,35);tft.print(fn[i]);
    }
    auto filtered=getFilteredDetections();
    tft.setTextColor(0x7BEF);tft.setCursor(220,35);tft.printf("%d found",filtered.size());
    if(filtered.empty()){tft.setTextColor(0x7BEF);tft.setCursor(80,120);tft.print("Scanning for devices...");}
    else{
        int y=50,endIdx=min((int)filtered.size(),scrollOffset+maxVisibleItems);
        for(int i=scrollOffset;i<endIdx;i++){
            auto&d=filtered[i];bool sel=(i==selectedDetectionIdx);
            tft.fillRoundRect(3,y,314,38,4,sel?0x2945:0x18C3);
            tft.fillRect(3,y,4,38,getRelevanceColor(d.relevance));
            tft.setTextColor(TFT_WHITE);tft.setTextSize(1);tft.setCursor(12,y+3);tft.print(d.manufacturer);
            tft.setTextColor(d.isBLE?TFT_BLUE:TFT_ORANGE);tft.setCursor(220,y+3);tft.print(d.isBLE?"BLE":"WiFi");
            tft.setTextColor(getThreatColor(d.threatScore));tft.setCursor(260,y+3);tft.printf("T:%d",d.threatScore);
            tft.setTextColor(0xAD55);tft.setCursor(12,y+15);
            tft.printf("%s | %d dBm | x%d",getCategoryName(d.category),d.rssi,d.sightingCount);
            tft.setCursor(12,y+27);tft.setTextColor(0x7BEF);tft.printf("%s",d.isStationary?"FIXED":"MOBILE");
            if(d.hasLocation){tft.setCursor(80,y+27);tft.print("GPS");}
            y+=42;
        }
        if(scrollOffset>0)tft.fillTriangle(305,52,310,48,315,52,TFT_CYAN);
        if(endIdx<(int)filtered.size())tft.fillTriangle(305,205,310,209,315,205,TFT_CYAN);
    }
    drawNavbar();tft.endWrite();
}

void drawRadarScreen(){
    tft.startWrite();tft.fillScreen(TFT_BLACK);drawHeader("RADAR");
    int cx=160,cy=115,r=78;
    for(int i=1;i<=3;i++)tft.drawCircle(cx,cy,(r*i)/3,0x18C3);
    tft.drawLine(cx-r,cy,cx+r,cy,0x18C3);tft.drawLine(cx,cy-r,cx,cy+r,0x18C3);
    tft.setTextColor(0x4208);tft.setTextSize(1);tft.setCursor(cx+(r/3)+2,cy+2);tft.print("NEAR");
    tft.setCursor(cx+r-20,cy+2);tft.print("FAR");tft.fillCircle(cx,cy,3,TFT_CYAN);
    auto filtered=getFilteredDetections();
    for(auto&d:filtered){
        float dist=map(constrain(d.rssi,-100,-30),-100,-30,r,8);
        unsigned long hash=0;for(int c=0;c<(int)d.macAddress.length();c++)hash=hash*31+d.macAddress.charAt(c);
        float angle=(float)(hash%360)*PI/180.0;
        int px=cx+(int)(dist*cos(angle)),py=cy+(int)(dist*sin(angle));
        uint16_t col=getThreatColor(d.threatScore);
        tft.fillCircle(px,py,5,col);tft.drawCircle(px,py,7,col);
    }
    drawNavbar();tft.endWrite();
}

void drawGraphScreen(){
    tft.startWrite();tft.fillScreen(TFT_BLACK);drawHeader("SIGNAL");
    auto filtered=getFilteredDetections();
    if(selectedDetectionIdx>=0&&selectedDetectionIdx<(int)filtered.size()){
        auto&d=filtered[selectedDetectionIdx];
        tft.setTextColor(TFT_WHITE);tft.setTextSize(1);tft.setCursor(10,32);tft.print(d.manufacturer);
        tft.setCursor(10,44);tft.setTextColor(0xAD55);tft.printf("MAC: %s",d.macAddress.c_str());
        int gx=20,gy=60,gw=280,gh=120;tft.drawRect(gx,gy,gw,gh,0x4208);
        tft.setTextColor(0x7BEF);tft.setCursor(0,gy);tft.print("-30");tft.setCursor(0,gy+gh-8);tft.print("-100");
        for(int i=1;i<4;i++)tft.drawFastHLine(gx,gy+(gh*i)/4,gw,0x18C3);
        if(d.rssiHistoryCount>1){
            for(int i=1;i<d.rssiHistoryCount;i++){
                int x1=gx+map(i-1,0,19,0,gw),y1=gy+map(d.rssiHistory[i-1],-30,-100,0,gh);
                int x2=gx+map(i,0,19,0,gw),y2=gy+map(d.rssiHistory[i],-30,-100,0,gh);
                tft.drawLine(x1,y1,x2,y2,TFT_CYAN);tft.fillCircle(x2,y2,2,TFT_CYAN);
            }
        }
        tft.setTextColor(TFT_WHITE);tft.setCursor(10,190);
        tft.printf("Threat: %d  Sightings: %d  %s",d.threatScore,d.sightingCount,d.isStationary?"FIXED":"MOBILE");
    }else{tft.setTextColor(0x7BEF);tft.setTextSize(1);tft.setCursor(30,110);tft.print("Tap a device on LIST to view graph");}
    drawNavbar();tft.endWrite();
}

void drawHistoryScreen(){
    tft.startWrite();tft.fillScreen(TFT_BLACK);drawHeader("HISTORY");
    xSemaphoreTake(xHistoryMutex,portMAX_DELAY);auto snap=historyEntries;xSemaphoreGive(xHistoryMutex);
    if(snap.empty()){tft.setTextColor(0x7BEF);tft.setTextSize(1);tft.setCursor(60,110);tft.print("No history. Insert SD card.");}
    else{int y=32;for(int i=0;i<min((int)snap.size(),8);i++){
        tft.fillRoundRect(3,y,314,20,3,0x18C3);tft.setTextColor(TFT_WHITE);tft.setTextSize(1);
        tft.setCursor(8,y+4);tft.print(snap[i].manufacturer);tft.setTextColor(0xAD55);
        tft.setCursor(160,y+4);tft.printf("%d dBm",snap[i].rssi);tft.setCursor(230,y+4);tft.print(snap[i].category);y+=24;
    }}
    drawNavbar();tft.endWrite();
}

void drawMapScreen(){
    tft.startWrite();tft.fillScreen(TFT_BLACK);drawHeader("MAP");
    int cx=160,cy=115;
    for(int gx=0;gx<320;gx+=40)tft.drawFastVLine(gx,30,180,0x18C3);
    for(int gy=30;gy<210;gy+=30)tft.drawFastHLine(0,gy,320,0x18C3);
    tft.fillCircle(cx,cy,5,TFT_CYAN);tft.setTextColor(TFT_CYAN);tft.setTextSize(1);tft.setCursor(cx-8,cy+8);tft.print("YOU");
    auto filtered=getFilteredDetections();
    for(auto&d:filtered){
        float dist=map(constrain(d.rssi,-100,-30),-100,-30,85,10);
        unsigned long hash=0;for(int c=0;c<(int)d.macAddress.length();c++)hash=hash*31+d.macAddress.charAt(c);
        float angle=(float)(hash%360)*PI/180.0;
        int px=cx+(int)(dist*cos(angle)),py=cy+(int)(dist*sin(angle));
        uint16_t col=getThreatColor(d.threatScore);
        tft.fillRect(px-4,py-4,8,8,col);tft.setTextColor(col);tft.setCursor(px+6,py-3);tft.print(d.manufacturer.substring(0,6));
    }
    drawNavbar();tft.endWrite();
}

void drawSettingsScreen(){
    tft.startWrite();tft.fillScreen(TFT_BLACK);drawHeader("CONFIG");int y=32;
    drawToggle(10,y,config.enableBLE,"BLE Scanning");y+=25;tft.drawFastHLine(10,y-2,300,0x18C3);
    drawToggle(10,y,config.enableWiFi,"WiFi Scanning");y+=25;tft.drawFastHLine(10,y-2,300,0x18C3);
    #if PROMISCUOUS_ENABLED
    drawToggle(10,y,config.enablePromiscuous,"Promiscuous");y+=25;tft.drawFastHLine(10,y-2,300,0x18C3);
    #endif
    drawToggle(10,y,config.enableLogging,"SD Logging");y+=25;tft.drawFastHLine(10,y-2,300,0x18C3);
    drawToggle(10,y,config.secureLogging,"Encrypted");y+=25;tft.drawFastHLine(10,y-2,300,0x18C3);
    drawToggle(10,y,config.autoBrightness,"Auto Bright");y+=25;tft.drawFastHLine(10,y-2,300,0x18C3);
    #if WEB_PORTAL_ENABLED
    drawToggle(10,y,config.enableWebPortal,"Web Portal");y+=25;tft.drawFastHLine(10,y-2,300,0x18C3);
    #endif
    if(!config.autoBrightness){drawSlider(10,y,300,config.brightness,255,"Brightness");y+=25;}
    drawNavbar();tft.endWrite();
}

void drawInfoScreen(){
    tft.startWrite();tft.fillScreen(TFT_BLACK);drawHeader("STATUS");
    tft.setTextSize(1);int y=34,sp=17;char buf[64];
    auto row=[&](const char*l,const char*v,uint16_t c=TFT_WHITE){
        tft.setTextColor(0xAD55);tft.setCursor(10,y);tft.print(l);
        tft.setTextColor(c);tft.setCursor(130,y);tft.print(v);y+=sp;
    };
    row("FIRMWARE",VERSION);
    snprintf(buf,sizeof(buf),"%.2fV",batteryVoltage);row("BATTERY",buf);
    snprintf(buf,sizeof(buf),"%d entries",dynamicDatabase.size());row("OUI DATABASE",buf);
    snprintf(buf,sizeof(buf),"%d KB",ESP.getFreeHeap()/1024);row("FREE MEMORY",buf);
    row("TOUCH",i2cAvailable?"OK":"ERROR",i2cAvailable?TFT_GREEN:TFT_RED);
    row("SD CARD",sdCardAvailable?"READY":"N/A",sdCardAvailable?TFT_GREEN:TFT_RED);
    snprintf(buf,sizeof(buf),"%d",totalPacketsCaptured);row("PACKETS",buf);
    snprintf(buf,sizeof(buf),"%d",getRecurringDeviceCount());row("RECURRING",buf,TFT_ORANGE);
    #if WEB_PORTAL_ENABLED
    snprintf(buf,sizeof(buf),"%d clients",WiFi.softAPgetStationNum());row("WEB PORTAL",buf,TFT_GREEN);
    #endif
    unsigned long up=millis()/1000;
    snprintf(buf,sizeof(buf),"%02d:%02d:%02d",(int)(up/3600),(int)((up%3600)/60),(int)(up%60));row("UPTIME",buf);
    drawNavbar();tft.endWrite();
}

void updateDisplay(){
    switch(currentScreen){
        case SCREEN_WIZARD:drawWizardScreen();break;case SCREEN_MAIN:drawMainScreen();break;
        case SCREEN_RADAR:drawRadarScreen();break;case SCREEN_GRAPH:drawGraphScreen();break;
        case SCREEN_HISTORY:drawHistoryScreen();break;case SCREEN_MAP:drawMapScreen();break;
        case SCREEN_SETTINGS:drawSettingsScreen();break;case SCREEN_INFO:drawInfoScreen();break;
    }
}

// ============================================================================
//  TOUCH HANDLER
// ============================================================================
void handleTouchGestures(){
    static unsigned long lastTouchTime=0;uint16_t x,y;
    if(!readCapacitiveTouch(&x,&y))return;
    if(millis()-lastTouchTime<200)return;lastTouchTime=millis();
    if(currentScreen==SCREEN_WIZARD){
        if(x>210&&y>170&&y<200){wizardStep++;
            if(wizardStep>2){config.setupComplete=true;saveConfig();currentScreen=SCREEN_MAIN;loadHistoryFromSD();}
        }
    }else if(y>212){int tabW=320/7;int idx=x/tabW;if(idx>=0&&idx<=6)currentScreen=(Screen)(idx+1);}
    else if(currentScreen==SCREEN_MAIN){
        if(y>=30&&y<=46){int fi=(x-5)/50;if(fi>=0&&fi<=3){config.filter=(FilterMode)fi;scrollOffset=0;}}
        else if(x>290&&y<60&&scrollOffset>0)scrollOffset--;
        else if(x>290&&y>190){auto f=getFilteredDetections();if(scrollOffset+maxVisibleItems<(int)f.size())scrollOffset++;}
        else if(y>=50&&y<210){int ti=scrollOffset+(y-50)/42;auto f=getFilteredDetections();if(ti<(int)f.size())selectedDetectionIdx=ti;}
    }else if(currentScreen==SCREEN_SETTINGS){
        int baseY=32,step=25,toggleX=165;
        if(x>toggleX&&x<toggleX+44){
            int row=(y-baseY)/step,si=0;
            if(row==si++)config.enableBLE=!config.enableBLE;
            else if(row==si++)config.enableWiFi=!config.enableWiFi;
            #if PROMISCUOUS_ENABLED
            else if(row==si++)config.enablePromiscuous=!config.enablePromiscuous;
            #endif
            else if(row==si++)config.enableLogging=!config.enableLogging;
            else if(row==si++)config.secureLogging=!config.secureLogging;
            else if(row==si++)config.autoBrightness=!config.autoBrightness;
            #if WEB_PORTAL_ENABLED
            else if(row==si++)config.enableWebPortal=!config.enableWebPortal;
            #endif
            saveConfig();
        }
        if(!config.autoBrightness){
            int sliderY=baseY+step*7;
            if(y>=sliderY&&y<=sliderY+18&&x>=100&&x<=310){
                config.brightness=map(x,100,310,0,255);analogWrite(TFT_BL,config.brightness);saveConfig();
            }
        }
    }
}

// ============================================================================
//  PERSISTENT CONFIGURATION (NVS)
// ============================================================================
void saveConfig(){
    preferences.begin("oui-spy",false);
    preferences.putBool("setup",config.setupComplete);preferences.putBool("ble",config.enableBLE);
    preferences.putBool("wifi",config.enableWiFi);preferences.putBool("promisc",config.enablePromiscuous);
    preferences.putBool("log",config.enableLogging);preferences.putBool("secure",config.secureLogging);
    preferences.putBool("autobr",config.autoBrightness);preferences.putBool("cloud",config.enableCloudSync);
    preferences.putBool("webp",config.enableWebPortal);
    preferences.putInt("bright",config.brightness);preferences.putInt("filter",(int)config.filter);
    preferences.end();
}
void loadConfig(){
    preferences.begin("oui-spy",true);
    config.setupComplete=preferences.getBool("setup",false);config.enableBLE=preferences.getBool("ble",true);
    config.enableWiFi=preferences.getBool("wifi",true);config.enablePromiscuous=preferences.getBool("promisc",true);
    config.enableLogging=preferences.getBool("log",true);config.secureLogging=preferences.getBool("secure",false);
    config.autoBrightness=preferences.getBool("autobr",true);config.enableCloudSync=preferences.getBool("cloud",false);
    config.enableWebPortal=preferences.getBool("webp",true);
    config.brightness=preferences.getInt("bright",255);config.filter=(FilterMode)preferences.getInt("filter",0);
    preferences.end();
}

// ============================================================================
//  SETUP & LOOP
// ============================================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n[UK-OUI-SPY] Booting v7.1 PRO Edition...");
    xDetectionMutex=xSemaphoreCreateMutex();
    xHistoryMutex=xSemaphoreCreateMutex();

    // GPIO
    pinMode(LED_R_PIN,OUTPUT);digitalWrite(LED_R_PIN,LOW);
    #if !GPS_ENABLED
    pinMode(LED_G_PIN,OUTPUT);digitalWrite(LED_G_PIN,LOW);
    pinMode(LED_B_PIN,OUTPUT);digitalWrite(LED_B_PIN,LOW);
    #endif
    pinMode(TFT_BL,OUTPUT);digitalWrite(TFT_BL,HIGH);
    pinMode(BAT_ADC,INPUT);

    // I2C & Touch
    Wire.begin(TOUCH_SDA,TOUCH_SCL);
    Wire.beginTransmission(TOUCH_ADDR);
    i2cAvailable=(Wire.endTransmission()==0);

    // Display
    tft.init();tft.setRotation(1);tft.fillScreen(TFT_BLACK);

    // Config
    loadConfig();
    currentScreen=config.setupComplete?SCREEN_MAIN:SCREEN_WIZARD;

    // SD Card
    sdCardAvailable=SD.begin(SD_CS);

    // OUI Database
    if(!loadOUIDatabaseFromSD("/oui.csv")){initializeStaticDatabase();}

    // BLE
    NimBLEDevice::init("UK-OUI-SPY");

    // WiFi: AP+STA dual mode for simultaneous scanning and web serving
    #if WEB_PORTAL_ENABLED
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, 0, AP_MAX_CONN);
    dnsServer.start(53, "*", WiFi.softAPIP());
    setupWebServer();
    Serial.printf("[WEB] AP: %s  IP: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
    #else
    WiFi.mode(WIFI_STA);
    #endif
    WiFi.disconnect();

    // GPS
    #if GPS_ENABLED
    gpsSerial.begin(9600,SERIAL_8N1,GPS_RX_PIN,GPS_TX_PIN);
    #endif

    // Battery
    batteryVoltage=(analogRead(BAT_ADC)/4095.0)*2.0*3.3*1.1;
    lastInteraction=millis();
    if(config.setupComplete)loadHistoryFromSD();

    // FreeRTOS
    xTaskCreatePinnedToCore(ScanTask,"ScanTask",16384,NULL,1,NULL,0);
    xTaskCreatePinnedToCore(UITask,"UITask",16384,NULL,1,NULL,1);

    Serial.println("[UK-OUI-SPY] v7.1 Boot complete. System ready.");
}

void loop(){
    vTaskDelay(pdMS_TO_TICKS(1000));
}
