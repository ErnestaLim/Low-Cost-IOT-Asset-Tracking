#include "esp_wifi.h"
#include <WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "Yes";
const char* password = "B9503#9v";
const char* tagIP = "192.168.137.33";
const int udpPort = 4210;
const int desiredChannel = 1; 

WiFiUDP udp;

void setupWiFi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n[WiFi] Connected!");
    esp_wifi_set_channel(desiredChannel, WIFI_SECOND_CHAN_NONE);
    udp.begin(udpPort);
}

void setup() {
    Serial.begin(115200);
    setupWiFi();
}

void loop() {
    int64_t start_time = esp_timer_get_time(); // Timestamp before sending

    udp.beginPacket(tagIP, udpPort);
    udp.write((const uint8_t*)"RTT", 3);
    udp.endPacket();

    char buffer[64];
    int64_t end_time = 0;
    int64_t processing_time = 0;

    int packetSize = udp.parsePacket();
    if (packetSize) {
        udp.read(buffer, sizeof(buffer));
        end_time = esp_timer_get_time(); // Timestamp after receiving response

        // Parse processing time from response
        if (strncmp(buffer, "ACK:", 4) == 0) {
            processing_time = atoll(buffer + 4); // Convert to int64_t
        }
    }

    if (end_time > start_time) {
        int64_t rtt = end_time - start_time; // Total RTT
        int64_t travel_time = (rtt - processing_time) / 2; // One-way travel time

        Serial.print("[WiFi] RTT (total round trip): ");
        Serial.print(rtt);
        Serial.println(" µs");

        Serial.print("[WiFi] One-way travel time: ");
        Serial.print(travel_time);
        Serial.println(" µs");
    }

    delay(1000);
}
