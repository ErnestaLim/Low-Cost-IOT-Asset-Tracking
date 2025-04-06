#include <M5StickCPlus.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEServer.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <BLEUtils.h>
#include "mbedtls/md.h"
#include <vector>
#include <map>
#include <time.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8" 
#define BEACON_NAME "BEACON1" // Change this per beacon

const char* beaconMac;
const char* beaconSSID = "BeaconNetwork";
const char* beaconPassword = "B9503#9v";

const char* ssid = "Yes";
const char* password = "B9503#9v";
const char* mqtt_server = "192.168.20.175";
const char* mqtt_username = "beacon";
const char* mqtt_password = "securepassword";

const char* tagName = "TAG";

// RSSI Path Loss Model Parameters
float Pr_BLE = -78.9;  // RSSI at 1m
float N_BLE = 1.88;   // Path Loss Exponent

float Pr_WIFI = -67.3;  // RSSI at 1m
float N_WIFI = 2.80;   // Path Loss Exponent

const char* ca_cert = R"EOF(
-----BEGIN CERTIFICATE-----
MIID3TCCAsWgAwIBAgIUOlm1yauBIxyVBmsnQ8rg61VJaq4wDQYJKoZIhvcNAQEL
BQAwdjELMAkGA1UEBhMCU0cxEjAQBgNVBAgMCVNpbmdhcG9yZTESMBAGA1UEBwwJ
U2luZ2Fwb3JlMQwwCgYDVQQKDANJb1QxDDAKBgNVBAsMA0lvVDEPMA0GA1UEAwwG
SW9UIENBMRIwEAYJKoZIhvcNAQkBFgNJb1QwHhcNMjUwNDA2MTA1NDEyWhcNMjYw
NDA2MTA1NDEyWjB2MQswCQYDVQQGEwJTRzESMBAGA1UECAwJU2luZ2Fwb3JlMRIw
EAYDVQQHDAlTaW5nYXBvcmUxDDAKBgNVBAoMA0lvVDEMMAoGA1UECwwDSW9UMQ8w
DQYDVQQDDAZJb1QgQ0ExEjAQBgkqhkiG9w0BCQEWA0lvVDCCASIwDQYJKoZIhvcN
AQEBBQADggEPADCCAQoCggEBAL5yfFJ4HQyCR8wGeDr9wk8PXeVWOHOcrs5EgTKv
wEJWR2KsW7jQWjZilDCeQnvBEMi1sWjsvcll12i8TPDAD68v3He/8/XFPjLiOzdC
t6WRUScpU6mFuuWUtbIGVhKDoxpkXBooaQm76oZCWghC7X2oQ/p5oqpe3Pvp+DxY
ON2cJAH1WVJ63DSfcJgp2pK966aG6TyiKMGNFA3ea47GhyNpxfcQqor3flQjUfav
eKzIslkau0FuyFimIOEkqev7oZqvAvLhDMrJy1OligvPYvpdY/hlbfpcvy6co6ma
zrCS5aj10vZVfEmtRNWuS2NbpX5qJ+FjUarCO2a+QTYGhIkCAwEAAaNjMGEwHQYD
VR0OBBYEFARgvRm4qBhQk0S+y2po4aAMSarXMB8GA1UdIwQYMBaAFARgvRm4qBhQ
k0S+y2po4aAMSarXMA8GA1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgGGMA0G
CSqGSIb3DQEBCwUAA4IBAQC5vHU3x9BRidSa86Ab797coHMZ/X+vX1MdvIcTbUYv
mBlrz8/9JIN25+LLvaOiUjf5aa6b40SANw38nvQKMq4/eRttWXid8TRM63yXtI/6
u1zEEvy0mp0AIBufgxvaYAWum085bEb72N368Q94W+5IYKkj45APB3NU9K64cJw9
JMCRagfZl79xjB53hvW96xtAOEdszytXJUHap8ZXJqsQLRnRRaj2p4wc18tBReBn
4anR2C9Bm6tEG1zdqyyxviNLA0LfQuMG57yPq2qpqJC6nGhjgvNR+VhVMAkvhAEw
lD/MJkUjfwQCOBNInOaKL9DAC/GNBSBUcMZzBx2LwSjR
-----END CERTIFICATE-----
)EOF";


WiFiClientSecure espClient;
PubSubClient client(espClient);

BLEServer *pServer;
BLEAdvertising *pAdvertising;
BLEScan* pBLEScan;

std::map<String, int> tagRssiMap;
std::map<String, float> tagDistanceMap;


float bleRssiToDistance(int rssi) {
    return pow(10, (Pr_BLE - rssi) / (10 * N_BLE));
}

float wifiRssiToDistance(int rssi) {
    return pow(10, (Pr_WIFI - rssi) / (10 * N_WIFI));
}

void sendMQTT(const char* beacon, const char* tag,  float distance, const char* channel) { 
    char msg[55];
    snprintf(msg, 50, "%s , %s , %.2f", beacon, tag , distance); 
    Serial.println(msg);
    client.publish(channel, msg);
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

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    Serial.println("Client connected.");
  }

  void onDisconnect(BLEServer* pServer) override {
    Serial.println("Client disconnected. Restart advertising.");
    delay(100);
    pServer->startAdvertising(); // Restart advertising
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.println("Received JSON:");
      Serial.println(value.c_str());

      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, value);
      if (error) {
        Serial.println("Failed to parse JSON");
        return;
      }

      String jsonNoSig;
      if (!doc.containsKey("signature") || !doc.containsKey("wifi_data") || doc["wifi_data"].size() == 0) {
        Serial.println("Missing fields");
        return;
      }

      // Save and temporarily remove signature
      String receivedSignature = doc["signature"];
      doc.remove("signature");
      serializeJson(doc, jsonNoSig);

      String calcSig = calculateSHA256(jsonNoSig);
      if (receivedSignature != calcSig) {
        Serial.println("Signature mismatch!");
        return;
      }

      Serial.println("Signature verified.");
      const char* tagmac = doc["tag_mac"];
      JsonArray rssiArray = doc["wifi_data"];
      for (JsonObject net : rssiArray) {
        const char* mac = net["mac"];
        int rssi = net["rssi_data"];
        float dist = wifiRssiToDistance(rssi);
        sendMQTT(mac,tagmac,dist,"WIFI");
      }
    }
  }
};

class MySecurityCallbacks : public BLESecurityCallbacks {
  uint32_t onPassKeyRequest() {
    Serial.println("Passkey requested.");
    return 123456;  // Use a secure passkey (shared secret)
  }

  void onPassKeyNotify(uint32_t pass_key) {
    Serial.print("Passkey received: ");
    Serial.println(pass_key);
  }

  bool onSecurityRequest() {
    Serial.println("Security request received.");
    return true;  // Allow pairing
  }

  bool onConfirmPIN(uint32_t pin) {
    Serial.print("Confirm PIN: ");
    Serial.println(pin);
    return true;  // Accept the PIN
  }

  // Implement the missing pure virtual method
  void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl) {
    if (auth_cmpl.success) {
      Serial.println(auth_cmpl.auth_mode);
      Serial.println("Authentication successful.");
    } else {
      Serial.println("Authentication failed.");
    }
  }
};

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    String name = advertisedDevice.getName().c_str();
    int signal = advertisedDevice.getRSSI();

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().toString() == SERVICE_UUID) {
      if (name == tagName) {
        String mac = advertisedDevice.getAddress().toString().c_str();
        tagRssiMap[mac] = signal;
        tagDistanceMap[mac] = bleRssiToDistance(signal);
      }
    }
  }
};

const char* getWiFiMAC() {
    static char buf[20];
    
    // Assuming you get the MAC address from some source (e.g., WiFi)
    String macStr = WiFi.softAPmacAddress();
    
    // Convert to C-style string (const char*)
    macStr.toCharArray(buf, sizeof(buf));
    
    return buf;
}

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
        if (client.connect(BEACON_NAME, mqtt_username, mqtt_password)) {
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
    configTime(0, 0, "pool.ntp.org");  // Sync time via NTP
    Serial.println("Waiting for time sync...");
    while (time(nullptr) < 1680600000) {  // Wait until time is synced (after April 2023)
      delay(500);
      Serial.print(".");
    }
    Serial.println("Time synced!");
    espClient.setCACert(ca_cert);
    client.setServer(mqtt_server, 8883);

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(beaconSSID, beaconPassword);

    beaconMac = getWiFiMAC();
    
    Serial.print("Beacon AP started: ");
    Serial.println(beaconSSID);
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());

    BLEDevice::init(BEACON_NAME);

    // **Set security callbacks for BLE pairing**
    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND); 
    pSecurity->setCapability(ESP_IO_CAP_OUT);  // Display only
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    pSecurity->setKeySize(16);
    pSecurity->setStaticPIN(123456); 
    BLEDevice::setSecurityCallbacks(new MySecurityCallbacks());

    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true);
    pBLEScan->setActiveScan(true);

    pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pServer->setCallbacks(new MyServerCallbacks()); 
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ
    );
    pCharacteristic->setCallbacks(new MyCallbacks());

    pService->start();
    
    // Advertise with service UUID
    pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->start();
}

void loop() {
    if (!client.connected()) {
        reconnectMQTT();
    }
    client.loop();

    tagRssiMap.clear();
    tagDistanceMap.clear();

    pBLEScan->clearResults();
    pBLEScan->start(1, false); // 1 second scan

    int y =20;
    for (auto const& entry : tagRssiMap) {
        String mac = entry.first;
        int rssi = entry.second;
        float dist = tagDistanceMap[mac];

        M5.Lcd.setCursor(0, y);
        M5.Lcd.printf(mac.c_str());
        y += 20;
        M5.Lcd.setCursor(0, y);
        M5.Lcd.printf("RSSI:%d Dist:%.2f", rssi, dist);
        y += 20;
        sendMQTT(beaconMac,mac.c_str(), dist,"BLE");
    }

    delay(1000);
}

