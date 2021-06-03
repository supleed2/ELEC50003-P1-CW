#include <Arduino.h>
#include <string>
// #include <SoftwareSerial.h> Software Serial not currently needed
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Ticker.h>
#include <WebSocketsServer.h>
#include <credentials.h>
#include <ArduinoJson.h>
#ifdef LOG_LOCAL_LEVEL
	#undef LOG_LOCAL_LEVEL
#endif
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#define RX1pin 14 // Pin 10 on expansion board, UART1
#define TX1pin 4  // Pin 11 on expansion board, UART1

// Function Declarations
void printFPGAoutput();
void returnSensorData();
void notFound(AsyncWebServerRequest *request);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);

// Global objects
AsyncWebServer webserver(80);
WebSocketsServer websocketserver(81);
Ticker ticker(returnSensorData, 500, 0, MILLIS);

// Global variables
int battery_voltage = 0;
int distance_travelled = 0;

#pragma region HTMLsource
char index_html[] PROGMEM = R"=====(

<!DOCTYPE html>
<html lang='en'>
<head>
<title>Rover Control Panel</title>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
* {
      box-sizing: border-box;
    }

.section_container {
  float: left;
  width: 50%;
  padding: 10px;
}
.flex-container {
  display: flex;
  flex-wrap: nowrap;
}
ul {
  list-style-type: none;
  margin: 0;
  padding: 0;
}

li {
  padding: 0px;
  margin-bottom: 0px;
}

:is(h1, h2, h3, h4, h5, h6, label, strong, meter) {
    font-family: Arial, Helvetica, sans-serif;
}
.movement_control {
    text-align: center;
}

.sensor_data {
    text-align: center;
}

meter{
    width: 100%;
    height: 40px;
    transform: translateY(-8px);
}

meter::after {
 content : attr(value) attr(title);
 top:-28px;
 left:0px;
 position:relative;
}

.button {
  display: inline-block;
  padding: 15px 25px;
  font-size: 24px;
  cursor: pointer;
  text-align: center;
  text-decoration: none;
  outline: none;
  color: rgb(255, 255, 255);
  background-color: #161616;
  border: none;
  border-radius: 15px;
  box-shadow: 0 9px rgb(161, 161, 161);
}

.button:hover {background-color: #585858}

.button:active {
  background-color: #107C10;
  box-shadow: 0 5px rgb(161, 161, 161);
  transform: translateY(4px);
}

.pressed {
  background-color: #107C10;
  box-shadow: 0 5px rgb(161, 161, 161);
  transform: translateY(4px);
}

.clearfix::after {
  content: "";
  clear: both;
  display: table;
}

</style>
<script>

var connection = new WebSocket('ws://'+location.hostname+':81/');

var MVM_F_status = 0;
var MVM_L_status = 0;
var MVM_R_status = 0;
var MVM_B_status = 0;

var BTRY_VOLT = 0;
var ODO_DIST = 0;

function round(value, precision) {
    var multiplier = Math.pow(10, precision || 0);
    return Math.round(value * multiplier) / multiplier;
}

connection.onmessage = function(event){
    var raw_data = event.data;
    console.log(raw_data);
    var data = JSON.parse(raw_data);
    digiBTRY_VOLT = data.BTRY_VOLT;
    BTRY_VOLT = round((digiBTRY_VOLT*(4.8e-4)+4), 1)
    ODO_DIST = data.ODO_DIST;
    document.getElementById("btry_meter").value = BTRY_VOLT;
    document.getElementById("Odometer").innerHTML = ODO_DIST;
}

function send_data()
{
  var raw_data = '{"MVM_F":'+MVM_F_status+',"MVM_L":'+MVM_L_status+',"MVM_R":'+MVM_R_status+',"MVM_B":'+MVM_B_status+'}';
  connection.send(raw_data);
  console.log(raw_data);
}

function left_pressed(){
    MVM_L_status = 1;
    send_data();
}
function left_unpressed(){
    MVM_L_status = 0;
    send_data();
}
function up_pressed(){
    MVM_F_status = 1;
    send_data();
}
function up_unpressed(){
    MVM_F_status = 0;
    send_data();
}
function right_pressed(){
    MVM_R_status = 1;
    send_data();
}
function right_unpressed(){
    MVM_R_status = 0;
    send_data();
}
function down_pressed(){
    MVM_B_status = 1;
    send_data();
}
function down_unpressed(){
    MVM_B_status = 0;
    send_data();
}

var timer = null;

function up_mouseDown() {
  timer = setInterval(up_pressed, 100);
}
function up_mouseUp() {
    clearInterval(timer);
    up_unpressed();
}

function down_mouseDown() {
  timer = setInterval(down_pressed, 100);
}
function down_mouseUp() {
    clearInterval(timer);
    down_unpressed();
}

function right_mouseDown() {
  timer = setInterval(right_pressed, 100);
}
function right_mouseUp() {
    clearInterval(timer);
    right_unpressed();
}

function left_mouseDown() {
  timer = setInterval(left_pressed, 100);
}
function left_mouseUp() {
    clearInterval(timer);
    left_unpressed();
}


document.onkeydown = function(e) {
    switch (e.keyCode) {
        case 37:
            document.getElementById("left_arrow").className = "button pressed";
            left_pressed();
            break;
        case 38:
            document.getElementById("up_arrow").className = "button pressed";
            up_pressed();
            break;
        case 39:
            document.getElementById("right_arrow").className = "button pressed";
            right_pressed();
            break;
        case 40:
            document.getElementById("down_arrow").className = "button pressed";
            down_pressed();
            break;
    }
};
document.onkeyup = function(e) {
    switch (e.keyCode) {
        case 37:
            document.getElementById("left_arrow").className = "button";
            left_unpressed();
            break;
        case 38:
            document.getElementById("up_arrow").className = "button";
            up_unpressed();
            break;
        case 39:
            document.getElementById("right_arrow").className = "button";
            right_unpressed();
            break;
        case 40:
            document.getElementById("down_arrow").className = "button";
            down_unpressed();
            break;
    }
};

</script>
</head>
<body>

<h1 style="text-align:center;">ROVER COMMAND CENTER</h1>

<div class="clearfix">

    <div class="section_container">
    <div class ="movement_control">
        <h2>Movement Control</h2>
        <div style="transform: translateY(0px);">
            <button id="up_arrow" onmousedown="up_mouseDown()" onmouseup="up_mouseUp()" class="button" ><span>&#8679;</span></button>
        </div>
        <div style="transform: translateY(13px);">
            <button id="left_arrow" onmousedown="left_mouseDown()" onmouseup="left_mouseUp()" class="button"><span>&#8678;</span></button>
            <button id="down_arrow" onmousedown="down_mouseDown()" onmouseup="down_mouseUp()" class="button"><span>&#8681;</span></button>
            <button id="right_arrow" onmousedown="right_mouseDown()" onmouseup="right_mouseUp()" class="button"><span>&#8680;</span></button>
        </div>
        
    </div>
    </div>

    <div class="section_container">
        <div id="bleh" class="sensor_data">
            <h2>Sensor Data</h2>
            <ul>

                <li><div class="section_container">
                    <label>Battery Voltage</label>
                </div>
                <div class="section_container">
                    <meter id="btry_meter" min="4.0" max="6.0" low ="4.5" optimum="5.0" high="4.8" value="5.8" title="V"></meter>
                </div>
                </li>

                
                <li><div class="section_container">
                    <label>Odometer</label>
                </div>
                <div class="section_container">
                    <strong id="Odometer">28</strong><strong>mm</strong>
                </div>
                </li>

            </ul>        
        </div>
        
    </div>

</div>

</body>
</html>


)=====";
#pragma endregion

void setup()
{
	esp_log_level_set("*", ESP_LOG_ERROR);        // set all components to ERROR level
	esp_log_level_set("wifi", ESP_LOG_WARN);      // enable WARN logs from WiFi stack
	esp_log_level_set("dhcpc", ESP_LOG_INFO);     // enable INFO logs from DHCP client

	Serial.begin(115200);							 // Set up hardware UART0 (Connected to USB port)
	Serial1.begin(9600, SERIAL_8N1, RX1pin, TX1pin); // Set up hardware UART1
	// Set up remaining communication ports here (Energy, Drive, Vision)

	Serial.println("Connecting to AP");
	WiFi.begin(WIFI_SSID, WIFI_PW);
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	Serial.println("\nConnected to AP");
	while (!MDNS.begin("rover"))
	{
		Serial.println("Error setting up mDNS, retrying in 5s");
		delay(5000);
	}
	Serial.println("mDNS set up, access Control Panel at 'rover.local/'");

	webserver.on("/", [](AsyncWebServerRequest *request)
				 { request->send_P(200, "text/html", index_html); });
	webserver.onNotFound(notFound);
	webserver.begin();

	websocketserver.begin();
	websocketserver.onEvent(webSocketEvent);
	ticker.start();
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
	String JSON_Data = String("{\"BTRY_VOLT\":") + battery_voltage + String(",\"ODO_DIST\":") + distance_travelled + "}";
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
		default:
		{
			Serial.println(String("Websocket received invalid event type: ") + type + String(", exiting"));
			exit(1);
		}
	}
}
