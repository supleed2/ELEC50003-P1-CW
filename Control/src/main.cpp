#pragma region Includes
#include <Arduino.h>
#include <string>
// #include <SoftwareSerial.h> Software Serial not currently needed
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "TickerV2.h"
#include <WebSocketsServer.h>
#include "credentials.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "status.h"
#pragma endregion

#pragma region Enable extra debugging info for ESP32
#undef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#pragma endregion

#pragma region Definitions eg pins
#define RX1pin 14 // Pin 10 on expansion board, UART1
#define TX1pin 4  // Pin 11 on expansion board, UART1
#pragma endregion

#pragma region Function Declarations
void printFPGAoutput();
void returnSensorData();
void notFound(AsyncWebServerRequest *request);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
#pragma endregion

#pragma region Global objects
AsyncWebServer webserver(80);
WebSocketsServer websocketserver(81);
Ticker ticker;
#pragma endregion

#pragma region Global variables
ControlStatus_t Status;
float battery_voltage = 4.0f;
int distance_travelled = 0;
#pragma endregion

void setup()
{
	esp_log_level_set("*", ESP_LOG_ERROR);	  // set all components to ERROR level
	esp_log_level_set("wifi", ESP_LOG_WARN);  // enable WARN logs from WiFi stack
	esp_log_level_set("dhcpc", ESP_LOG_INFO); // enable INFO logs from DHCP client

	Status = CS_IDLE;
	Serial.begin(115200);							 // Set up hardware UART0 (Connected to USB port)
	Serial1.begin(9600, SERIAL_8N1, RX1pin, TX1pin); // Set up hardware UART1
	// Set up remaining communication ports here (Energy, Drive, Vision)

	if (!SPIFFS.begin(true)) // Mount SPIFFS
	{
		Serial.println("SPIFFS failed to mount");
		return;
	}
	Serial.println("SPIFFS mounted");

	WiFi.begin(WIFI_SSID, WIFI_PW);
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
	}
	while (!MDNS.begin("rover"))
	{
		Serial.println("Error setting up mDNS, retrying in 5s");
		delay(5000);
	}
	Serial.println("mDNS set up, access Control Panel at 'rover.local/'");

	webserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
				 { request->send(SPIFFS, "/index.html", "text/html"); });
	webserver.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
				 { request->send(SPIFFS, "/favicon.ico", "image/png"); });
	webserver.onNotFound(notFound);
	webserver.begin();

	websocketserver.begin();
	websocketserver.onEvent(webSocketEvent);
	ticker.attach(0.5, returnSensorData);
}

void loop()
{
	printFPGAoutput();

	String FPGAinput; // Forward serial monitor input to FPGA
	if (Serial.available())
	{
		FPGAinput = String(Serial.readStringUntil('\n'));
		Serial1.println(FPGAinput);
	}

	websocketserver.loop(); // Handle incoming client connections
}

void printFPGAoutput()
{ // Print serial communication from FPGA to serial monitor
	String FPGAoutput;
	if (Serial1.available())
	{
		FPGAoutput = String(Serial1.readStringUntil('\n'));
		Serial.println(FPGAoutput);
	}
}

void returnSensorData()
{
	// Collect sensor data here?
	distance_travelled++;
	if (battery_voltage < 6)
	{
		battery_voltage += 0.2;
	}
	else
	{
		battery_voltage = 4;
	}
	String JSON_Data = String("{\"BTRY_VOLT\":") + battery_voltage + String(",\"ODO_DIST\":") + distance_travelled + "}";
	Serial.println(JSON_Data);
	websocketserver.broadcastTXT(JSON_Data);
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
	case WStype_TEXT:
	{
		Serial.printf("Client[%u] sent Text: %s\n", num, payload);
		String command = String((char *)(payload));

		DynamicJsonDocument doc(200);								//creating an instance of a DynamicJsonDocument allocating 200bytes on the heap.
		DeserializationError error = deserializeJson(doc, command); // deserialize 'doc' and parse for parameters we expect to receive.
		if (error)
		{
			Serial.print("deserializeJson() failed: ");
			Serial.println(error.c_str());
			return;
		}

		int MVM_F_status = doc["MVM_F"];
		int MVM_L_status = doc["MVM_L"];
		int MVM_R_status = doc["MVM_R"];
		int MVM_B_status = doc["MVM_B"];

		Serial.println('<' + MVM_F_status + ',' + MVM_B_status + ',' + MVM_L_status + ',' + MVM_R_status + '>');
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
	}
}
