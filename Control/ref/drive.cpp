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
    DynamicJsonDocument rdoc(1024); // receive doc, not sure how big this needs to be
    deserializeJson(rdoc, Serial1); // Take JSON input from UART1
    int requiredHeading = rdoc["rH"]; // if -1: command in progress, returning requested heading, dist/sp to be ignored
    int distance = rdoc["dist"];
    float speed = rdoc["sp"];
    int currentHeading = rdoc["cH"];

    bool commandComplete = 0;
    float powerUsage_mW = 0.0;
    int distTravelled_mm = 0;
    int current_x = 0;
    int current_y = 0;

    // Do Drive stuff, set the 5 values above

    DynamicJsonDocument tdoc(1024); // transmit doc, not sure how big this needs to be
    tdoc["comp"] = commandComplete; // If 0: command in progress, current heading requested
    tdoc["mW"] = powerUsage_mW;
    tdoc["mm"] = distTravelled_mm;
    tdoc["pos"][0] = current_x;
    tdoc["pos"][1] = current_y;
    serializeJson(tdoc, Serial1); // Build JSON and send on UART1
}