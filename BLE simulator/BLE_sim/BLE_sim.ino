#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer;
BLEService* pService;
BLECharacteristic* pBlevelCharacteristic;
BLECharacteristic* pCBvoltageCharacteristic;
BLECharacteristic* pCBampereCharacteristic;
BLECharacteristic* pCTextCharacteristic;

bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID        "0000180F-0000-1000-8000-00805F9B34FB"
#define BATTERY_LEVEL_CHARACTERISTIC_UUID "00002A19-0000-1000-8000-00805F9B34FB"
#define CUSTOM_BATTERY_VOLTAGE_CHARACTERISTIC_UUID "347BA623-F41A-4B59-A508-DE45079B4F20"
#define CUSTOM_BATTERY_AMPERE_CHARACTERISTIC_UUID "32B4E46D-807F-4E75-ADD7-B08A613E76F3"
#define CUSTOM_BATTERY_AMPERE_CHARACTERISTIC_UUID "32B4E46D-807F-4E75-ADD7-B08A613E76F3"
#define CUSTOM_BYTE_ARRAY_CHARACTERISTIC_UUID "7AC7B653-6E94-4976-A5DB-9253BDDD5727"

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

  // Text:
  pCTextCharacteristic = pService->createCharacteristic(
                     CUSTOM_BYTE_ARRAY_CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_NOTIFY
                   );
  pCTextCharacteristic->addDescriptor(new BLE2902());

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

    delay(random(1000, 4000));

    // Generate random voltage level
    int voltageNoDecimal = random(34, 39);
    float voltageLevel = float(voltageNoDecimal) + static_cast<float>(random(0, 1000)) / 1000.0;
    int voltageInt = static_cast<int>(voltageLevel * 1000.0); // Multiply by 1000 to keep 3 decimal places
    pCBvoltageCharacteristic->setValue((uint8_t*)&voltageInt, sizeof(voltageInt));
    pCBvoltageCharacteristic->notify();

    delay(random(1000, 4000));

    // Generate random Ampere level
    int ampereNoDecimal = random(3, 6);
    float ampereLevel = float(ampereNoDecimal) + static_cast<float>(random(0, 1000)) / 1000.0;
    int ampereInt = static_cast<int>(ampereLevel * 1000.0); // Multiply by 1000 to keep 3 decimal places
    pCBampereCharacteristic->setValue((uint8_t*)&ampereInt, sizeof(ampereInt));
    pCBampereCharacteristic->notify();

    delay(random(1000, 4000));

    // Prepare battery information JSON
    DynamicJsonDocument jsonDoc(1024); // Assuming 512 bytes is sufficient
    JsonObject batteryInfo = jsonDoc.createNestedObject("battery_info");
    batteryInfo["manufacturer"] = "ABC Batteries Inc.";
    batteryInfo["model"] = "BLi30";
    batteryInfo["capacity_Ah"] = 7.7;
    batteryInfo["voltage_V"] = 36;
    batteryInfo["chemistry"] = "Li-Ion";
    batteryInfo["status"] = "Good";
    batteryInfo["charge_cycles"] = 150;
    batteryInfo["health"] = "Excellent";
    batteryInfo["temperature_C"] = 25.5;
    batteryInfo["charging"] = false;
    batteryInfo["charging_method"] = "Unkown";
    batteryInfo["last_charge_time"] = "2024-04-11T15:30:00Z";
    batteryInfo["estimated_runtime_minutes"] = 240;

    JsonObject additionalInfo = batteryInfo.createNestedObject("additional_info");
    additionalInfo["serial_number"] = "ABC123456789";
    additionalInfo["manufacturing_date"] = "2024-03-01";
    additionalInfo["country_of_origin"] = "Sweden";

    JsonArray features = additionalInfo.createNestedArray("features");
    features.add("overcharge protection");
    features.add("temperature control");

    size_t jsonSize = measureJson(jsonDoc);
    Serial.println("JSON size: " + String(jsonSize) + " byte");

    String jsonString;
    serializeJson(jsonDoc, jsonString);

    pCTextCharacteristic->setValue((uint8_t*)jsonString.c_str(), jsonString.length());
    pCTextCharacteristic->notify();
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