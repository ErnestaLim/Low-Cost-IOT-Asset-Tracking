#include <esp_now.h>
#include <WiFi.h>

uint8_t tagAddress[] = {0xD4, 0xD4, 0xDA, 0x85, 0x31, 0x28}; // Replace with Tag MAC
int64_t end_time;

typedef struct {
    int64_t send_time;
} esp_now_msg_t;

typedef struct {
    int64_t send_time;
    int64_t processing_time;
} esp_now_response_t;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.println("[MASTER] Packet Sent!");
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    // 🔹Move end_time timestamping after copying the struct
    if (len != sizeof(esp_now_response_t)) {
        Serial.println("[MASTER] Received incorrect response size!");
        return;
    }

    esp_now_response_t response;
    memcpy(&response, incomingData, sizeof(response));
    
    end_time = esp_timer_get_time();  //🔹Timestamp *after* data is copied

    int64_t rtt = end_time - response.send_time; // Round-trip time
    int64_t travel_time = (rtt - response.processing_time) / 2; // One-way time

    Serial.print("[MASTER] RTT: ");
    Serial.print(rtt);
    Serial.println(" µs");

    Serial.print("[MASTER] One-way travel time: ");
    Serial.print(travel_time);
    Serial.println(" µs");

    Serial.print("[MASTER] Processing time: ");
    Serial.print(response.processing_time);
    Serial.println(" µs");
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("[MASTER] ESP-NOW Init Failed");
        return;
    }
    delay(100);
    
    esp_now_peer_info_t peerInfo = {};
    memset(&peerInfo, 0, sizeof(peerInfo));  // Zero out struct
    memcpy(peerInfo.peer_addr, tagAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;  // Disable encryption
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("[MASTER] Failed to add peer");
        return;
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
    esp_now_msg_t msg;
    msg.send_time = esp_timer_get_time(); // Timestamp before sending

    esp_err_t result = esp_now_send(tagAddress, (uint8_t*)&msg, sizeof(msg));
    if (result != ESP_OK) {
        Serial.print("[MASTER] ERROR sending request: ");
        Serial.println(result);
    }
    delay(1000);
}
