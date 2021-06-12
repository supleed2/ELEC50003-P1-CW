#include <Arduino.h>
#include <ArduinoJson.h>
#include <string>

#define WebSocket 0
int state, batteryVoltage, batteryLevel, totalTripDistance, currentHeading, current_x, current_y, signal_strength, lastCompletedCommand_id; // Info Control ==> Command
int command_id, mode, reqHeading, reqDistance, reqSpeed, reqCharge; // Info Command ==> Control

void setup() {}

void loop()
{
	DynamicJsonDocument rdoc(1024); // receive doc, not sure how big this needs to be
	deserializeJson(rdoc, WebSocket); // Take JSON input from WebSocket
	state = rdoc["st"]; // State: -1 = Error, 0 = Idle, 1 = Moving, 2 = Charging
	batteryVoltage =  rdoc["bV"];
	batteryLevel =  rdoc["bL"];
	totalTripDistance = rdoc["tD"];
	currentHeading = rdoc["cH"];
	current_x = rdoc["pos"][0];
	current_y = rdoc["pos"][1];
	signal_strength = rdoc["rssi"];
	lastCompletedCommand_id = rdoc["LCCid"];

	// ResetTelemetry / STOP / M 0 50 1 / C %
	// [20] Heading: 0, Distance: 50, Speed: 1 / [20] Charging to: ??%
	// {"Cid":20,"rH":0,}

	DynamicJsonDocument tdoc(1024); // transmit doc, not sure how big this needs to be
	tdoc["Cid"] = command_id;
	tdoc["mode"] = mode;	// Switch (mode):
							// -1 = Add to queue, reset x/y/odometer (telemetry data)
							//  0 = Stop immediately, clear command cache
							//  1 = Normal movement command, added to end of command cache
							//  2 = Normal charge command, results in no motion, added to end of command cache
	tdoc["rH"] = reqHeading;
	tdoc["rD"] = reqDistance;
	tdoc["rS"] = reqSpeed;
	tdoc["rC"] = reqCharge;
	serializeJson(tdoc, WebSocket, WebSocket); // Build JSON and send on UART1
}
