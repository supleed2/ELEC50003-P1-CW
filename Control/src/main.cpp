#include <Arduino.h>
#include <string>
#include <SoftwareSerial.h>

#define RX1pin 14 // Pin 10 on expansion board
#define TX1pin  4 // Pin 11 on expansion board
#define RX2pin 15 // Pin 12 on expansion board
#define TX2pin  2 // Pin 13 on expansion board
#define RX3pin 18 // Pin 6 on expansion board
#define TX3pin  5 // Pin 7 on expansion board
#define RX4pin 17 // Pin 8 on expansion board
#define TX4pin 16 // Pin 9 on expansion board

void forwardprint1() {
	if(Serial1.available()){
		input1 = String(Serial1.readStringUntil('\n'));
		Serial2.println(input1);
	}
}
void forwardprint2() {
	if(Serial2.available()){
		input2 = String(Serial2.readStringUntil('\n'));
		Serial3.println(input2);
	}
}
void forwardprint3() {
	if(Serial3.available()){
		input3 = String(Serial3.readStringUntil('\n'));
		Serial4.println(input3);
	}
}
void forwardprint4() {
	if(Serial4.available()){
		input4 = String(Serial4.readStringUntil('\n'));
		Serial.println(input4);
	}
}

int counter;
String input, input1, input2, input3, input4;
SoftwareSerial Serial3;
SoftwareSerial Serial4;

void setup() {
	Serial.begin(115200); // Set up hardware UART 0 (Connected to USB port)
	Serial1.begin(9600, SERIAL_8N1, RX1pin, TX1pin); // Set up hardware UART 1
	Serial2.begin(9600, SERIAL_8N1, RX2pin, TX2pin); // Set up hardware UART 2
	Serial3.begin(9600, SWSERIAL_8N1, RX3pin, TX3pin); // Set up software UART 3
	Serial4.begin(9600, SWSERIAL_8N1, RX4pin, TX4pin); // Set up software UART 4
}

void loop() {
	if(Serial.available()){
		input = String(Serial.readStringUntil('\n'));
		Serial1.println(input);
	}
	forwardprint1();
	forwardprint2();
	forwardprint3();
	forwardprint4();
}