#ifndef SCREEN_H
#define SCREEN_H

#include "Arduino.h"
#include <BLEDevice.h>

extern float testValue;
extern BLECharacteristic * pTxCharacteristic;
extern bool deviceConnected;
void screen_Init();

#endif // SCREEN_H
