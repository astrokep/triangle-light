#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h> 
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>   // Include the WebServer library
#include <FS.h>
#include <ArduinoJson.h>

// #include "config.h"
#include "led_control.hpp"

ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

ESP8266WebServer server(80);

#define NUM_TRIS 2
#define LEDS_PER_TRI 3

LEDStrip s1(NUM_TRIS * LEDS_PER_TRI, 10, 200);

String getContentType(String filename); // convert the file extension to the MIME type
bool handleFileRead(String path);       // send the right file to the client (if it exists)

StaticJsonDocument<200> doc;
std::unique_ptr<char[]> buf;

const char* WIFI_SSID = NULL;
const char* WIFI_PASS = NULL; 

const char* OTA_NAME = NULL;
const char* OTA_PASS = NULL;

const char* MDNS_NAME = NULL;



////////////////////////////
//JSON Config file loading:
bool loadConfig(){
  File configFile = SPIFFS.open("/config.json", "r");
  if(!configFile){
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if(size > 1024){
    Serial.println("Config file too large");
    return false;
  }

  buf = std::unique_ptr<char[]>(new char[size]);

  configFile.readBytes(buf.get(), size);

  // StaticJsonDocument<200> doc;
  auto error = deserializeJson(doc, buf.get());
  if (error) {
    Serial.println("Failed to parse config file");
    return false;
  } 

  WIFI_SSID = doc["WIFI_SSID"];
  WIFI_PASS = doc["WIFI_PASS"];

  OTA_NAME = doc["OTA_NAME"];
  OTA_PASS = doc["OTA_PASS"];

  MDNS_NAME = doc["MDNS_NAME"];

  Serial.println("Config Data:");
  Serial.println(WIFI_SSID);
  Serial.println(WIFI_PASS);
  Serial.println(OTA_NAME);
  Serial.println(OTA_PASS);
  Serial.println(MDNS_NAME);
  
  return true;
}



////////////////////////////


void handleRoot() {
  server.send(200, "text/html", "<form action=\"/LED\" method=\"POST\">\
              <input type=\"text\" name=\"triangle\" placeholder=\"0\"></br>\
              <input type=\"text\" name=\"red\" placeholder=\"0\"></br>\
              <input type=\"text\" name=\"green\" placeholder=\"0\"></br>\
              <input type=\"text\" name=\"blue\" placeholder=\"0\"></br>\
              <input type=\"submit\" value=\"update\"></form>\
              <form action=\"/LED_Randomize\" method=\"POST\">\
              <input type=\"submit\" value=\"randomize\"></form>");
}

void handleLED(){
  int t = server.arg("triangle").toInt();
  byte r = server.arg("red").toInt();
  byte g = server.arg("green").toInt();
  byte b = server.arg("blue").toInt();
  Serial.print("R: ");
  Serial.println(r);
  Serial.print("G: ");
  Serial.println(g);
  Serial.print("B: ");
  Serial.println(b);
  s1.setSubStripTargets(t * LEDS_PER_TRI, (t + 1) * LEDS_PER_TRI, r, g, b);
  s1.update();
  server.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303); 
}

void handleLEDRandom(){
  for(int i = 0; i < NUM_TRIS; ++i){
    s1.randomizeSubStrip(i * LEDS_PER_TRI, (i + 1) * LEDS_PER_TRI);
  }
  server.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303); 
}

void setup() {
  s1.init();
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');
  if(!SPIFFS.begin()){
    Serial.println("Loading FS failed");
  }
  else{
    loadConfig();
  }
  wifiMulti.addAP(WIFI_SSID, WIFI_PASS);   // add Wi-Fi networks you want to connect to

  Serial.println("Connecting ...");
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(250);
    Serial.print('.');
  }
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer

  ArduinoOTA.setHostname(OTA_NAME);
  ArduinoOTA.setPassword(OTA_PASS);
  ArduinoOTA.onStart([]() {
      Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready");

  if (MDNS.begin(MDNS_NAME)) {              // Start the mDNS responder for esp8266.local
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  server.on("/", HTTP_GET, handleRoot);               // Call the 'handleRoot' function when a client requests URI "/"
  server.on("/LED", HTTP_POST, handleLED);
  server.on("/LED_Randomize", HTTP_POST, handleLEDRandom);
  server.onNotFound([]() {                              // If the client requests any URI
    if (!handleFileRead(server.uri()))                  // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.begin();                           // Actually start the server
  Serial.println("HTTP server started");
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  s1.update();
}


String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    size_t sent = server.streamFile(file, contentType); // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  Serial.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
}