#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer;
BLEService* pService;
BLECharacteristic* pBlevelCharacteristic;
BLECharacteristic* pCBvoltageCharacteristic;
BLECharacteristic* pCBampereCharacteristic;

bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID        "0000180F-0000-1000-8000-00805F9B34FB"
#define BATTERY_LEVEL_CHARACTERISTIC_UUID "00002A19-0000-1000-8000-00805F9B34FB"
#define CUSTOM_BATTERY_VOLTAGE_CHARACTERISTIC_UUID "347BA623-F41A-4B59-A508-DE45079B4F20"
#define CUSTOM_BATTERY_AMPERE_CHARACTERISTIC_UUID "32B4E46D-807F-4E75-ADD7-B08A613E76F3"

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
  pBlevelCharacteristic = pService->createCharacteristic(
                     BATTERY_LEVEL_CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_NOTIFY
                   );
  pBlevelCharacteristic->addDescriptor(new BLE2902());

  // Battery voltage:
  pCBvoltageCharacteristic = pService->createCharacteristic(
                     CUSTOM_BATTERY_VOLTAGE_CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_NOTIFY
                   );
  pCBvoltageCharacteristic->addDescriptor(new BLE2902());

  // Battery Ampere:
  pCBampereCharacteristic = pService->createCharacteristic(
                     CUSTOM_BATTERY_AMPERE_CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_NOTIFY
                   );
  pCBampereCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  Serial.println("GATT Server started");

  pServer->getAdvertising()->start();
}

void loop() {
  if (deviceConnected) {
    // Generate random battery level between 0 and 100
    int batteryLevel = random(0, 101);
    pBlevelCharacteristic->setValue((uint8_t*)&batteryLevel, sizeof(batteryLevel));
    pBlevelCharacteristic->notify();

    delay(random(4000, 7000));

    // Generate random voltage level
    int voltageNoDecimal = random(34, 39);
    float voltageLevel = float(voltageNoDecimal) + static_cast<float>(random(0, 1000)) / 1000.0;
    int voltageInt = static_cast<int>(voltageLevel * 1000.0); // Multiply by 1000 to keep 3 decimal places
    pCBvoltageCharacteristic->setValue((uint8_t*)&voltageInt, sizeof(voltageInt));
    pCBvoltageCharacteristic->notify();

    delay(random(4000, 7000));

    // Generate random Ampere level
    int ampereNoDecimal = random(3, 6);
    float ampereLevel = float(ampereNoDecimal) + static_cast<float>(random(0, 1000)) / 1000.0;
    int ampereInt = static_cast<int>(ampereLevel * 1000.0); // Multiply by 1000 to keep 3 decimal places
    pCBampereCharacteristic->setValue((uint8_t*)&ampereInt, sizeof(ampereInt));
    pCBampereCharacteristic->notify();

    delay(random(4000, 7000));

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