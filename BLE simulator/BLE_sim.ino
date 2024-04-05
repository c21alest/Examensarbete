#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer;
BLEService* pService;
BLECharacteristic* pBatteryCharacteristic;
BLECharacteristic* pCustomCharacteristic;

bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID        "0000180F-0000-1000-8000-00805F9B34FB"
#define BATTERY_LEVEL_CHARACTERISTIC_UUID "00002A19-0000-1000-8000-00805F9B34FB"
#define CUSTOM_BATTERY_VOLTAGE_CHARACTERISTIC_UUID "347BA623-F41A-4B59-A508-DE45079B4F20"

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void setup() {
  Serial.begin(115200);

  BLEDevice::init("BLE Battery Demo");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  pService = pServer->createService(SERVICE_UUID);

  // Battery level:
  pBatteryCharacteristic = pService->createCharacteristic(
                     BATTERY_LEVEL_CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_NOTIFY
                   );
  pBatteryCharacteristic->addDescriptor(new BLE2902());

  // Battery voltage:
  pCustomCharacteristic = pService->createCharacteristic(
                     CUSTOM_BATTERY_VOLTAGE_CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_NOTIFY
                   );
  pCustomCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  Serial.println("GATT Server started");

  pServer->getAdvertising()->start();
}

void loop() {
  if (deviceConnected) {
    // Generate random battery level between 0 and 100
    int batteryLevel = random(0, 101);
    pBatteryCharacteristic->setValue((uint8_t*)&batteryLevel, sizeof(batteryLevel));
    pBatteryCharacteristic->notify();

    // Generate random voltage level between 35 and 36 with 3 decimal places
    float voltageLevel = 35.0 + static_cast<float>(random(0, 1000)) / 1000.0;
    int voltageInt = static_cast<int>(voltageLevel * 1000.0); // Multiply by 1000 to keep 3 decimal places
    pCustomCharacteristic->setValue((uint8_t*)&voltageInt, sizeof(voltageInt));
    pCustomCharacteristic->notify();

    delay(5000); // Update every 5 seconds
  }

  // If new connection
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("Start advertising");
    oldDeviceConnected = deviceConnected;
  }

  // Store connection for next loop
  oldDeviceConnected = deviceConnected;
}
