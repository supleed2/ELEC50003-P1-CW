#include <WiFi.h>
#include<ESPmDNS.h>
#include <WebSocketsServer.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "Ticker.h"

const int potPin = 34; //used to simulate battery voltage.
const int butPin = 16; //used to increment a variable to simulate distance increasing.
const int U_Led = 14; //LED subsitute for the 'movement forward command'.
const int L_Led = 12; //LED subsitute for the 'movement left command'.
const int R_Led = 15; //LED subsitute for the 'movement right command'.
const int D_Led = 13; //LED subsitute for the 'movement back command'.

/* const char* ssid     = "ssid";
const char* password = "xxxxxxxx"; */

int potVal = 0;
bool butState = 1;  //Variables only for testing - will be removed in final

int d = 0; //Initializing variable for odometer distance.

void send_sensor();
Ticker timer;

char index_html[] PROGMEM = R"=====(

<!DOCTYPE html>
<html lang='en'>
<head>
<title>Rover Command Center</title>
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

AsyncWebServer server(80); // server port 80 for initial HTTP request for the main webpage.
WebSocketsServer websockets(81); // server port 81 for real time data flow through websockets.

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Page Not found. Check URI/IP address.");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch (type) 
  {
    case WStype_DISCONNECTED:
      Serial.printf("Client[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
        IPAddress ip = websockets.remoteIP(num);
        Serial.printf("Client[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT: {
      Serial.printf("Client[%u] sent Text: %s\n", num, payload);
      String command = String((char*)( payload));
      
      DynamicJsonDocument doc(200); //creating an instance of a DynamicJsonDocument allocating 200bytes on the heap.
      DeserializationError error = deserializeJson(doc, command); // deserialize 'doc' and parse for parameters we expect to receive.
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }
    
      int MVM_F_status = doc["MVM_F"];
      int MVM_L_status = doc["MVM_L"];
      int MVM_R_status = doc["MVM_R"];
      int MVM_B_status = doc["MVM_B"];
      
      digitalWrite(U_Led,MVM_F_status);
      digitalWrite(L_Led,MVM_L_status);
      digitalWrite(R_Led,MVM_R_status);
      digitalWrite(D_Led,MVM_B_status);
    }
  }
}

void setup()
{
  
  Serial.begin(115200);
  pinMode(U_Led,OUTPUT);
  pinMode(L_Led,OUTPUT);
  pinMode(R_Led,OUTPUT);
  pinMode(D_Led,OUTPUT);
  pinMode(butPin, INPUT_PULLUP);
  

/*   Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("Rover connected to ");
    Serial.println(ssid);
    Serial.println();
    Serial.println("Rover IP address: ");
    Serial.println(WiFi.localIP()); */

  WiFi.softAP("RoverAP", "SplendidCheeks");
  Serial.println();
  Serial.println("RoverAP running");
  Serial.print("Rover IP address: ");
  Serial.println(WiFi.softAPIP());


  if (!MDNS.begin("rover")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(2000);
    }
  }
  Serial.println("mDNS responder started! Rover Command Center can now be accessed at 'rover.local' ");



  server.on("/", [](AsyncWebServerRequest * request)
  {  
    request->send_P(200, "text/html", index_html);
  });

  server.onNotFound(notFound);

  server.begin();
  websockets.begin();
  websockets.onEvent(webSocketEvent);
  timer.attach(0.5,send_sensor_data);
}

void loop()
{
 websockets.loop();
 potVal = analogRead(potPin);
}

void send_sensor_data()
{
 
  butState = digitalRead(butPin);
  if (butState == LOW) {
      //increment ODO:
      d += 10;
      }
  // JSON_Data = {"BTRY_VOLT":v,"ODO_DIST":d}
  String JSON_Data = "{\"BTRY_VOLT\":";
         JSON_Data += potVal;
         JSON_Data += ",\"ODO_DIST\":";
         JSON_Data += d;
         JSON_Data += "}";     
  websockets.broadcastTXT(JSON_Data);
}