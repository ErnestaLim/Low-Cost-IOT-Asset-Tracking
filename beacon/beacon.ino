#include <M5StickCPlus.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// Custom Service UUID
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

BLEServer *pServer;
BLEAdvertising *pAdvertising;

void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.println("BLE Beacon");

  // Initialize BLE
  BLEDevice::init("M5_BEACON");
  
  // Create BLE Server and Service
  pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pService->start();

  // Configure Advertising
  pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
  
  M5.Lcd.setTextSize(1);
  M5.Lcd.println("Advertising started...");
}

void loop() {
  delay(1000);
}