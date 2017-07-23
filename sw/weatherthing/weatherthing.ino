#include <BME280I2C.h>
#include "SSD1306.h"

//Wi-Fi
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

//Webserver and page to be served included in PROGMEM within guihtml.h
#include <ESP8266WebServer.h>
#include "guihtml.h"

//mDNS
#include <ESP8266mDNS.h>

//EEPROM
#include <EEPROM.h>

//OTA
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

//OLED Screen
SSD1306  display(0x3c, 4, 5);
char screen_buf[100]="";

//WiFi configuration
#define STRLEN 32
char ssid1[STRLEN] = "";
char pass1[STRLEN] = "";
char ssid2[STRLEN] = "";
char pass2[STRLEN] = "";
char hostn[STRLEN] = "";

//Thingspeak stuff
#include "ThingSpeak.h"
char channel[STRLEN] = "";
char apikey[STRLEN] = "";

//JSON Allocation
#include <ArduinoJson.h>
StaticJsonBuffer<300> jsonBuffer;
JsonObject& jsonroot = jsonBuffer.createObject();
char charbuffer[300];

WiFiClient  client;

ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer webServer = ESP8266WebServer(80);

//Temperature sensors and variables
BME280I2C bme_i(1,1,1,3,5,0,0,0x77);
BME280I2C bme_o(1,1,1,3,5,0,0,0x76);

uint8_t bme_i_detected=0;
uint8_t bme_o_detected=0;
uint8_t pressure_unit=1; //hPa
bool metric=true;

float temp(NAN), hum(NAN), pres(NAN), alt(NAN), dew(NAN);

uint32_t current_micros=0;
uint32_t period=0;
uint32_t last_update=0;

void setup() {
  int retries = 0;
  
  Serial.begin(115200);
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  Serial.println();
  bme_i_detected = bme_i.begin();
  bme_o_detected = bme_o.begin();
  Serial.print("Inside :");
  Serial.println(bme_i_detected);
  Serial.print("Outside :");
  Serial.println(bme_o_detected);
  ThingSpeak.begin(client);
  
  //Load WiFi
  
  Serial.println("Loading Wi-Fi config");
  EEPROM.begin(512);
  
  EEPROM.get(0*STRLEN, ssid1);
  EEPROM.get(1*STRLEN, pass1);
  EEPROM.get(2*STRLEN, ssid2);
  EEPROM.get(3*STRLEN, pass2);
  EEPROM.get(4*STRLEN, hostn);
  EEPROM.get(5*STRLEN, channel);
  EEPROM.get(6*STRLEN, apikey);
  Serial.println(ssid1);
  Serial.println(pass1);
  Serial.println(ssid2);
  Serial.println(pass2);
  Serial.println(hostn);
  Serial.println(channel);
  Serial.println(apikey);
  WiFi.hostname(hostn);
  WiFi.mode(WIFI_AP_STA);
  WiFiMulti.addAP(ssid1, pass1);
  WiFiMulti.addAP(ssid2, pass2);

  while(retries < 20){
    if(WiFiMulti.run() == WL_CONNECTED) { 
      break;
    }
    delay(500);
    Serial.println("Attempting Wi-Fi connection");
    retries++;
  }
  
  if(WiFiMulti.run() == WL_CONNECTED){
    WiFi.mode(WIFI_STA);
    Serial.println("");
    Serial.println("Wi-Fi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    ArduinoOTA.setHostname(hostn);
  } else {
    WiFi.softAP("WEATHER-AP");
    Serial.println("");
    Serial.println("WiFi AP Started");
    Serial.println("IP address: ");
    Serial.println(WiFi.softAPIP());
  }
  
  ArduinoOTA.onStart([]() {
    Serial.println("\nStart");
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
    ESP.restart();
  });
  
  ArduinoOTA.begin();
  Serial.println("OTA service started");
  
  webServer.on("/", []() {
    webServer.send(200, "text/html", GUIHTML);
  });
  webServer.on("/update.js", []() {
    size_t size = jsonroot.printTo(charbuffer, sizeof(charbuffer));
    webServer.send(200, "text/json", charbuffer);
  });
  webServer.on("/config.js", []() {
    String output = "var ssid1='" + String(ssid1) + "';";
    output += "var pass1='" + String(pass1) + "';";
    output += "var ssid2='" + String(ssid2) + "';";
    output += "var pass2='" + String(pass2) + "';";
    output += "var hostname='" + String(hostn) + "';";
    output += "var channel='" + String(channel) + "';";
    output += "var apikey='" + String(apikey) + "';";
    webServer.send(200, "text/json", output);
  });
  webServer.on("/configwifi", []() {
    Serial.println("Saving WiFi Config");
    webServer.arg("ssid1").toCharArray(ssid1, sizeof(ssid1)-1);
    webServer.arg("pass1").toCharArray(pass1, sizeof(pass1)-1);
    webServer.arg("ssid2").toCharArray(ssid2, sizeof(ssid2)-1);
    webServer.arg("pass2").toCharArray(pass2, sizeof(pass2)-1);
    webServer.arg("hostname").toCharArray(hostn, sizeof(hostn)-1);
    webServer.arg("channel").toCharArray(channel, sizeof(channel)-1);
    webServer.arg("apikey").toCharArray(apikey, sizeof(apikey)-1);
    
    EEPROM.put(0*STRLEN, ssid1);
    EEPROM.put(1*STRLEN, pass1);
    EEPROM.put(2*STRLEN, ssid2);
    EEPROM.put(3*STRLEN, pass2);
    EEPROM.put(4*STRLEN, hostn);
    EEPROM.put(5*STRLEN, channel);
    EEPROM.put(6*STRLEN, apikey);
    EEPROM.commit();
    EEPROM.end();
    webServer.send(200, "text/html", "Config saved - Rebooting, please reconnect.");
    Serial.println("Rebooting...");
    ESP.reset();
  });
  webServer.begin();
    
  if(MDNS.begin(hostn)) {
    Serial.println("MDNS responder started");
  }
  MDNS.addService("http", "tcp", 80);
  
}

void loop() {
  webServer.handleClient();
  ArduinoOTA.handle();
  
  current_micros = millis();
  period = current_micros - last_update;
  if(period > 30000){
    last_update = current_micros;
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 0, "Shauna");
    display.drawString(0,16,"Temp");
    display.drawString(0,32,"Humidity");
    display.drawString(0,48,"Pressure");
  
    if(bme_i_detected){
      bme_i.read(pres, temp, hum, metric, pressure_unit);
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.drawString(85, 0, "Inside");
      display.drawString(85,16,String(temp));
      display.drawString(85,32,String(hum));
      display.drawString(85,48,String(pres));
      ThingSpeak.setField(1, temp);
      ThingSpeak.setField(2, hum);
      ThingSpeak.setField(3, pres);
      jsonroot["temp_i"] = temp;
      jsonroot["hum_i"] = hum;
      jsonroot["pres_i"] = pres;
    }
    if(bme_o_detected){
      bme_o.read(pres, temp, hum, metric, pressure_unit);
      display.drawString(128, 0, "Outside");
      display.drawString(128,16,String(temp));
      display.drawString(128,32,String(hum));
      display.drawString(128,48,String(pres));
      ThingSpeak.setField(4, temp);
      ThingSpeak.setField(5, hum);
      ThingSpeak.setField(6, pres);
      jsonroot["temp_o"] = temp;
      jsonroot["hum_o"] = hum;
      jsonroot["pres_o"] = pres;
    }

    //sprintf(screen_buf, "Temp: %3.2f\nHumidity: %3.2f\nPressure: %5.2f", temp, hum, pres);
    sprintf(screen_buf, "Temp: %d.%02d\nHumidity: %2d\nPressure: %4d", (int)temp,(int)(temp*100)%100, (int)hum, (int)pres);
    Serial.println(screen_buf);
    
    display.display();
    if( (atol(channel) != 0) && (apikey != "")){
      Serial.println("Updating ThingSpeak");
      Serial.println(atol(channel));
      Serial.println(apikey);
      ThingSpeak.writeFields(atol(channel), apikey);
    }
  }
}
