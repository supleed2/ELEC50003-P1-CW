#pragma region Includes
#include <Arduino.h>
#include <string>
#include <SoftwareSerial.h> // Software Serial not currently needed
#include <SoftwareSerial.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "TickerV2.h"
#include <WebSocketsServer.h>
#include "credentials.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "status.h"
#include "instruction.h"
#include <queue>
#pragma endregion

#pragma region Enable extra debugging info for ESP32
#undef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#pragma endregion

#pragma region Definitions eg pins
#define RX1pin 17 // Pin 6 on expansion board, UART1
#define TX1pin 16 // Pin 7 on expansion board, UART1
#define RX2pin 18 // Pin 8 on expansion board, UART2
#define TX2pin 5 // Pin 9 on expansion board, UART2
#define RX3pin 14 // Pin 10 on expansion board, UART3
#define TX3pin 4  // Pin 11 on expansion board, UART3
#define RX4pin 15 // Pin 12 on expansion board, UART4
#define TX4pin 2  // Pin 13 on expansion board, UART4
#pragma endregion

#pragma region Function Declarations
void notFound(AsyncWebServerRequest *request);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void queueInstruction(RoverInstruction instruction);
void sendToCommand();
void sendToDrive(RoverInstruction instruction);
void recvFromDrive();
void sendToEnergy(bool instruction);
void recvFromEnergy();
void sendToVision();
void recvFromVision();
void recvFromCompass();
void emergencyStop();
#pragma endregion

#pragma region Global objects
AsyncWebServer webserver(80);
WebSocketsServer websocketserver(81);
Ticker ticker;
SoftwareSerial Serial3, Serial4;
std::queue<RoverInstruction> InstrQueue;
#pragma endregion

#pragma region Global variables
ControlStatus_t Status;
float batteryVoltage;
float batteryLevel;
float batteryCycles;
int odometer;
int heading;
int xpos, ypos;
int signalStrength;
int lastExecutedCommand, lastCompletedCommand;
bool driveCommandComplete;
int bb_left, bb_right, bb_top, bb_bottom;
int bb_centre_x, bb_centre_y;
float chargeGoal;
int waitGoal;
#pragma endregion

void setup()
{
	esp_log_level_set("*", ESP_LOG_ERROR);	  // set all components to ERROR level
	esp_log_level_set("wifi", ESP_LOG_WARN);  // enable WARN logs from WiFi stack
	esp_log_level_set("dhcpc", ESP_LOG_INFO); // enable INFO logs from DHCP client

	Serial.begin(115200);								 // Set up hardware UART0 (Connected to USB port)
	Serial1.begin(9600, SERIAL_8N1, RX1pin, TX1pin);	 // Set up hardware UART1 (Connected to Drive)
	Serial2.begin(9600, SERIAL_8N1, RX2pin, TX2pin);	 // Set up hardware UART2 (Connected to Energy)
	Serial3.begin(9600, SWSERIAL_8N1, RX3pin, TX3pin);	 // Set up software UART3 (Connected to Vision)
	Serial4.begin(9600, SWSERIAL_8N1, RX4pin, TX4pin); // Set up software UART4 (Connected to Compass)

	// Set global variable startup values
	Status = CS_IDLE;
	batteryVoltage = 0;
	batteryLevel = 0;
	batteryCycles = 0;
	odometer = 0;
	heading = 0;
	xpos = 0;
	ypos = 0;
	signalStrength = 0;
	lastExecutedCommand = 0;
	lastCompletedCommand = 0;
	driveCommandComplete = 1;
	chargeGoal = 0;
	waitGoal = 0;

	if (!SPIFFS.begin(true)) // Mount SPIFFS
	{
		Serial.println("SPIFFS failed to mount");
		return;
	}
	Serial.println("SPIFFS mounted");

	WiFi.begin(WIFI_SSID, WIFI_PW);
	while (WiFi.status() != WL_CONNECTED) // Wait for ESP32 to connect to AP in "credentials.h"
	{
		delay(500);
	}
	while (!MDNS.begin("rover2")) // Set up mDNS cast at "rover.local/"
	{
		Serial.println("Error setting up mDNS, retrying in 5s");
		delay(5000);
	}
	Serial.println("mDNS set up, access Control Panel at 'rover2.local/'");

	webserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
				 { request->send(SPIFFS, "/index.html", "text/html"); }); // Serve "index.html" at root page
	webserver.on("/command.js", HTTP_GET, [](AsyncWebServerRequest *request)
				 { request->send(SPIFFS, "/command.js", "text/js"); }); // Serve "command.js" for root page to accessj
	webserver.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
				 { request->send(SPIFFS, "/favicon.ico", "image/png"); }); // Serve tab icon
	webserver.onNotFound(notFound);										   // Set up basic 404NotFound page
	webserver.begin();													   // Start Asynchronous Web Server

	websocketserver.begin();				 // Start Websocket Server
	websocketserver.onEvent(webSocketEvent); // Set up function call when event received from Command
	ticker.attach(0.5, sendToCommand);		 // Set up recurring function to forward rover status to Command
}

void loop()
{
	websocketserver.loop(); // Handle incoming client connections
	recvFromDrive();		// Update stats from Drive
	recvFromEnergy();		// Update stats from Energy
	// recvFromVision();		// Update stats from Vision
	recvFromCompass();		// Update stats from Compass
	switch (Status)
	{
	case CS_ERROR:
	{
		Serial.println("Rover in error state, rebooting...");
		exit(1);
	}
	break;
	case CS_IDLE:
	{
		if (!InstrQueue.empty()) // If Rover idle and InstrQueue NOT empty: Do the next command in the queue
		{
			RoverInstruction *instr = &InstrQueue.front(); // Get next command
			switch (instr->instr)						   // Determine command type
			{
			case INSTR_RESET: // Reset telemetry values (zeroing position/distance)
			{
				odometer = 0;
				xpos = 0;
				ypos = 0;
				DynamicJsonDocument tdoc(128);
				tdoc["rstD"] = 1;
				serializeJson(tdoc, Serial1); // Send reset odometer signal to Drive
			}
			break;
			case INSTR_STOP: // Emergency stop
			{
				Status = CS_ERROR;
				while (1)
				{
					Serial.println("Emergency Stop should not get queued, hold and print");
					delay(1000);
				}
			}
			break;
			case INSTR_MOVE: // Normal movement
			{
				Status = CS_MOVING; // Set moving state
				driveCommandComplete = 0;
				sendToDrive(*instr); // Forward to Drive handler
			}
			break;
			case INSTR_CHARGE: // Normal charge
			{
				Status = CS_CHARGING;			   // Set charging state
				chargeGoal = (float)instr->charge; // Set charging goal
				sendToEnergy(1);				   // Forward to Energy handler
			}
			break;
			case INSTR_WAIT: // Normal wait
			{
				Status = CS_WAITING;			   // Set waiting state
				waitGoal = millis() + 1000*(instr->time); // Set wait time
			}
			break;
			default:
			{
				Serial.println("Unknown instruction type in queue, skipping...");
			}
			break;
			}
			lastExecutedCommand = instr->id; // Update tracker of last processed command
			InstrQueue.pop();
		}
	}
	break;
	case CS_MOVING:
	{
		if (driveCommandComplete) // If movement command complete:
		{
			Status = CS_IDLE;							// Set rover state back to idle
			lastCompletedCommand = lastExecutedCommand; // Update last completed command
		}
		else // If movement command NOT complete:
		{	 // Send (up to date) current heading to Drive
			DynamicJsonDocument tdoc(128);
			tdoc["rH"] = -1;
			tdoc["cH"] = heading;
			serializeJson(tdoc, Serial1);
		}
	}
	break;
	case CS_CHARGING:
	{
		if (batteryLevel >= chargeGoal) // Compare batteryLevel to chargeGoal
		{
			Status = CS_IDLE;
			lastCompletedCommand = lastExecutedCommand; // Update last completed command
			sendToEnergy(0);							// Stop charging if goal reached
		}
		// Otherwise continue charging, no change
	}
	break;
	case CS_WAITING:
	{
		if (millis() >= waitGoal) // Compare waitGoal to current time
		{
			Status = CS_IDLE;
			lastCompletedCommand = lastExecutedCommand; // Update last completed command
		}
		// Otherwise continue waiting, no change
	}
	break;
	default:
	{
		Serial.println("Unknown rover state, exiting...");
		exit(1);
	}
	break;
	}
	// delay(500);
}

void notFound(AsyncWebServerRequest *request)
{
	request->send(404, "text/plain", "Page Not found. Check URI/IP address.");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
	switch (type)
	{
	case WStype_DISCONNECTED:
	{
		Serial.printf("Client[%u] Disconnected!\n", num);
	}
	break;
	case WStype_CONNECTED:
	{
		IPAddress ip = websocketserver.remoteIP(num);
		Serial.printf("Client[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
	}
	break;
	case WStype_TEXT: // MSG received from command panel
	{
		Serial.printf("Client[%u] sent Text: %s\n", num, payload); // Echo received command to terminal
		String command = String((char *)(payload));				   // Convert received command to string type

		DynamicJsonDocument rdoc(200);								 // Create instance of DynamicJsonDocument on heap, 200 Bytes
		DeserializationError error = deserializeJson(rdoc, command); // Convert command string to JSONDocument and capture any errors
		if (error)
		{
			Serial.print("deserializeJson() failed: ");
			Serial.println(error.c_str());
			return;
		}

		RoverInstruction instr;
		int mode = rdoc["mode"];
		switch (mode)
		{
		case -1: // Add to queue, reset x/y/odometer (telemetry data)
		{
			Serial.println("Reset telemetry command received");
			instr.id = rdoc["Cid"];
			instr.instr = INSTR_RESET;
			// Ignore rdoc["rH"], rdoc["rD"], rdoc["rS"], rdoc["rC"], rdoc["pSt"]

			queueInstruction(instr); // Put reset command in InstrQueue
		}
		break;
		case 0: // Stop immediately, clear command cache
		{
			Serial.println("Emergency stop command received");
			// instr.instr = INSTR_STOP; // Not needed as Emergency Stop is not queued
			// Ignore rdoc["Cid"], rdoc["rH"], rdoc["rD"], rdoc["rS"], rdoc["rC"], rdoc["pSt"]

			emergencyStop();
		}
		break;
		case 1: // Normal movement command, added to end of command cache
		{
			Serial.println("Normal movement command received");
			instr.id = rdoc["Cid"];
			instr.instr = INSTR_MOVE;
			instr.heading = rdoc["rH"];
			instr.distance = rdoc["rD"];
			instr.speed = rdoc["rS"];
			// Ignore rdoc["rC"], rdoc["pSt"]

			queueInstruction(instr); // Put movement command in InstrQueue
		}
		break;
		case 2: // Normal charge command, results in no motion, added to end of command cache
		{
			Serial.println("Normal charge command received");
			instr.id = rdoc["Cid"];
			instr.instr = INSTR_CHARGE;
			instr.charge = rdoc["rC"];
			// Ignore rdoc["rH"], rdoc["rD"], rdoc["rS"], rdoc["pSt"]

			queueInstruction(instr); // Put charge command in InstrQueue
		}
		break;
		case 3: // Normal wait command, results in no motion, added to end of command cache
		{
			Serial.println("Normal wait command received");
			instr.id = rdoc["Cid"];
			instr.instr = INSTR_WAIT;
			instr.time = rdoc["pSt"];
			// Ignore rdoc["rH"], rdoc["rD"], rdoc["rS"], rdoc["rC"]

			queueInstruction(instr); // Put charge command in InstrQueue
		}
		break;
		default:
		{
			// Default case, print and continue
			Serial.println("Unknown Command type received, ignoring");
			// Ignore rdoc["Cid"], rdoc["rH"], rdoc["rD"], rdoc["rS"], rdoc["rC"]
		}
		break;
		}
	}
	break;
	case WStype_PONG:
	{
		Serial.println("Websocket keep-alive PONG");
	}
	break;
	default:
	{
		Serial.println(String("Websocket received invalid event type: ") + type + String(", exiting"));
		exit(1);
	}
	break;
	}
}

void queueInstruction(RoverInstruction instruction)
{
	InstrQueue.push(instruction);
}

void sendToCommand()
{
	DynamicJsonDocument tdoc(1024);
	tdoc["st"] = Status;
	tdoc["bV"] = batteryVoltage;
	tdoc["bL"] = batteryLevel;
	tdoc["bC"] = batteryCycles;
	tdoc["tD"] = odometer;
	tdoc["cH"] = heading;
	tdoc["pos"][0] = xpos;
	tdoc["pos"][1] = ypos;
	tdoc["rssi"] = signalStrength;
	tdoc["LCCid"] = lastCompletedCommand;
	String JSON_Data;
	serializeJson(tdoc, JSON_Data);
	websocketserver.broadcastTXT(JSON_Data);
}

void sendToDrive(RoverInstruction instruction)
{
	DynamicJsonDocument tdoc(1024);
	tdoc["rH"] = instruction.heading;
	tdoc["dist"] = instruction.distance;
	tdoc["sp"] = instruction.speed;
	tdoc["cH"] = heading;
	serializeJson(tdoc, Serial1);
}

void recvFromDrive() // Update telemetry data and state info from Drive packet
{
	if (Serial1.available()) // Check for input from UART1 (Connected to Drive)
	{
		DynamicJsonDocument rdoc(1024);
		deserializeJson(rdoc, Serial1);
		driveCommandComplete = rdoc["comp"];
		odometer = rdoc["mm"];
		xpos = rdoc["pos"][0];
		ypos = rdoc["pos"][1];
	}
}

void sendToEnergy(bool instruction)
{
	DynamicJsonDocument tdoc(128);
	tdoc["ch"] = instruction; // Start charging
	serializeJson(tdoc, Serial2);
}

void recvFromEnergy() // Update telemetry data and state info from Energy packet
{
	if (Serial2.available()) // Check for input from UART2 (Connected to Energy)
	{
		DynamicJsonDocument rdoc(1024);
		deserializeJson(rdoc, Serial2);
		batteryLevel = rdoc["soc"];
		batteryVoltage = rdoc["mV"];
		batteryCycles = rdoc["cyc"];
	}
}

void sendToVision()
{
	Serial3.print("R"); // Request new data from Vision
}

void recvFromVision() // Update bounding box and obstacle detection data from Vision packet
{
	if (Serial3.available()) // Check for input from UART3 (Connected to Vision)
	{
		DynamicJsonDocument rdoc(1024);
		deserializeJson(rdoc, Serial3);
		bb_left = rdoc["bb"][0];
		bb_right = rdoc["bb"][1];
		bb_top = rdoc["bb"][2];
		bb_bottom = rdoc["bb"][3];
		bb_centre_x = rdoc["cen"][0];
		bb_centre_y = rdoc["cen"][1];
		heading = rdoc["cH"];
	}
}

void recvFromCompass()
{
	if (Serial4.available())
	{
		DynamicJsonDocument rdoc(1024);
		deserializeJson(rdoc, Serial4);
		heading = rdoc["cH"];
	}
}

void emergencyStop()
{
	DynamicJsonDocument tdoc(1024);
	tdoc["stp"] = 1;
	serializeJson(tdoc, Serial1); // Send stop signals to Drive
	sendToEnergy(0);			  // Send stop signal to Energy
	while (InstrQueue.size())
	{
		InstrQueue.pop(); // Clear Instruction Queue
	}
	Status = CS_IDLE; // Reset rover to idle state
	Serial.println("Instruction Queue cleared");
}