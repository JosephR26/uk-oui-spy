#ifndef WIFI_PROMISCUOUS_H
#define WIFI_PROMISCUOUS_H

#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"

// WiFi packet types
#define WIFI_PKT_MGMT 0x00
#define WIFI_PKT_CTRL 0x01
#define WIFI_PKT_DATA 0x02

// Management frame subtypes
#define WIFI_MGMT_BEACON       0x08
#define WIFI_MGMT_PROBE_REQ    0x04
#define WIFI_MGMT_PROBE_RESP   0x05
#define WIFI_MGMT_ASSOC_REQ    0x00
#define WIFI_MGMT_ASSOC_RESP   0x01

// WiFi frame structure
typedef struct {
    unsigned frame_ctrl:16;
    unsigned duration_id:16;
    uint8_t addr1[6]; // Receiver address
    uint8_t addr2[6]; // Sender address
    uint8_t addr3[6]; // BSSID
    unsigned sequence_ctrl:16;
    uint8_t addr4[6]; // Optional
} wifi_ieee80211_mac_hdr_t;

typedef struct {
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t payload[0]; // Network data
} wifi_ieee80211_packet_t;

// Callback function type
typedef void (*wifi_promiscuous_callback_t)(const uint8_t* addr, int8_t rssi, uint8_t channel);

// Function prototypes
void initWiFiPromiscuous();
void startWiFiPromiscuous(wifi_promiscuous_callback_t callback);
void stopWiFiPromiscuous();
void setPromiscuousChannel(uint8_t channel);
void scanAllChannels(wifi_promiscuous_callback_t callback, int dwell_time_ms);

#endif
