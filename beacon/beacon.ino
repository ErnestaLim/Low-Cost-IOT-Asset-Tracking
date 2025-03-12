#include <M5StickCPlus.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEServer.h>

// Same UUID as tags
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define DEVICE_NAME "BEACON3"

BLEServer *pServer;
BLEAdvertising *pAdvertising;
BLEScan* pBLEScan;
int rssiBeacon1 = 0, rssiBeacon2 = 0, rssiBeacon3 = 0, rssiTag1 = 0, rssiTag2 = 0;
bool tag1Found = false, tag2Found = false, beacon1Found = false, beacon2Found = false, beacon3Found = false;

// Scan callback to detect tags
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {;
    if (advertisedDevice.haveServiceUUID() && 
        advertisedDevice.getServiceUUID().toString() == SERVICE_UUID &&
        advertisedDevice.getName().c_str() != DEVICE_NAME) {
      Serial.print("Found device: ");
      Serial.println(advertisedDevice.getName().c_str());
      String name = advertisedDevice.getName().c_str();
      if (name == "TAG1") {
        rssiTag1 = advertisedDevice.getRSSI();
        tag1Found = true;
      } else if (name == "TAG2") {
        rssiTag2 = advertisedDevice.getRSSI();
        tag2Found = true;
      } else if (name == "BEACON1"){
        rssiBeacon1 = advertisedDevice.getRSSI();
        beacon1Found = true;
      } else if (name == "BEACON2"){
        rssiBeacon2 = advertisedDevice.getRSSI();
        beacon2Found = true;
      } else if (name == "BEACON3"){
        rssiBeacon3 = advertisedDevice.getRSSI();
        beacon3Found = true;
      }
    }
  }
};

void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.println(DEVICE_NAME);

  // Initialize BLE
  BLEDevice::init(DEVICE_NAME); // Unique name per tag

  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(),true);
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
  // Reset flags
  tag1Found = false;
  tag2Found = false;
  beacon1Found = false;
  beacon2Found = false;
  beacon3Found = false;

  // Scan for 1 second
  pBLEScan->clearResults();
  Serial.println("Clear");
  Serial.println("");
  pBLEScan->start(1, false);
  // Update display
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.print("BEACON1:");
  if (beacon1Found) M5.Lcd.printf("%d", rssiBeacon1);
  else M5.Lcd.print("---");

  M5.Lcd.setCursor(0, 20);
  M5.Lcd.setTextSize(2);
  M5.Lcd.print("BEACON2:");
  if (beacon2Found) M5.Lcd.printf("%d", rssiBeacon2);
  else M5.Lcd.print("---");

  M5.Lcd.setCursor(0,40);
  M5.Lcd.setTextSize(2);
  M5.Lcd.print("BEACON3:");
  if (beacon3Found) M5.Lcd.printf("%d", rssiBeacon3);
  else M5.Lcd.print("---");

  M5.Lcd.setCursor(0, 60);
  M5.Lcd.setTextSize(2);
  M5.Lcd.print("TAG1: ");
  if (tag1Found) M5.Lcd.printf("%d", rssiTag1);
  else M5.Lcd.print("---");
  
  M5.Lcd.setCursor(0, 80);
  M5.Lcd.print("TAG2: ");
  if (tag2Found) M5.Lcd.printf("%d", rssiTag2);
  else M5.Lcd.print("---");
  delay(100);
}