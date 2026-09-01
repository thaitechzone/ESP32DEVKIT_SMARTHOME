#include <Arduino.h>

 int Relay1pin = 17;

void setup() {
   pinMode(Relay1pin, OUTPUT);
   digitalWrite(Relay1pin, HIGH);
}

void loop() {
    digitalWrite(Relay1pin, LOW);
    delay(1000);
   
}

 