
#include "Arduino.h"
#include <haptic.h>

#define ERM_MOTOR   6

bool isShake;
uint32_t _cnt, _interval_ms;

void haptic_shake(const uint32_t cnt, const uint32_t interval)
{
    _cnt = (cnt + 1) * 2;
    _interval_ms = interval;
    isShake = true;
}

void haptic_loop(uint32_t millis)
{
    static uint32_t Millis;

    if (millis - Millis > _interval_ms && isShake)
    {
        digitalWrite(ERM_MOTOR, _cnt % 2);
        if (!--_cnt)
            haptic_stop();
        Millis = millis;
    }
}

void haptic_stop()
{
    digitalWrite(ERM_MOTOR, LOW);
    isShake = false;
    _cnt = 0;
    _interval_ms = 0xFFFFFFFF;
}

void haptic_Task(void *pvParameters)
{
    static uint32_t ms;

    pinMode(ERM_MOTOR, OUTPUT);
    haptic_stop();

    while(1)
    {
        ms = millis();
        haptic_loop(ms);
        delay(2);
    }
   
}

void haptic_Init()
{
    xTaskCreatePinnedToCore( haptic_Task, "Haptic", 4096, NULL, 38 | portPRIVILEGE_BIT, NULL, 1);
}
