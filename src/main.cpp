#include <Arduino.h>
#include <Wire.h> //Include arduino Wire Library to enable to I2C
#include <tb-h70.h>
#include <screen.h>
#include <haptic.h>
#include <zoomer.h>

#include "FS.h"
#include <LittleFS.h>
#include <time.h>
#define FORMAT_LITTLEFS_IF_FAILED true

#include <SPIFFSIniFile.h>


#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

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
        String s = (char*)pCharacteristic->getData();
        Serial.println("rx:" + s);  // 이게 데이터!

        // Do stuff based on the command received from the app
        if(s.charAt(0) == 'B'){ 
            String val = s.substring(2, 3);
            if(val.toInt() == 1){
                Serial.println("Button1Click");
                haptic_shake(3, 50);
                stepper.moveTo(2000);
            }
            else if(val.toInt() == 2){
                Serial.println("Button2Click");
                haptic_shake(5, 80);
                stepper.moveTo(1000);
            }
        }
    }
};

class MyCallbacks_org: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();
        String s = (char*)pCharacteristic->getData();
Serial.println("rx:" + s);  // 이게 데이터!
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
////////////////////////////////////////////////////////////////////////////////////
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");

            Serial.print(file.name());
            time_t t= file.getLastWrite();
            struct tm * tmstruct = localtime(&t);
            Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n",(tmstruct->tm_year)+1900,( tmstruct->tm_mon)+1, tmstruct->tm_mday,tmstruct->tm_hour , tmstruct->tm_min, tmstruct->tm_sec);

            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");

            Serial.print(file.size());
            time_t t= file.getLastWrite();
            struct tm * tmstruct = localtime(&t);
            Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n",(tmstruct->tm_year)+1900,( tmstruct->tm_mon)+1, tmstruct->tm_mday,tmstruct->tm_hour , tmstruct->tm_min, tmstruct->tm_sec);
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char * path){
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char * path){
    String s;
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while(file.available()){
        //s = (char*)file.read();
        //Serial.println("file:" + s);
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("- file renamed");
    } else {
        Serial.println("- rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}

// SPIFFS-like write and delete file

// See: https://github.com/esp8266/Arduino/blob/master/libraries/LittleFS/src/LittleFS.cpp#L60
void writeFile2(fs::FS &fs, const char * path, const char * message){
    if(!fs.exists(path)){
		if (strchr(path, '/')) {
            Serial.printf("Create missing folders of: %s\r\n", path);
			char *pathStr = strdup(path);
			if (pathStr) {
				char *ptr = strchr(pathStr, '/');
				while (ptr) {
					*ptr = 0;
					fs.mkdir(pathStr);
					*ptr = '/';
					ptr = strchr(ptr+1, '/');
				}
			}
			free(pathStr);
		}
    }

    Serial.printf("Writing file to: %s\r\n", path);
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

// See:  https://github.com/esp8266/Arduino/blob/master/libraries/LittleFS/src/LittleFS.h#L149
void deleteFile2(fs::FS &fs, const char * path){
    Serial.printf("Deleting file and empty folders on path: %s\r\n", path);

    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }

    char *pathStr = strdup(path);
    if (pathStr) {
        char *ptr = strrchr(pathStr, '/');
        if (ptr) {
            Serial.printf("Removing all empty folders on path: %s\r\n", path);
        }
        while (ptr) {
            *ptr = 0;
            fs.rmdir(pathStr);
            ptr = strrchr(pathStr, '/');
        }
        free(pathStr);
    }
}

void testFileIO(fs::FS &fs, const char * path){
    Serial.printf("Testing file I/O with %s\r\n", path);

    static uint8_t buf[512];
    size_t len = 0;
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }

    size_t i;
    Serial.print("- writing" );
    uint32_t start = millis();
    for(i=0; i<2048; i++){
        if ((i & 0x001F) == 0x001F){
          Serial.print(".");
        }
        file.write(buf, 512);
    }
    Serial.println("");
    uint32_t end = millis() - start;
    Serial.printf(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
    file.close();

    file = fs.open(path);
    start = millis();
    end = start;
    i = 0;
    if(file && !file.isDirectory()){
        len = file.size();
        size_t flen = len;
        start = millis();
        Serial.print("- reading" );
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            if ((i++ & 0x001F) == 0x001F){
              Serial.print(".");
            }
            len -= toRead;
        }
        Serial.println("");
        end = millis() - start;
        Serial.printf("- %u bytes read in %u ms\r\n", flen, end);
        file.close();
    } else {
        Serial.println("- failed to open file for reading");
    }
}


void printErrorMessage(uint8_t e, bool eol = true)
{
  switch (e) {
  case SPIFFSIniFile::errorNoError:
    Serial.print("no error");
    break;
  case SPIFFSIniFile::errorFileNotFound:
    Serial.print("file not found");
    break;
  case SPIFFSIniFile::errorFileNotOpen:
    Serial.print("file not open");
    break;
  case SPIFFSIniFile::errorBufferTooSmall:
    Serial.print("buffer too small");
    break;
  case SPIFFSIniFile::errorSeekError:
    Serial.print("seek error");
    break;
  case SPIFFSIniFile::errorSectionNotFound:
    Serial.print("section not found");
    break;
  case SPIFFSIniFile::errorKeyNotFound:
    Serial.print("key not found");
    break;
  case SPIFFSIniFile::errorEndOfFile:
    Serial.print("end of file");
    break;
  case SPIFFSIniFile::errorUnknownError:
    Serial.print("unknown error");
    break;
  default:
    Serial.print("unknown error value");
    break;
  }
  if (eol)
    Serial.println();
}

void test()
{
    int val = 0;
  const size_t bufferLen = 4096;
  char buffer[bufferLen];

  const char *filename = "/config.txt";
  
  SPIFFSIniFile  ini(filename);
  if (!ini.open()) {
    Serial.print("Ini file ");
    Serial.print(filename);
    Serial.println(" does not exist");
    // Cannot do anything else
    while (1)
      ;
  }
  Serial.println("Ini file exists");
  // Check the file is valid. This can be used to warn if any lines
  // are longer than the buffer.
  if (!ini.validate(buffer, bufferLen)) {
    Serial.print("ini file ");
    Serial.print(ini.getFilename());
    Serial.print(" not valid: ");
    printErrorMessage(ini.getError());
    // Cannot do anything else
    while (1)
      ;
  }
  // Fetch a value from a key which is present
  if (ini.getValue("network", "mac", buffer, bufferLen)) {
    Serial.print("section 'network' has an entry 'mac' with value ");
    Serial.println(buffer);
  }
  else {
    Serial.print("Could not read 'mac' from section 'network', error was ");
    printErrorMessage(ini.getError());
  }

  // Fetch a value from a key which is present
  if (ini.getValue("misc", "volt", buffer, bufferLen, val)) {
    Serial.print("section 'volt' has an entry 'misc' with value ");
    Serial.println(val);
    Serial.println(buffer);
  }
  else {
    Serial.print("Could not read 'mac' from section 'network', error was ");
    printErrorMessage(ini.getError());
  }

}

void setup(void)
{
    Serial.begin(115200);                       // Initialize Serial to log output
    while (!Serial) ;

    if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
        Serial.println("LittleFS Mount Failed");
        return;
    }
    readFile(LittleFS, "/config.txt");
    test();
/*
    listDir(LittleFS, "/", 0);
	createDir(LittleFS, "/mydir");
	writeFile(LittleFS, "/mydir/hello2.txt", "Hello2");
  //writeFile(LittleFS, "/mydir/newdir2/newdir3/hello3.txt", "Hello3");
    writeFile2(LittleFS, "/mydir/newdir2/newdir3/hello3.txt", "Hello3");
	listDir(LittleFS, "/", 3);
	deleteFile(LittleFS, "/mydir/hello2.txt");
  //deleteFile(LittleFS, "/mydir/newdir2/newdir3/hello3.txt");
    deleteFile2(LittleFS, "/mydir/newdir2/newdir3/hello3.txt");
	removeDir(LittleFS, "/mydir");
	listDir(LittleFS, "/", 3);
    writeFile(LittleFS, "/hello.txt", "Hello ");
    appendFile(LittleFS, "/hello.txt", "World!\r\n");
    readFile(LittleFS, "/hello.txt");
    renameFile(LittleFS, "/hello.txt", "/foo.txt");
    readFile(LittleFS, "/foo.txt");
    deleteFile(LittleFS, "/foo.txt");
    testFileIO(LittleFS, "/test.txt");
    deleteFile(LittleFS, "/test.txt");
    Serial.println( "Test complete" );
    readFile(LittleFS, "/config.txt");
*/	
//    Serial.println( "Test complete :" + xx );

    //delay(1000); // 5 seconds
    tb_i2c_h70_Init();
    //bleInit();
    haptic_Init();
    zoomerInit();
    screen_Init();

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

	//100ms 한번씩 체크
	if(millis() - lastTime > 100) {
		lastTime = millis();
		ConnectTime++;

        if (deviceConnected) {
            String newValue = "T:" + String((float(get_tbh70())*0.02)-273.15);
            //str = String((float(get_tbh70())*0.02)-273.15);
            //Serial.println(newValue);
            pTxCharacteristic->setValue(newValue.c_str());
            pTxCharacteristic->notify();
            //haptic_shake(2, 20);
            //delay(100); // bluetooth stack will go into congestion, if too many packets are sent
        }


	}
    delay(10); // 1 second
}





