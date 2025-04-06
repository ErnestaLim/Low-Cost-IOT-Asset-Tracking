#include <M5StickCPlus.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEServer.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <BLEUtils.h>
#include "mbedtls/md.h"
#include <ArduinoJson.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8" 
#define BEACON_NAME "BEACON4" // Change this per beacon

const char* beaconSSID = "BeaconNetwork";
const char* beaconPassword = "B9503#9v";

const char* ssid = "Yes";
const char* password = "B9503#9v";
const char* mqtt_server = "192.168.20.175";
const char* mqtt_username = "beacon";
const char* mqtt_password = "securepassword";

WiFiClient espClient;
PubSubClient client(espClient);

BLEServer *pServer;
BLEAdvertising *pAdvertising;
BLEScan* pBLEScan;

const char* beaconNames[] = {"BEACON1", "BEACON2", "BEACON3", "BEACON4"};
const char* tagName = "TAG";

// Store distances dynamically
int rssi[4] = {0, 0, 0, 0};
float distances[4] = {0, 0, 0, 0};
bool beaconFound[4] = {false, false, false, false};
int tagRssi = 0;
float tagDistance = 0;
bool tagFound = false;

// RSSI Path Loss Model Parameters
float Pr = -69;  // RSSI at 1m
float N = 4.4;   // Path Loss Exponent

float rssiToDistance(int rssi) {
    return pow(10, (Pr - rssi) / (10 * N));
}

void sendMQTT(const char* object, float value) {
    char msg[50];
    snprintf(msg, 50, "%s:%.2f", object, value);
    Serial.println(msg);
    client.publish(BEACON_NAME, msg);
}

String calculateSHA256(const String& input) {
  uint8_t hash[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const unsigned char*)input.c_str(), input.length());
  mbedtls_md_finish(&ctx, hash);
  mbedtls_md_free(&ctx);

  String signature = "";
  for (int i = 0; i < 32; i++) {
    if (hash[i] < 0x10) signature += "0";
    signature += String(hash[i], HEX);
  }
  return signature;
}


class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.println("[BLE] Received JSON:");
      Serial.println(value.c_str());

      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, value);
      if (error) {
        Serial.println("[BLE] Failed to parse JSON");
        return;
      }

      String jsonNoSig;
      if (!doc.containsKey("signature") || !doc.containsKey("wifi_data") || doc["wifi_data"].size() == 0) {
        Serial.println("[BLE] Missing fields");
        return;
      }

      // Save and temporarily remove signature
      String receivedSignature = doc["signature"];
      doc.remove("signature");
      serializeJson(doc, jsonNoSig);

      String calcSig = calculateSHA256(jsonNoSig);
      if (receivedSignature != calcSig) {
        Serial.println("[BLE] Signature mismatch!");
        return;
      }

      Serial.println("[BLE] Signature verified.");

      JsonArray rssiArray = doc["rssi_data"];
      for (JsonObject net : rssiArray) {
        const char* mac = net["mac"];
        int rssi = net["rssi"];
        float dist = rssiToDistance(rssi);

        Serial.printf("[BLE] MAC: %s, RSSI: %d, Distance: %.2f\n", mac, rssi, dist);

        // Optional: send to MQTT
        sendMQTT(mac, dist);
      }
    }
  }
};


// **BLE Scan Callback**
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        String name = advertisedDevice.getName().c_str();
        int signal = advertisedDevice.getRSSI();
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().toString() == SERVICE_UUID) {
          for (int i = 0; i < 4; i++) {
              if (name == beaconNames[i] && name != BEACON_NAME) {
                  rssi[i] = signal;
                  distances[i] = rssiToDistance(signal);
                  beaconFound[i] = true;
              }
          }

          if (name == tagName) {
              tagRssi = signal;
              tagDistance = rssiToDistance(signal);
              tagFound = true;
          }
        }
    }
};

void connectWiFi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected");
}

void reconnectMQTT() {
    while (!client.connected()) {
        if (client.connect(BEACON_NAME)) {
            Serial.println("MQTT Connected");
        } else {
            delay(5000);
        }
    }
}

void setup() {
    M5.begin();
    M5.Lcd.setRotation(3);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.printf("%s", BEACON_NAME);
    Serial.begin(115200);
    
    connectWiFi();
    client.setServer(mqtt_server, 1883);

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(beaconSSID, beaconPassword);

    Serial.print("Beacon AP started: ");
    Serial.println(beaconSSID);
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());

    BLEDevice::init(BEACON_NAME);
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true);
    pBLEScan->setActiveScan(true);

    pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ
    );
    pCharacteristic->setCallbacks(new MyCallbacks());

    pService->start();
    
    // Advertise with service UUID
    pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    // Set advertising interval - each unit is 0.625nanosecond
    pAdvertising->setMinInterval(0x640); // 1second
    pAdvertising->setMaxInterval(0x640); // 1second
    pAdvertising->start();
}

void loop() {
    if (!client.connected()) {
        reconnectMQTT();
    }
    client.loop();
    int pos = 20;
    tagFound = false;
    for (int i = 0; i < 4; i++) beaconFound[i] = false;

    pBLEScan->clearResults();
    pBLEScan->start(1, false);
    for (int i = 0; i < 4; i++) {
      if(beaconFound[i]){
        M5.Lcd.setCursor(0,pos);
        M5.Lcd.setTextSize(2);
        M5.Lcd.print(beaconNames[i]);
        M5.Lcd.printf(":%d", rssi[i]);
        M5.Lcd.printf(",%.2f", distances[i]);
        sendMQTT(beaconNames[i], distances[i]);
        pos += 20;
      }
    }
    if (tagFound){
      M5.Lcd.setCursor(0,80);
      M5.Lcd.setTextSize(2);
      M5.Lcd.print(tagName);
      M5.Lcd.printf(":%d", tagRssi);
      M5.Lcd.printf(",%.2f", tagDistance);
      sendMQTT("TAG", tagDistance);
    }

    delay(1000);
}
