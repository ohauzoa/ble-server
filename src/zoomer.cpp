#include <Arduino.h>

#include <AccelStepper.h>
#include <zoomer.h>

AccelStepper stepper(AccelStepper::FULL4WIRE, 15,16,17,18);

boolean moveClockwise = true;

void zoomerTask(void *pvParameters)
{
    stepper.setMaxSpeed(300.0);
    Serial.println();
    Serial.print(stepper.currentPosition()); // get the RPM of the stepper
    Serial.print(" rpm = delay of ");
    Serial.print(stepper.maxSpeed()); // get delay between steps for set RPM
    Serial.print(" microseconds between steps");
    Serial.println();
    stepper.setAcceleration(100.0);
    moveClockwise = true;
    //stepper.moveTo(1000);

    while(1){
        while (stepper.currentPosition() != stepper.targetPosition()) // Full speed up to 300
        {
            stepper.run();
            delay(1);
        }    
        stepper.stop(); // Stop as fast as possible: sets new target
        digitalWrite(15, LOW);
        digitalWrite(16, LOW);
        digitalWrite(17, LOW);
        digitalWrite(18, LOW);
        delay(100);

    }

}

void zoomerInit(void)
{
    xTaskCreatePinnedToCore( zoomerTask, "zoomer", 4096, NULL, 39 | portPRIVILEGE_BIT, NULL, 1);
}
