// ------------------- blog.make2explore.com---------------------------------------------------------------//
// Project           - ESP32 LoRa Weather Station + Web Server
// Created By        - blog@make2explore.com
// Version - 1.0
// Software Platform - C/C++,HTML,CSS, Arduino IDE, Libraries
// Hardware Platform - TTGo LoRa ESP32 OLED v1, 0.96" OLED, Arduino MKRWAN 1300, BMP280 Sensor, DHT22 Sensor                
// Sensors Used      - BMP280 Atm Pressure Sensor, DHT22 Temperature Humidity Sensor
// Last Modified     - 31/03/2020 21:44:00
// -------------------------------------------------------------------------------------------------------//
// This is Receiver Node ESP32 LoRa Code - (WebServer)

// Import Wi-Fi library
#include <WiFi.h>
#include "ESPAsyncWebServer.h"

#include <SPIFFS.h>

//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Libraries to get time from NTP Server
#include <NTPClient.h>
#include <WiFiUdp.h>

//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 433E6

//OLED pins
#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Replace with your network credentials
const char* ssid     = "****";
const char* password = "****";

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String day;
String hour;
String timestamp;


// Initialize variables to get and save LoRa data
int rssi;
String loRaMessage;
String temperature;
String humidity;
String pressure;
String readingID;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return temperature;
  }
  else if(var == "HUMIDITY"){
    return humidity;
  }
  else if(var == "PRESSURE"){
    return pressure;
  }
  else if(var == "TIMESTAMP"){
    return timestamp;
  }
  else if (var == "RRSI"){
    return String(rssi);
  }
  return String();
}

//------------------Initialize OLED Module-------------------------------------------//
void initOLED(){
  //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("LORA RECEIVER");
}

//----------------------Initialize LoRa Module-----------------------------------------//
void initLoRA(){
  int counter;
  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);

  while (!LoRa.begin(BAND) && counter < 10) {
    Serial.print(".");
    counter++;
    delay(500);
  }
  if (counter == 10) {
    // Increment readingID on every new reading
    Serial.println("Starting LoRa failed!"); 
  }
  Serial.println("LoRa Initialization OK!");
  display.setCursor(0,10);
  display.clearDisplay();
  display.print("LoRa Initializing OK!");
  display.display();
  delay(2000);
}

//----------------------Connect to WiFi Function-----------------------------------------//
void connectWiFi(){
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  display.setCursor(0,20);
  display.print("Access web server at: ");
  display.setCursor(0,30);
  display.print(WiFi.localIP());
  display.display();
}

//-----------------Read LoRa packet and get the sensor readings-----------------------//
void getLoRaData() {
  Serial.print("Lora packet received: ");
  // Read packet
  while (LoRa.available()) {
    String LoRaData = LoRa.readString();
    // LoRaData format: readingID/temperature&soilMoisture#batterylevel
    // String example: 1/27.43&654#95.34
    Serial.print(LoRaData); 
    
    // Get readingID, temperature and soil moisture
    int pos1 = LoRaData.indexOf('/');
    int pos2 = LoRaData.indexOf('&');
    int pos3 = LoRaData.indexOf('#');
    readingID = LoRaData.substring(0, pos1);
    temperature = LoRaData.substring(pos1 +1, pos2);
    humidity = LoRaData.substring(pos2+1, pos3);
    pressure = LoRaData.substring(pos3+1, LoRaData.length());    
  }
  // Get RSSI
  rssi = LoRa.packetRssi();
  Serial.print(" with RSSI ");    
  Serial.println(rssi);
}

//-----------------------Function to get date and time from NTPClient------------------//
void getTimeStamp() {
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();
  Serial.println(formattedDate);

  // Extract date
  int splitT = formattedDate.indexOf("T");
  day = formattedDate.substring(0, splitT);
  Serial.println(day);
  // Extract time
  hour = formattedDate.substring(splitT+1, formattedDate.length()-1);
  Serial.println(hour);
  timestamp = day + " " + hour;
}

//-------------------------Display Readings on OLED-------------------------------------//
void displayReadings()
{
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("LoRa RECEIVER 1- Pune");
  display.setCursor(0,20);
  display.setTextSize(1);
  display.print("TEMPERATURE");
  display.setCursor(70,20);
  display.print(":");
  display.setCursor(80,20);
  display.print(temperature);
  display.drawCircle(116,20,1, WHITE);
  display.setCursor(121,20);
  display.print("C");
  display.setCursor(0,37);
  display.print("HUMIDITY(%)");
  display.setCursor(70,37);
  display.print(":");
  display.setCursor(80,37);
  display.print(humidity);
  display.setCursor(116,37);
  display.print("Rh");
  display.setCursor(0,55);
  display.print("PRESSURE");
  display.setCursor(50,55);
  display.print(":");
  display.setCursor(60,55);
  display.print(pressure);
  display.setCursor(100,55);
  display.print("mmHg");
  display.display();
  delay(1000);
}


void setup() { 
  // Initialize Serial Monitor
  Serial.begin(115200);
  initOLED();
  initLoRA();
  connectWiFi();
  
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", temperature.c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", humidity.c_str());
  });
  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", pressure.c_str());
  });
  server.on("/timestamp", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", timestamp.c_str());
  });
  server.on("/rssi", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(rssi).c_str());
  });
  server.on("/background", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/background.jpg", "image/jpg");
  });
  // Start server
  server.begin();
  
  // Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(19800);
}

void loop() {
  // Check if there are LoRa packets available
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    getLoRaData();
    displayReadings();
    getTimeStamp();
  }
}

//============================ www.blog.make2explore.com ==============================//