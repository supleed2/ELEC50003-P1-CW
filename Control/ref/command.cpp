#include <Arduino.h>
#include <ArduinoJson.h>
#include <string>

#define WebSocket 0
int state, totalTripDistance, currentHeading, current_x, current_y, signal_strength, lastCompletedCommand_id; // Info Control ==> Command
float batteryVoltage, batteryLevel, batteryCycles; // Info Control ==> Command
int command_id, mode, reqHeading, reqDistance, reqCharge, reqTime, reqColour; // Info Command ==> Control
float reqSpeed; // Info Command ==> Control

void setup() {}

void loop()
{
	DynamicJsonDocument rdoc(1024); // receive doc, not sure how big this needs to be
	deserializeJson(rdoc, WebSocket); // Take JSON input from WebSocket
	state = rdoc["st"]; // State: -1 = Error, 0 = Idle, 1 = Moving, 2 = Charging, 3 = Waiting
	batteryVoltage = rdoc["bV"];
	batteryLevel = rdoc["bL"];
	batteryCycles = rdoc["bC"];
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
							//  3 = Pause command, wait for defined time in seconds, added to end of command cache
							//  4 = Change colour tracking using Vision
	tdoc["rH"] = reqHeading;
	tdoc["rD"] = reqDistance;
	tdoc["rS"] = reqSpeed;
	tdoc["rC"] = reqCharge;
	tdoc["pSt"] = reqTime;
	tdoc["col"] = reqColour;
	serializeJson(tdoc, WebSocket, WebSocket); // Build JSON and send on UART1
}
