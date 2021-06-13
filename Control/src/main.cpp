#pragma region Includes
#include <Arduino.h>
#include <string>
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
#define RX1pin 18 // Pin 6 on expansion board, UART1
#define TX1pin 5  // Pin 7 on expansion board, UART1
#define RX2pin 17 // Pin 8 on expansion board, UART2
#define TX2pin 16 // Pin 9 on expansion board, UART2
#define RX3pin 14 // Pin 10 on expansion board, UART3
#define TX3pin 4  // Pin 11 on expansion board, UART3
#pragma endregion

#pragma region Function Declarations
void notFound(AsyncWebServerRequest *request);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void queueInstruction(RoverInstruction instruction);
void sendToCommand();
void sendToDrive(RoverInstruction instruction);
void recvFromDrive();
void sendToEnergy(RoverInstruction instruction);
void recvFromEnergy();
void sendToVision();
void recvFromVision();
#pragma endregion

#pragma region Global objects
AsyncWebServer webserver(80);
WebSocketsServer websocketserver(81);
Ticker ticker;
SoftwareSerial Serial3;
std::queue<RoverInstruction> InstrQueue;
#pragma endregion

#pragma region Global variables
ControlStatus_t Status;
float batteryVoltage;
int batteryLevel;
int odometer;
int heading;
int xpos, ypos;
int signalStrength;
int lastCompletedCommand;
#pragma endregion

void setup()
{
	esp_log_level_set("*", ESP_LOG_ERROR);	  // set all components to ERROR level
	esp_log_level_set("wifi", ESP_LOG_WARN);  // enable WARN logs from WiFi stack
	esp_log_level_set("dhcpc", ESP_LOG_INFO); // enable INFO logs from DHCP client

	Serial.begin(115200);							   // Set up hardware UART0 (Connected to USB port)
	Serial1.begin(9600, SERIAL_8N1, RX1pin, TX1pin);   // Set up hardware UART1 (Connected to Drive)
	Serial2.begin(9600, SERIAL_8N1, RX2pin, TX2pin);   // Set up hardware UART2 (Connected to Energy)
	Serial3.begin(9600, SWSERIAL_8N1, RX3pin, TX3pin); // Set up software UART3 (Connected to Vision)

	// Set global variable startup values
	Status = CS_IDLE;
	batteryVoltage = 0;
	batteryLevel = 0;
	odometer = 0;
	heading = 0;
	xpos = 0;
	ypos = 0;
	signalStrength = 0;
	lastCompletedCommand = 0;

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
	while (!MDNS.begin("rover")) // Set up mDNS cast at "rover.local/"
	{
		Serial.println("Error setting up mDNS, retrying in 5s");
		delay(5000);
	}
	Serial.println("mDNS set up, access Control Panel at 'rover.local/'");

	webserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
				 { request->send(SPIFFS, "/index.html", "text/html"); }); // Serve "index.html" at root page
	webserver.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
				 { request->send(SPIFFS, "/favicon.ico", "image/png"); }); // Serve tab icon
	webserver.onNotFound(notFound);										   // Set up basic 404NotFound page
	webserver.begin();													   // Start Asynchronous Web Server

	websocketserver.begin();				 // Start Websocket Server
	websocketserver.onEvent(webSocketEvent); // Set up function call when event received from Command
	ticker.attach(0.5, sendToCommand);		 // Set up recurring function to forward rover status to Command
}

void loop() // TO DO
{
	websocketserver.loop(); // Handle incoming client connections
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
		if (InstrQueue.empty()) // If Rover idle and InstrQueue empty:
		{
			// TO DO: Collect all data (recvFrom) and
			sendToCommand(); // Update command panel
							 // Maybe wait 1s? Possibly prevent from looping too fast
		}
		else
		{
			// Do the next command in the queue
			RoverInstruction *instr = &InstrQueue.front();
			switch (instr->instr)
			{
			case INSTR_RESET:
			{
				odometer = 0;
				xpos = 0;
				ypos = 0;
			}
			break;
			case INSTR_STOP:
			{
				while (1)
				{
					Serial.println("Emergency Stop should not get queued, hold and print");
					delay(1000);
				}
			}
			break;
			case INSTR_MOVE:
			{
				Status = CS_MOVING;
				sendToDrive(*instr);
			}
			break;
			case INSTR_CHARGE:
			{
				Status = CS_CHARGING;
				sendToEnergy(*instr);
			}
			break;
			default:
			{
				Serial.println("Unknown instruction type in queue, skipping...");
			}
			break;
			}
		}
	}
	break;
	case CS_MOVING:
	{
		// TO DO
	}
	break;
	case CS_CHARGING:
	{
		// TO DO
	}
	break;
	default:
	{
		Serial.println("Unknown rover state, exiting...");
		exit(1);
	}
	}
}

void notFound(AsyncWebServerRequest *request)
{
	request->send(404, "text/plain", "Page Not found. Check URI/IP address.");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) // TO DO
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
			// Ignore rdoc["rH"], rdoc["rD"], rdoc["rS"], rdoc["rC"]

			/* Put reset command in commandFIFO */
		}
		break;
		case 0: // Stop immediately, clear command cache
		{
			Serial.println("Emergency stop command received");
			// instr.instr = INSTR_STOP; // Not needed as Emergency Stop is not queued
			// Ignore rdoc["Cid"], rdoc["rH"], rdoc["rD"], rdoc["rS"], rdoc["rC"]

			/* Clear commandFIFO */
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
			// Ignore rdoc["rC"]

			/* Put movement command in commandFIFO */
		}
		break;
		case 2: // Normal charge command, results in no motion, added to end of command cache
		{
			Serial.println("Normal charge command received");
			instr.id = rdoc["Cid"];
			instr.instr = INSTR_CHARGE;
			instr.charge = rdoc["rC"];
			// Ignore rdoc["rH"], rdoc["rD"], rdoc["rS"]

			/* Put charge command in commandFIFO */
		}
		break;
		default:
		{
			Serial.println("Unknown Command type received, ignoring"); // Default case, print and continue
																	   // Ignore rdoc["Cid"], rdoc["rH"], rdoc["rD"], rdoc["rS"], rdoc["rC"]
		}
		break;
		}

		queueInstruction(instr);
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

void recvFromDrive() // TO DO
{
}

void sendToEnergy(RoverInstruction instruction)
{
	DynamicJsonDocument tdoc(1024);
	tdoc["ch"] = instruction.charge;
	serializeJson(tdoc, Serial2);
}

void recvFromEnergy() // TO DO
{
}

void sendToVision()
{
	Serial3.print("R"); // Request new data from Vision
}

void recvFromVision() // TO DO
{
}

void emergencyStop()
{
	DynamicJsonDocument tdoc(1024);
	tdoc["rH"] = heading;
	tdoc["dist"] = -1;
	tdoc["sp"] = -1;
	tdoc["cH"] = heading;
	tdoc["ch"] = 0;
	serializeJson(tdoc, Serial1); // Send stop signals to Drive
	serializeJson(tdoc, Serial2); // Send stop signals to Energy
	while (InstrQueue.size())
	{
		InstrQueue.pop(); // Clear Instruction Queue
	}
	Status = CS_IDLE; // Reset rover to idle state
	Serial.println("Instruction Queue cleared");
}