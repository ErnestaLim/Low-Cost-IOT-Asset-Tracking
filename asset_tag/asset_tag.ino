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

// === CONFIGURATIONS ===
const char* targetSSID = "BeaconNetwork";  // WiFi SSID to look for
const char* TAG_NAME = "TAG";          // Unique tag identifier
const int scanInterval = 1000;             // ms between scans

// === BLE UUIDs ===
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer *pServer;
BLEAdvertising *pAdvertising;
BLEScan* pBLEScan;

static BLEAdvertisedDevice* myDevice;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static boolean doConnect = false;
static boolean connected = false;
static BLEClient*  pClient;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
      Serial.println("Found a beacon, will try to connect.");
      BLEDevice::getScan()->stop();  // Stop scanning
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

bool connectToServer() {
  Serial.println("Connecting to BLE Server...");
  pClient = BLEDevice::createClient();
  pClient->connect(myDevice);  // Connect to the server

  BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
  if (pRemoteService == nullptr) {
    Serial.println("Failed to find service.");
    return false;
  }

  pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("Failed to find characteristic.");
    return false;
  }

  connected = true;
  return true;
}

void sendJsonToBeacon(String jsonStr) {
  if (connected && pRemoteCharacteristic->canWrite()) {
    pRemoteCharacteristic->writeValue(jsonStr.c_str(), jsonStr.length());
    Serial.println("Sent JSON via BLE.");
  } else {
    Serial.println("Not connected or cannot write.");
  }
}



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

  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("BLE advertising started.");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.printf("%s", TAG_NAME);
  delay(100);

  setupBLE();
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true);
  pBLEScan->setActiveScan(true);
}

void loop() {
  String hash = scanWiFi();
  if (!hash.isEmpty()) {
    Serial.println("Hashed Data: " + hash);
    if (!connected) {
      Serial.println("scanning");
      pBLEScan->start(2, false); // non-blocking scan
    }
    if (doConnect) {
      if (connectToServer()) {
        Serial.println("Connected to server!");
      } else {
        Serial.println("Failed to connect.");
      }
      doConnect = false;
    }

    if (connected && pRemoteCharacteristic != nullptr) {
      sendJsonToBeacon(hash);
    }
  }

  delay(scanInterval);
}
