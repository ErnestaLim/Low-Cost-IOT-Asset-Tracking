#include <esp_now.h>
#include <WiFi.h>

uint8_t masterAddress[] = {0xAC, 0x0B, 0xFB, 0x6F, 0x9E, 0x88}; // Replace with Master MAC

typedef struct {
    int64_t send_time;
} esp_now_msg_t;

typedef struct {
    int64_t send_time;
    int64_t processing_time;
} esp_now_response_t;

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    int64_t processing_start = esp_timer_get_time();
    
    if (len != sizeof(esp_now_msg_t)) {
        Serial.println("[TAG] Received incorrect data size!");
        return;
    }

    esp_now_msg_t received_msg;
    memcpy(&received_msg, incomingData, sizeof(received_msg));

    esp_now_response_t response;
    response.send_time = received_msg.send_time;
    
    response.processing_time = esp_timer_get_time() - processing_start; //🔹Ensure minimal processing delay
    
    // 🔹Send the response immediately
    esp_err_t result = esp_now_send(mac, (uint8_t*)&response, sizeof(response));
    if (result != ESP_OK) {
        Serial.print("[TAG] ERROR sending response: ");
        Serial.println(result);
    } else {
        Serial.println("[TAG] Response Sent!");
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("[TAG] ESP-NOW Init Failed");
        return;
    }
    delay(100);

    esp_now_peer_info_t peerInfo = {};
    memset(&peerInfo, 0, sizeof(peerInfo));  // Zero out struct
    memcpy(peerInfo.peer_addr, masterAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;  // Disable encryption

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("[TAG] Failed to add peer");
        return;
    }

    esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
    delay(1000);
}
