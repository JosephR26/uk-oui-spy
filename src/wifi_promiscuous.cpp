#include "wifi_promiscuous.h"

static wifi_promiscuous_callback_t user_callback = nullptr;
static uint8_t current_channel = 1;

// Larger hash cache reduces collisions during high-density scans
static unsigned long detected_macs[256];
static int detected_count = 0;

// djb2-style hash — far better collision resistance than the old hash*256+byte
static unsigned long hashMAC(const uint8_t* mac) {
    unsigned long hash = 5381;
    for (int i = 0; i < 6; i++) {
        hash = ((hash << 5) + hash) ^ mac[i]; // hash * 33 XOR byte
    }
    return hash;
}

// Check if MAC was recently detected (deduplication within a scan cycle)
bool isRecentlyDetected(const uint8_t* mac) {
    unsigned long hash = hashMAC(mac);
    for (int i = 0; i < detected_count; i++) {
        if (detected_macs[i] == hash) return true;
    }
    if (detected_count < 256) {
        detected_macs[detected_count++] = hash;
    } else {
        // Slide oldest entry out, add new at end
        memmove(detected_macs, detected_macs + 1, 255 * sizeof(unsigned long));
        detected_macs[255] = hash;
    }
    return false;
}

// Reset detection cache (called at the start of each full scan cycle)
void resetDetectionCache() {
    detected_count = 0;
}

// ──────────────────────────────────────────────────────────────
// SSID extraction from raw 802.11 frame payload
//
// Management frame MAC header is 24 bytes (3 addresses, no addr4):
//   frame_ctrl(2) + duration(2) + addr1(6) + addr2(6) + addr3(6) + seq_ctrl(2)
//
// Frame body offsets from start of raw payload:
//   Beacon (0x08) / Probe Response (0x05):
//     +24: timestamp(8) + beacon_interval(2) + capability(2) = 12 fixed bytes
//     +36: tagged parameters (TLV)
//   Probe Request (0x04):
//     +24: tagged parameters directly (no fixed header fields)
//
// Tagged parameter format: [tag_id(1)] [tag_len(1)] [data(tag_len)]
// Tag 0 = SSID element
// ──────────────────────────────────────────────────────────────
static void extractSSID(const uint8_t* payload, int pkt_len,
                        uint8_t frame_subtype, char* ssid_out, int ssid_max) {
    ssid_out[0] = '\0';

    // Body offset: probe requests have no fixed fields; beacons/responses have 12
    int body_off = (frame_subtype == 0x04) ? 24 : 36;
    int body_end = pkt_len - 4; // exclude 4-byte FCS at end of frame
    if (body_end <= body_off) return;

    const uint8_t* body = payload + body_off;
    int body_len = body_end - body_off;

    int pos = 0;
    while (pos + 1 < body_len) {
        uint8_t tag_id  = body[pos];
        uint8_t tag_len = body[pos + 1];
        if (pos + 2 + tag_len > body_len) break; // bounds guard
        if (tag_id == 0) { // SSID element
            int copy_len = (tag_len < ssid_max - 1) ? tag_len : ssid_max - 1;
            // Filter non-printable bytes to keep strings clean
            for (int j = 0; j < copy_len; j++) {
                uint8_t c = body[pos + 2 + j];
                ssid_out[j] = (c >= 0x20 && c < 0x7F) ? (char)c : '?';
            }
            ssid_out[copy_len] = '\0';
            return;
        }
        if (tag_len == 0) break; // zero-length tag guard against infinite loop
        pos += 2 + tag_len;
    }
}

// ──────────────────────────────────────────────────────────────
// WiFi promiscuous mode packet callback
// Captures management frames, extracts source MAC + SSID, deduplicates,
// then fires user_callback.
// ──────────────────────────────────────────────────────────────
void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT || user_callback == nullptr) return;

    const wifi_promiscuous_pkt_t    *ppkt = (wifi_promiscuous_pkt_t *)buff;
    const wifi_ieee80211_packet_t   *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t  *hdr  = &ipkt->hdr;

    uint8_t frame_type    = (hdr->frame_ctrl & 0x0C) >> 2;
    uint8_t frame_subtype = (hdr->frame_ctrl & 0xF0) >> 4;

    const uint8_t* source_mac = nullptr;
    char ssid[33] = "";

    if (frame_type == 0) { // Management frame
        switch (frame_subtype) {
            case 0x04: // Probe Request — client looking for APs (reveals client MAC + desired SSID)
            case 0x00: // Association Request
            case 0x02: // Reassociation Request
            case 0x0B: // Authentication
                source_mac = hdr->addr2;
                extractSSID(ppkt->payload, ppkt->rx_ctrl.sig_len, frame_subtype, ssid, sizeof(ssid));
                break;
            case 0x08: // Beacon — AP advertising (reveals AP MAC + SSID)
            case 0x05: // Probe Response
                source_mac = hdr->addr2;
                extractSSID(ppkt->payload, ppkt->rx_ctrl.sig_len, frame_subtype, ssid, sizeof(ssid));
                break;
        }
    }

    if (source_mac != nullptr) {
        // Only unicast MACs (bit 0 of first byte = 0 means unicast)
        if (!(source_mac[0] & 0x01)) {
            if (!isRecentlyDetected(source_mac)) {
                user_callback(source_mac, ppkt->rx_ctrl.rssi, ppkt->rx_ctrl.channel, ssid);
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
    filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
    esp_wifi_set_promiscuous_filter(&filter);

    Serial.println("WiFi promiscuous mode started (all 13 channels)");
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

// ──────────────────────────────────────────────────────────────
// Full 13-channel scan with smart dwell times
//
// Non-overlapping primary channels (1, 6, 11) carry the most real-world
// traffic so they get 2× dwell time. Overlapping secondary channels
// (2-5, 7-10, 12-13) get standard dwell — still fully scanned to catch
// surveillance devices configured on non-standard channels.
//
// Total scan time at dwell_time_ms=150: (3×300) + (10×150) = 2400 ms
// vs the old 3-channel scan: (3×150) = 450 ms
// ──────────────────────────────────────────────────────────────
void scanAllChannels(wifi_promiscuous_callback_t callback, int dwell_time_ms) {
    user_callback = callback;
    resetDetectionCache();

    static const uint8_t channels[] = { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13};
    static const bool    primary[]  = { 1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  1,  0,  0};
    const int num_channels = 13;

    for (int i = 0; i < num_channels; i++) {
        setPromiscuousChannel(channels[i]);
        delay(primary[i] ? dwell_time_ms * 2 : dwell_time_ms);
    }
}
