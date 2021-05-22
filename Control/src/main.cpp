#include <Arduino.h>
#include <string>

#define RX1pin 14 // Pin 10 on expansion board
#define TX1pin  4 // Pin 11 on expansion board
#define RX2pin 15 // Pin 12 on expansion board
#define TX2pin  2 // Pin 13 on expansion board

void forwardprint();

int counter;
String input1, input2;

void setup() {
	Serial.begin(115200); // Set up hardware UART 0 (Connected to USB port)
	Serial1.begin(9600, SERIAL_8N1, RX1pin, TX1pin); // Set up hardware UART 1
	Serial2.begin(9600, SERIAL_8N1, RX2pin, TX2pin); // Set up hardware UART 2
}

void loop() {
	forwardprint();
	if(Serial.available()){
		input1 = String(Serial.readStringUntil('\n'));
		Serial1.println(input1);
	}
}

void forwardprint() {
	if(Serial2.available()){
		input2 = String(Serial2.readStringUntil('\n'));
		Serial.println(input2);
	}
}
