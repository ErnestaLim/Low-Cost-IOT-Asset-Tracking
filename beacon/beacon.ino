#include <M5StickCPlus.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>

// Same UUID as tags
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

BLEScan* pBLEScan;
int rssiTag1 = 0, rssiTag2 = 0;
bool tag1Found = false, tag2Found = false;

// Scan callback to detect tags
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && 
        advertisedDevice.getServiceUUID().toString() == SERVICE_UUID) {
      String name = advertisedDevice.getName().c_str();
      if (name == "TAG1") {
        rssiTag1 = advertisedDevice.getRSSI();
        tag1Found = true;
      } else if (name == "TAG2") {
        rssiTag2 = advertisedDevice.getRSSI();
        tag2Found = true;
      }
    }
  }
};

void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.println("BEACON");

  // Initialize BLE
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

void loop() {
  // Reset flags
  tag1Found = false;
  tag2Found = false;

  // Scan for 1 second
  BLEScanResults results = pBLEScan->start(1, false);
  pBLEScan->clearResults();

  // Update display
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.print("TAG1: ");
  if (tag1Found) M5.Lcd.printf("%d", rssiTag1);
  else M5.Lcd.print("---");
  
  M5.Lcd.setCursor(0, 40);
  M5.Lcd.print("TAG2: ");
  if (tag2Found) M5.Lcd.printf("%d", rssiTag2);
  else M5.Lcd.print("---");

  delay(2000);
}