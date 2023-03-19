#include <Arduino.h>
#include <Wire.h> //Include arduino Wire Library to enable to I2C
#include <tb-h70.h>
#include <zoomer.h>
#include <haptic.h>

#include <Arduino_GFX_Library.h>

#define GFX_BL DF_GFX_BL // default backlight pin, you may replace DF_GFX_BL to actual backlight pin
#define TFT_BL   10  // LED back-light
#define TFT_MOSI 13 // In some display driver board, it might be written as "SDA" and so on.
#define TFT_SCLK 14
#define TFT_CS   9  // Chip select control pin
#define TFT_DC   12  // Data Command control pin
#define TFT_RST  11  // Reset pin (could connect to Arduino RESET pin)

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC /* DC */, TFT_CS /* CS */, TFT_SCLK /* SCK */, TFT_MOSI /* MOSI */, GFX_NOT_DEFINED /* MISO */);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RST /* RST */, 0 /* rotation */, true /* IPS */, 240 /* width */, 240 /* height */, 0 /* col offset 1 */, 0 /* row offset 1 */, 0 /* col offset 2 */, 0 /* row offset 2 */);


#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

static BLEServer *pServer = NULL;
static BLECharacteristic * pTxCharacteristic;
static bool deviceConnected = false;
static bool oldDeviceConnected = false;
static uint8_t txValue = 0;

// Timer variables
static unsigned long lastTime = 0;
static unsigned long ConnectTime = 0;


#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// UART Characteristic and Descriptor
BLECharacteristic uartHapticCharacteristics("cba1d466-344c-4be3-ab3f-189f80dd7518", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor uartHapticDescriptor(BLEUUID((uint16_t)0x2902));
// Temperature Characteristic and Descriptor
BLECharacteristic bmeTemperatureCelsiusCharacteristics("cba1d466-344c-4be3-ab3f-189f80dd7518", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor bmeTemperatureCelsiusDescriptor(BLEUUID((uint16_t)0x2902));
// Humidity Characteristic and Descriptor
BLECharacteristic bmeHumidityCharacteristics("ca73b3ba-39f6-4ab3-91ae-186dc9577d99", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor bmeHumidityDescriptor(BLEUUID((uint16_t)0x2903));




class BLEServerCB : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param) override {
        deviceConnected = true;
        pServer->startAdvertising();
        //BLEDevice::startAdvertising();
    };

} bleServerCB;

// BLE 연결 상태 콜백 함수 
// 연결되었거나 연결이 끊어졌거나
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
//        Serial.println("MyServerCallbacks: deviceConnected");
        pServer->startAdvertising();
        BLEDevice::startAdvertising();
    }
    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

class MyServerCallbacks1: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("MyServerCallbacks: deviceConnected");
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0) {
            ConnectTime = 0;
            Serial.print("Received Value: ");
            for (int i = 0; i < rxValue.length(); i++)
                Serial.print(rxValue[i]);
            Serial.println();

            // Do stuff based on the command received from the app
            if (rxValue.find("Button1Click") != -1) { 
                Serial.println("Button1Click");
                haptic_shake(3, 50);
                stepper.moveTo(5000);
                //digitalWrite(4, HIGH);
            }
            else if (rxValue.find("Button2Click") != -1) {
                Serial.println("Button2Click");
                haptic_shake(5, 150);
                stepper.moveTo(1000);
                //digitalWrite(4, LOW);
            }

        }
    }
};

void checkToReconnect() //added
{
    // disconnected so advertise
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("Disconnected: start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connected so reset boolean control
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        Serial.println("Reconnected");
        oldDeviceConnected = deviceConnected;
    }
}

void setup(void)
{
    Serial.begin(115200);                       // Initialize Serial to log output
    while (!Serial) ;


    gfx->begin();
    gfx->fillScreen(BLACK);

#ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
#endif

    gfx->setCursor(10, 10);
    gfx->setTextColor(RED);
    gfx->println("Hello World!");

    //delay(1000); // 5 seconds
    tb_i2c_h70_Init();
    //bleInit();
    haptic_Init();
    zoomerInit();

    // Create the BLE Device
    BLEDevice::init("UART Service");

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(&bleServerCB);
    //pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);                      
    
    pTxCharacteristic->addDescriptor(new BLE2902());
    BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                                                CHARACTERISTIC_UUID_RX,
                                            BLECharacteristic::PROPERTY_WRITE
                                        );
    pRxCharacteristic->setCallbacks(new MyCallbacks());
    pService->start();

    /****** Set the service UUID in the advertising  추가된 부분 *********/
    //pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
    /******                                                      *********/
    //pServer->getAdvertising()->start();
    //Serial.println("Waiting a client connection to notify...");

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    //pAdvertising->setScanResponse(false);
    //pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
    // // functions that help with iPhone connections issue
    //pAdvertising->setScanResponse(true);
    //pAdvertising->setMinPreferred(0x06);  
    //pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Waiting a client connection to notify...");
    

}

void audioTask(void *pvParameters)
{

}
void audioInit()
{
    xTaskCreatePinnedToCore( audioTask, "audioplay", 5000, NULL, 2 | portPRIVILEGE_BIT, NULL, 0);
}


void loop()
{
    checkToReconnect();
//    gfx->setCursor(random(gfx->width()), random(gfx->height()));
//    gfx->setTextColor(random(0xffff), random(0xffff));
//    gfx->setTextSize(random(6) /* x scale */, random(6) /* y scale */, random(2) /* pixel_margin */);
//    gfx->println("Hello World!");
    if (deviceConnected) {
        String newValue = String((float(get_tbh70())*0.02)-273.15);
        //str = String((float(get_tbh70())*0.02)-273.15);
        Serial.println(newValue);
            gfx->fillRect(80, 100, 80, 30, BLACK);
            gfx->setCursor(80, 100);
            gfx->setTextColor(WHITE);
            gfx->setTextSize(2/* x scale */, 2 /* y scale */, 0 /* pixel_margin */);
            gfx->println(newValue);
        //pTxCharacteristic->setValue(&txValue, 1);
        pTxCharacteristic->setValue(newValue.c_str());
        pTxCharacteristic->notify();
        txValue++;
        //haptic_shake(2, 20);
        delay(100); // bluetooth stack will go into congestion, if too many packets are sent
    }

	//100ms 한번씩 체크
	if(millis() - lastTime > 100) {
		lastTime = millis();
		ConnectTime++;
//		Serial.print("tm: ");
//		Serial.println(ConnectTime);
//        pTxCharacteristic->setValue(&txValue, 1);
//        pTxCharacteristic->notify();
	}
    delay(20); // 1 second
}





