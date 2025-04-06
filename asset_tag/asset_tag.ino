#include <WiFi.h>
#include <M5StickCPlus.h>
#include <esp_wifi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEClient.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "mbedtls/sha256.h"
#include <ArduinoJson.h>

// BLE Settings
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b" // Same for both tags
#define DEVICE_NAME "TAG" // Change to "TAG2" for the second tag
const char* beaconSSID = "BeaconNetwork";

WiFiClient espClient;

BLEServer *pServer;
BLEAdvertising *pAdvertising;

void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.printf("%s", DEVICE_NAME);


// === SHA256 Hash Function ===
String sha256(const String& input) {
  byte hash[32];
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts_ret(&ctx, 0);
  mbedtls_sha256_update_ret(&ctx, (const unsigned char*)input.c_str(), input.length());
  mbedtls_sha256_finish_ret(&ctx, hash);
  mbedtls_sha256_free(&ctx);

  char hashStr[65];
  for (int i = 0; i < 32; i++) {
    sprintf(hashStr + (i * 2), "%02x", hash[i]);
  }
  return String(hashStr);
}

// === WiFi Scan Function ===
String scanWiFi() {
  DynamicJsonDocument doc(1024);
  JsonArray arr = doc.createNestedArray("wifi_data");

  int n = WiFi.scanNetworks(false, true);
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == targetSSID) {
      JsonObject entry = arr.createNestedObject();
      entry["mac"] = WiFi.BSSIDstr(i);
      entry["rssi_data"] = WiFi.RSSI(i);
    }
  }

  if (arr.size() == 0) {
    Serial.println("Target SSID not found.");
    return "";
  }
  String payload;
  serializeJson(doc, payload);
  Serial.println("WiFi JSON Payload: " + payload);
  doc["signature"] = sha256(payload);
  String payloadwithhash;
  serializeJson(doc, payloadwithhash);
  return payloadwithhash;  
}

// === BLE Setup ===
void setupBLE() {
  BLEDevice::init(TAG_NAME);
  pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pService->start();

  // Advertise with service UUID
  pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  // Set advertising interval - each unit is 0.625nanosecond
  pAdvertising->setMinInterval(0x640); // 1second
  pAdvertising->setMaxInterval(0x640); // 1second
}

void loop(){
  WiFi.scanNetworks(true, false, false, 20, 1,beaconSSID);
  int numNetworks = WiFi.scanComplete();
    for (int i = 0; i < numNetworks; i++) {
        String macAddr = WiFi.BSSIDstr(i);  // Get the MAC address
        int rssi = WiFi.RSSI(i);            // Get RSSI value
        Serial.print("Beacon found - MAC: ");
        Serial.print(macAddr);
        Serial.print(" RSSI: ");
        Serial.println(rssi);
    }
    
}