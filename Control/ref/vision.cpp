#include <Arduino.h>
#include <ArduinoJson.h>
#include <string>

const int ARDUINO_IO[16] = {-1/*RX*/, -1/*RX*/, 23, 22, 21, 19, 18, 5, 17, 16, 14, 4, 15, 2, 13, 12}; // Expansion board mapping
#define RXpin ARDUINO_IO[11] // Define your RX pin here
#define TXpin ARDUINO_IO[10] // Define your TX pin here
FILE* SerialUART;

void setup()
{
    Serial.begin(115200);                          // Set up hardware UART0 (Connected to USB port)
}

void loop()
{
    int command = getc(SerialUART);
    // command char, used for controlling exposure/focus/gain settings:
    // e = increase exposure
    // d = decrease exposure
    // r = increase focus
    // f = decrease focus
    // t = increase gain
    // g = decrease gain

    // Bounding Box edges
    int bb_left = 0;
    int bb_right = 0;
    int bb_top = 0;
    int bb_bottom = 0;
    // Weighted average of detected pixels coordinates
    int centre_x = 0;
    int centre_y = 0;
    // Heading from DE10-Lite magnetometer
    float heading = 0.0;

    // Build hardcode JSON packet on DE10-Lite using fprintf() as space is minimal and library would be too large.
    // fprintf(SerialUART, "{\"bb\":[%d,%d,%d,%d],\"cen\":[%d,%d],\"cH\":%d\"}", bb_left, bb_right, bb_top, bb_bottom, centre_x, centre_y, heading);

    DynamicJsonDocument rdoc(1024); // receive doc, not sure how big this needs to be
    deserializeJson(rdoc, SerialUART);
    bb_left = rdoc["bb"][0];
    bb_right = rdoc["bb"][1];
    bb_top = rdoc["bb"][2];
    bb_bottom = rdoc["bb"][3];
    centre_x = rdoc["cen"][0];
    centre_y = rdoc["cen"][1];
    heading = rdoc["cH"];
}