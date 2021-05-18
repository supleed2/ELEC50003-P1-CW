#include <Arduino.h>
#define LED_BUILTIN 2

void setup() {
	Serial.begin(115200);
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
	delay(2000);
	digitalWrite(LED_BUILTIN, HIGH);
	delay(2000);
	digitalWrite(LED_BUILTIN, LOW);
}