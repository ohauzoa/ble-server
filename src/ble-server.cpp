#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

static BLEServer *pServer = NULL;
static BLECharacteristic * pTxCharacteristic;
static bool deviceConnected = false;
static bool oldDeviceConnected = false;
static uint8_t txValue = 0;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks1: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");
      }
    }
};


void bleTask(void *pvParameters)
{
    BLEDevice::init("UART Service");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pTxCharacteristic = pService->createCharacteristic(
                                    CHARACTERISTIC_UUID_TX,
                                    BLECharacteristic::PROPERTY_NOTIFY
                                );                      
    pTxCharacteristic->addDescriptor(new BLE2902());
    BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                                            CHARACTERISTIC_UUID_RX,
                                        BLECharacteristic::PROPERTY_WRITE
                                    );
    pRxCharacteristic->setCallbacks(new MyCallbacks1());
    pService->start();

    /****** Set the service UUID in the advertising  추가된 부분 *********/
    pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
    /******                                                      *********/
    pServer->getAdvertising()->start();
    Serial.println("Waiting a client connection to notify...");


    while(1){
        if (deviceConnected) {
            pTxCharacteristic->setValue(&txValue, 1);
            pTxCharacteristic->notify();
            txValue++;
            delay(10);
        }
        if (!deviceConnected && oldDeviceConnected) {
            delay(500);
            pServer->startAdvertising();
            Serial.println("start advertising");
            oldDeviceConnected = deviceConnected;
        }
        if (deviceConnected && !oldDeviceConnected) {
            oldDeviceConnected = deviceConnected;
        }

    }

}

void bleInit(void)
{
    xTaskCreatePinnedToCore( bleTask, "bleServer", 50000, NULL, 10 | portPRIVILEGE_BIT, NULL, 0);
}
