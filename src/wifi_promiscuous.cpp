#include "wifi_promiscuous.h"

static wifi_promiscuous_callback_t user_callback = nullptr;
static uint8_t current_channel = 1;
static unsigned long detected_macs[100]; // Hash cache to avoid duplicates
static int detected_count = 0;

// Simple hash function for MAC addresses
unsigned long hashMAC(const uint8_t* mac) {
    unsigned long hash = 0;
    for (int i = 0; i < 6; i++) {
        hash = hash * 256 + mac[i];
    }
    return hash;
}

// Check if MAC was recently detected (deduplication)
bool isRecentlyDetected(const uint8_t* mac) {
    unsigned long hash = hashMAC(mac);
    for (int i = 0; i < detected_count; i++) {
        if (detected_macs[i] == hash) {
            return true;
        }
    }

    // Add to cache
    if (detected_count < 100) {
        detected_macs[detected_count++] = hash;
    } else {
        // Rotate buffer
        for (int i = 0; i < 99; i++) {
            detected_macs[i] = detected_macs[i + 1];
        }
        detected_macs[99] = hash;
    }

    return false;
}

// Reset detection cache (call periodically)
void resetDetectionCache() {
    detected_count = 0;
}

// WiFi promiscuous mode packet callback
void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT || user_callback == nullptr) {
        return;
    }

    const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

    // Extract frame control info
    uint8_t frame_type = (hdr->frame_ctrl & 0x0C) >> 2;
    uint8_t frame_subtype = (hdr->frame_ctrl & 0xF0) >> 4;

    // We're interested in management frames that contain MAC addresses
    // Probe requests, beacons, association requests reveal device MACs
    const uint8_t* source_mac = nullptr;

    if (frame_type == 0) { // Management frame
        switch (frame_subtype) {
            case 0x04: // Probe Request - addr2 is source (client MAC)
            case 0x00: // Association Request - addr2 is source
            case 0x02: // Reassociation Request - addr2 is source
            case 0x0B: // Authentication - addr2 is source
                source_mac = hdr->addr2;
                break;
            case 0x08: // Beacon - addr2 is BSSID (AP MAC)
            case 0x05: // Probe Response - addr2 is BSSID
                source_mac = hdr->addr2;
                break;
        }
    }

    if (source_mac != nullptr) {
        // Check if this is a valid MAC (not broadcast/multicast)
        if (!(source_mac[0] & 0x01)) {
            // Deduplicate
            if (!isRecentlyDetected(source_mac)) {
                int8_t rssi = ppkt->rx_ctrl.rssi;
                user_callback(source_mac, rssi, ppkt->rx_ctrl.channel);
            }
        }
    }
}

void initWiFiPromiscuous() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(nullptr);
}

void startWiFiPromiscuous(wifi_promiscuous_callback_t callback) {
    user_callback = callback;

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);

    wifi_promiscuous_filter_t filter;
    filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT; // Only management frames
    esp_wifi_set_promiscuous_filter(&filter);

    Serial.println("WiFi promiscuous mode started");
}

void stopWiFiPromiscuous() {
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(nullptr);
    user_callback = nullptr;
    Serial.println("WiFi promiscuous mode stopped");
}

void setPromiscuousChannel(uint8_t channel) {
    if (channel >= 1 && channel <= 13) {
        current_channel = channel;
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    }
}

void scanAllChannels(wifi_promiscuous_callback_t callback, int dwell_time_ms) {
    user_callback = callback;

    // Reset detection cache for fresh scan
    resetDetectionCache();

    // Scan channels 1, 6, 11 (common non-overlapping channels)
    // For quick scan, or all 1-13 for thorough scan
    uint8_t channels[] = {1, 6, 11};
    int num_channels = 3;

    // For more thorough scanning, uncomment:
    // uint8_t channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    // int num_channels = 13;

    for (int i = 0; i < num_channels; i++) {
        setPromiscuousChannel(channels[i]);
        delay(dwell_time_ms);
    }
}
