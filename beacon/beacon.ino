#include <M5StickCPlus.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEServer.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BEACON_NAME "BEACON2" // Change this per beacon

const char* ssid = "Yes";
const char* password = "B9503#9v";
const char* mqtt_server = "192.168.20.175";

WiFiClient espClient;
PubSubClient client(espClient);

BLEServer *pServer;
BLEAdvertising *pAdvertising;
BLEScan* pBLEScan;

const char* beaconNames[] = {"BEACON1", "BEACON2", "BEACON3"};
const char* tagName = "TAG";

// Store distances dynamically
int rssi[3] = {0, 0, 0};
float distances[3] = {0, 0, 0};
bool beaconFound[3] = {false, false, false};
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

// **BLE Scan Callback**
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        String name = advertisedDevice.getName().c_str();
        int signal = advertisedDevice.getRSSI();
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().toString() == SERVICE_UUID) {
          for (int i = 0; i < 3; i++) {
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

    BLEDevice::init(BEACON_NAME);
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true);
    pBLEScan->setActiveScan(true);

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

void loop() {
    if (!client.connected()) {
        reconnectMQTT();
    }
    client.loop();
    int pos = 20;
    tagFound = false;
    for (int i = 0; i < 3; i++) beaconFound[i] = false;

    pBLEScan->clearResults();
    pBLEScan->start(1, false);
    for (int i = 0; i < 3; i++) {
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
      M5.Lcd.setCursor(0,60);
      M5.Lcd.setTextSize(2);
      M5.Lcd.print(tagName);
      M5.Lcd.printf(":%d", tagRssi);
      M5.Lcd.printf(",%.2f", tagDistance);
      sendMQTT("TAG", tagDistance);
    }

    delay(1000);
}
