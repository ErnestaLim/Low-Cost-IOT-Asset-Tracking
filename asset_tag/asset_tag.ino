#include <M5StickCPlus.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// BLE Settings
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b" // Same for both tags
#define DEVICE_NAME "TAG" // Change to "TAG2" for the second tag

BLEServer *pServer;
BLEAdvertising *pAdvertising;

void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.printf("%s", DEVICE_NAME);

  // Initialize BLE
  BLEDevice::init(DEVICE_NAME); // Unique name per tag
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
}