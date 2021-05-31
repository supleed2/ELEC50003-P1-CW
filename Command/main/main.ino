#include <WiFi.h>
#include <SPIFFS.h>
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

    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;
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
  
  if(!SPIFFS.begin(true)){
  Serial.println("An Error has occurred while mounting SPIFFS");
  return;
}
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

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(SPIFFS, "/index.html", "text/html");
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(SPIFFS, "/favicon.ico", "image/png");
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
