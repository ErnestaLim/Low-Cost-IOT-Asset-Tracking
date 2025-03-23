#include "esp_wifi.h"
#include <WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "Yes";
const char* password = "B9503#9v";
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
    int packetSize = udp.parsePacket();
    if (packetSize) {
        int64_t recv_time = esp_timer_get_time();
        char buffer[64];
        udp.read(buffer, sizeof(buffer));
        if (strncmp(buffer, "RTT", 3) == 0) {
           // When packet is received
            
            // Respond immediately
            udp.beginPacket(udp.remoteIP(), udp.remotePort());
            int64_t response_start = esp_timer_get_time(); // When response starts
            int64_t processing_time = response_start - recv_time; // Calculate processing time
            char response[32];
            sprintf(response, "ACK:%lld", processing_time); // Send ACK + processing time
            udp.write((const uint8_t*)response,sizeof(response));
            udp.endPacket();

            Serial.print("[Tag] Processing Time: ");
            Serial.print(processing_time);
            Serial.println(" µs");
        }
    }
}
