#include <Arduino.h>
#include <ArduinoJson.h>
#include <string>

#define RXpin 0 // Define your RX pin here
#define TXpin 0 // Define your TX pin here

void setup()
{
    Serial.begin(115200);                          // Set up hardware UART0 (Connected to USB port)
    Serial1.begin(9600, SERIAL_8N1, RXpin, TXpin); // Set up hardware UART1

    // Other Drive setup stuff
}

void loop()
{
    bool charge;
    DynamicJsonDocument rdoc(1024); // receive doc, not sure how big this needs to be
    if(Serial1.available()){
        deserializeJson(rdoc, Serial1); // Take JSON input from UART1
        charge = rdoc["ch"]; // {"ch":0}
    }


    float stateOfCharge = 0;
    float batteryVoltage = 0;
    float batteryCycles = 0;

    // Do Drive stuff, set the 5 values above

    DynamicJsonDocument tdoc(1024); // transmit doc, not sure how big this needs to be
    tdoc["soc"] = stateOfCharge;
    tdoc["mV"] = batteryVoltage;
    tdoc["cyc"] = batteryCycles;
    serializeJson(tdoc, Serial1); // Build JSON and send on UART1
}