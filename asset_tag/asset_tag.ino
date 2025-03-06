#include <M5StickCPlus.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>

// Same UUID as beacon
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

BLEScan* pBLEScan;
int rssi = 0;
bool beaconFound = false;

// Scan callback to detect beacon
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && 
        advertisedDevice.getServiceUUID().toString() == SERVICE_UUID) {
      rssi = advertisedDevice.getRSSI();
      beaconFound = true;
    }
  }
};

void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.println("Asset Tag");

  // Initialize BLE
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

void loop() {
  // Start BLE scan
  BLEScanResults foundDevices = pBLEScan->start(1, false);
  pBLEScan->clearResults();

  // Update display
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  
  if (beaconFound) {
    M5.Lcd.printf("RSSI: %d dBm", rssi);
    beaconFound = false;
  } else {
    M5.Lcd.println("No Beacon");
  }

  delay(2000);
}