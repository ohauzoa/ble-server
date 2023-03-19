#ifndef HAPTIC_H
#define HAPTIC_H

#include "Arduino.h"


void haptic_shake(const uint32_t cnt, const uint32_t interval);
void haptic_stop();
void haptic_Init();

#endif // HAPTIC_H
