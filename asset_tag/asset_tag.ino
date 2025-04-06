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
#include <esp_gap_ble_api.h>  // for esp_ble_gap_set_security_param
#include <esp_bt_defs.h>


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

class MySecurityCallbacks : public BLESecurityCallbacks {
  void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) {
    if (cmpl.success) {
      Serial.println("Authentication completed.");
      // Check for security status, e.g., auth_mode
      if (cmpl.auth_mode & ESP_LE_AUTH_REQ_SC_ONLY ) {
        Serial.println("SC Authenticated.");
      } else {
        Serial.print("Authentication mode: ");
        Serial.println(cmpl.auth_mode);
        pClient->disconnect();
      }
    } else {
      Serial.println("Authentication failed.");
      pClient->disconnect();
    }
  }
    uint32_t onPassKeyRequest() override {
    Serial.println("Passkey requested.");
    return 123456;
  }

  // Passkey notify callback
  void onPassKeyNotify(uint32_t pass_key) override {
    Serial.print("Passkey received: ");
    Serial.println(pass_key);
  }

  // Security request callback
  bool onSecurityRequest() override {
    Serial.println("Security request received.");
    return true;  // Allow pairing
  }

  // PIN confirmation callback
  bool onConfirmPIN(uint32_t pin) override {
    Serial.print("Confirm PIN: ");
    Serial.println(pin);
    return true;  // Accept the PIN (you can add logic to reject specific PINs)
  }
  
};

bool connectToServer() {
  Serial.println("Connecting to BLE Server...");
  if (!pClient) {
    pClient = BLEDevice::createClient();
  }
  pClient->connect(myDevice);  // Connect to the server
  esp_err_t err = esp_ble_set_encryption((uint8_t*)pClient->getPeerAddress().getNative(), ESP_BLE_SEC_ENCRYPT_MITM);
  esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
  esp_ble_io_cap_t iocap = ESP_IO_CAP_IN;
  uint8_t key_size = 16; // 16-byte encryption key

  esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));

  esp_ble_set_encryption(*(pClient->getPeerAddress().getNative()), ESP_BLE_SEC_ENCRYPT_MITM);

  if (!pClient->isConnected()) {
    Serial.println("Failed to connect.");
    return false;
  }
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

String getWiFiMAC() {
    return WiFi.macAddress();
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
  doc["tag_mac"] = getWiFiMAC().c_str();
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
  doc["signature"] = sha256(payload);
  String payloadwithhash;
  serializeJson(doc, payloadwithhash);
  return payloadwithhash;  
}

// === BLE Setup ===
void setupBLE() {
  BLEDevice::init(TAG_NAME);
  // Client-side GAP security config
  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND); // Bonding + MITM + Secure Conn
  pSecurity->setCapability(ESP_IO_CAP_IN);  // Display onlya
  // pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  pSecurity->setKeySize(16);
  // pSecurity->setStaticPIN(123456); 
  
  BLEDevice::setSecurityCallbacks(new MySecurityCallbacks());
  
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
      if (!pClient->isConnected()) {
        Serial.println("BLE connection lost. Attempting to reconnect...");
        connected = false;
        doConnect = true;
      }
      sendJsonToBeacon(hash);
    }
  }

  delay(scanInterval);
}
