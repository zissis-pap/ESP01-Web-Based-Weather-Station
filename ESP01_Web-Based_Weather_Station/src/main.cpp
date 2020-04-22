/*--------------------------------------------------------------------
ESP01 WEB-BASED WEATHER STATION, Version 1 
THIS VERSION IS BASED ON THE ARDUINO WEB-BASED WEATHER STATION PROJECT
Author: Zissis P. Papadopoulos - zissis.papadopoulos@gmail.com
Patras, April 2020
----------------------------------------------------------------------
  Added features: 1) Arduino OTA update support 
                  2) NTP based clock instead of RTC1302
                  3) Exports Json-formatted data through serial
  ****** THIS PROJECT IS FINISHED ******
*/
#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include "DHT.h" // Use DHT11 sensor library

const char *ssid     = "******";
const char *password = "******";
ESP8266WebServer server(80);
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Define DHT Sensor
#define DHTPIN 2     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

// Define BuiltIn LED
#define LED_BUILTIN 1

// Weather Station Variables
uint8_t Hours[24] {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float TempHist[24] {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float HumHist[24] {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float TempDHist[7] {0,0,0,0,0,0,0};
float HumDHist[7] {0,0,0,0,0,0,0};
float Temperature, Humidity;

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

// Store day and month names
const String Days[7] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"}; // Store Day names
const String Months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

// Server Functions
void handleRoot();
void handleNotFound();
String SendHTML();
void SendJSONPackage();
// Time Functions
void UpdateTime();
// Conditions Functions
void MeasureCond();
void ConditionCalculations();
// Display Functions
String DateAndTime(boolean x);

// Functions
void Time(void);
int getMonth(); // returns month in integer form 1-12
int getMonthDay(); // returns day of the month in integer form 1 - 31
int getYear(); // returns year in integer form 2020+

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay (500);
    Serial.print (".");
  }
  if (MDNS.begin("weather_station", WiFi.localIP())) {
    Serial.println(F("MDNS responder started"));
  }
  ArduinoOTA.setPassword("STATION");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  MDNS.addService("http", "tcp", 80);
  ArduinoOTA.begin();
  // Initialize a NTPClient to get time
  timeClient.begin();
  timeClient.setTimeOffset(10800);
  dht.begin();
  
}

void loop() {
  ArduinoOTA.handle();
  UpdateTime();
  MeasureCond();
  ConditionCalculations();
  server.handleClient();
  MDNS.update();
  SendJSONPackage();
}

String SendHTML() {
  String ptr =(F("<!DOCTYPE HTML><html><head><title>ESP01 WebBased Weather Station</title>"));
  ptr += (F("<meta name='viewport' content='width=device-width, initial-scale=1.0'><style>"));
  ptr += (F("@charset 'UTF-8';@import url(https://fonts.googleapis.com/css?family=Open+Sans:300,400,700);"));
  ptr += (F("body {background-color: black;font-family: 'Open Sans', sans-serif;line-height: 1em;"));
  ptr += (F("text-align: center;Color: #cccccc;}table {font-family: arial, sans-serif;border-collapse: collapse;"));
  ptr += (F("width: 100%;margin-left:auto;margin-right:auto;}th {border: 1px solid #336699;color:#0040ff;"));
  ptr += (F("text-align: center;padding: 8px;}td {border: 1px solid #336699;color:#0080ff;text-align: center;"));
  ptr += (F("padding: 8px;}td.td2 {border: 1px solid #336699;color:#0080ff;text-align: left;padding: 8px;}"));
  ptr += (F("</style><script>setInterval(loadDoc,1000);function loadDoc() {var xhttp = new XMLHttpRequest();"));
  ptr += (F("xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200) {"));
  ptr += (F("document.body.innerHTML =this.responseText}};xhttp.open('GET', ' ', true);xhttp.send();}"));
  ptr += (F("</script><h1>ESP01 Web-Based Weather Station</h1><hr><h3>WELCOME!</h3><p>Project is hosted on "));
  ptr += (F("github. Please visit my <a href='https://github.com/zissis-pap'>page</a> for more!</p><hr></head>"));
  ptr += (F("<body><br><table style='width:60%'><tr><th colspan='4'>DATE, TIME AND ROOM CONDITIONS</th></tr>"));
  ptr += (F("<tr><td>Date &#128197</td><td>Time &#128336</td><td>Temperature &#x1F321</td><td>Humidity &#x2614"));
  ptr += (F("</td></tr><tr><td><font color='00ff00'>"));
  ptr += DateAndTime(true);
  ptr += (F("</font></td><td><font color='00ff00'>"));
  ptr += DateAndTime(false);
  ptr += (F("</font></td><td><font color='00ff00'>"));
  ptr += Temperature;
  ptr += (F("</font></td><td><font color='00ff00'>"));
  ptr += Humidity;
  ptr += (F("</font></td></tr></table><hr><h3>HISTORY</h3><table style='width:100%'><tr>"));
  ptr += (F("<th colspan='25'>AVERAGE CONDITIONS FOR THE LAST 24 HOURS</th></tr><tr><td class='td2'>HOURS:</td>"));
  for (uint8_t i = 0; i <= 23; i++) {
    ptr += (F("<td><font color='#3d0099'>"));
    ptr += Hours[i];
    ptr += (F(":00</font></td>"));
  }
  ptr += (F("</tr><tr><td class='td2'>TEMPERATURE (&#8451):</td>"));
  for (uint8_t i = 0; i <= 23; i++) {                
    ptr += (F("<td><font color='#3d0099'>"));
    ptr += TempHist[i];             
    ptr += (F("</font></td>"));
  }           
  ptr += (F("</tr><tr><td class='td2'>HUMIDITY (%):</td>"));
  for (uint8_t i = 0; i <= 23; i++) {
    ptr += (F("<td><font color='#3d0099'>"));
    ptr += HumHist[i];
    ptr += (F("</font></td>"));
  }
  ptr += (F("</tr></table><br><table style='width:60%'><tr>"));
  ptr += (F("<th colspan='8'>AVERAGE CONDITIONS FOR THE LAST 7 DAYS</th></tr><tr><td class='td2'>DAY:</td>"));
  for (uint8_t i = 0; i <= 6; i++) {
    ptr += (F("<td><font color='#3d0099'>"));
    ptr += Days[i];
    ptr += (F("</font></td>"));
  }            
  ptr += (F("</tr><tr><td class='td2'>TEMPERATURE:</td>"));
  for (uint8_t i = 0; i <= 6; i++) {
    ptr += (F("<td><font color='#3d0099'>"));
    ptr += TempDHist[i];
    ptr += (F("&#8451</font></td>"));
  }               
  ptr += (F("</tr><tr><td class='td2'>HUMIDITY:</td>"));
  for (uint8_t i = 0; i <= 6; i++) {
    ptr += (F("<td><font color='#3d0099'>"));
    ptr += HumDHist[i];
    ptr += (F("%</font></td>"));
  }
  ptr += (F("</tr></table><hr><br><p align='left'>Free Memory: "));
  ptr += (float)ESP.getFreeHeap()/1024;
  ptr += (F(" KB</p><hr><h3>ABOUT THE PROJECT</h3>"));
  ptr += (F("<p align='right'>Author: Zissis Papadopoulos @2020</p></body></html>"));
  return ptr;
}

void handleRoot() {
  digitalWrite(LED_BUILTIN, 1);
  server.send(200, "text/html", SendHTML()); // Send web page
  digitalWrite(LED_BUILTIN, 0);
}

void handleNotFound() {
  digitalWrite(LED_BUILTIN, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(LED_BUILTIN, 0);
}

void SendJSONPackage() {
  static unsigned long check{0};
  if (check <= millis() - 5000) {
    StaticJsonDocument<1000> data;
    data["hour"] = timeClient.getHours();
    data["minutes"] = timeClient.getMinutes();
    data["temp"] = Temperature;
    data["hum"] = Humidity;
    data["ip"] = WiFi.localIP().toString();
    JsonArray hoursHist = data.createNestedArray("hoursHist");
    JsonArray tempHist = data.createNestedArray("tempHist");
    JsonArray humHist = data.createNestedArray("humHist");
    for (uint8_t i = 0; i <= 23; i++) {
      hoursHist.add(Hours[i]);
      tempHist.add(TempHist[i]);
      humHist.add(humHist[i]);
    }
    serializeJsonPretty(data, Serial);
    check = millis();
  }
}

void MeasureCond() { // Measure temperature and humidity every 2 seconds
  static unsigned long check {0};
  if (check <= millis() - 2000) {
    Temperature = dht.readTemperature();
    Humidity = dht.readHumidity();
    check = millis();
  }
}

void ConditionCalculations() { //
  static float tempTemp {0}; // Buffer to add temperature every 5 minutes
  static float tempHum {0}; // Buffer to humidity every 5 minutes
  static uint8_t minutesIndex = timeClient.getMinutes(); // Stores last minute
  static uint8_t hoursIndex = timeClient.getHours(); // Stores last hour
  static uint8_t dayIndex = timeClient.getDay(); // Stores last day
  static uint8_t CalcCount{0}; // Measure number of calculations per hour
  static uint8_t index{0}; // Hourly matrix index
  static uint8_t HoursCount{0}; // Measure number of calculations per day
  static unsigned long check {0};
  if (check <= millis() - 2000) {
    if (minutesIndex == timeClient.getMinutes() - 5) { // If 5 minutes have passed since last calculation
      tempTemp += Temperature; // Add current temperature to the temporary buffer
      tempHum += Humidity; // Add current humidity to the temporary buffer
      CalcCount++; // Add one to the calculation counter
      minutesIndex = timeClient.getMinutes(); // Reset the minutes counter
    }
    if (timeClient.getHours() != hoursIndex) { // If one hour passed
      /* If hour changes before any measurements were taken CalcCount will remain 0 and
         the division will be impossible. As such, we want to avoid calculations when CalcCount = 0 */
      if (CalcCount > 0) {
        if (index == 24) { // if index has passed array limits
          for (uint8_t i = 0; i <= 22; i++) { // shift data left
            TempHist[i] = TempHist[i+1];
            HumHist[i] = HumHist[i+1];
            Hours[i] = Hours[i+1];
            index = 23;
          }
        }
        TempHist[index] = tempTemp/CalcCount; // Calculate average temperature
        HumHist[index] = tempHum/CalcCount; // Calculate average humidity
        Hours[index] = timeClient.getHours(); // Store the hour when the calculations were made
        tempTemp = 0; // Reset temperature buffer
        tempHum = 0; // Reset humidity buffer
        CalcCount = 0; // Reset measurements counter
        minutesIndex = timeClient.getMinutes(); // Reset the minutes counter
        hoursIndex = timeClient.getHours(); // Reset the hours counter
        index++; // Increase array index
        HoursCount++; // Add one to the hours counter
        if (dayIndex != timeClient.getDay()) { // Check if day changed as well
          float indexTemp {0};
          float indexHum {0};
          for (uint8_t i = (index - HoursCount); i <= index - 1; i++) {
            indexTemp += TempHist[i];
            indexHum += HumHist[i];
          }
          TempDHist[dayIndex-1] = indexTemp/HoursCount; // Calculate average daily temperature
          HumDHist[dayIndex-1] = indexHum/HoursCount; // Calculate average daily humidity
          //DataLogger(index, HoursCount);
          HoursCount = 0; // Reset Hours counter
          dayIndex = timeClient.getDay(); // Reset the days counter
        }
      }
      else {
        minutesIndex = timeClient.getMinutes();
        hoursIndex = timeClient.getHours();
        dayIndex = timeClient.getDay();
      }
    }    
    check = millis();
  }
}

void UpdateTime() {
  static unsigned long PassedTime{0}; // Variable to check passed time without using delay
  
  if (PassedTime <= millis() - 1000) { // update time once per second
    while(!timeClient.update()) { 
      timeClient.forceUpdate();
    }
    PassedTime = millis();
  }
}

int getMonthDay() {
  int monthday;
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  monthday = (ti->tm_mday) < 10 ? 0 + (ti->tm_mday) : (ti->tm_mday);
  return monthday;
}

int getMonth() {
  int month;
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  month = (ti->tm_mon + 1) < 10 ? 0 + (ti->tm_mon + 1) : (ti->tm_mon + 1);
  return month;
}

int getYear() {
  int year;
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  year = ti->tm_year + 1900;
  return year;
}

String DateAndTime(boolean x) {
  if (x) {
    String buf = "";
    buf += Days[timeClient.getDay() - 1];
    buf += " the ";
    buf += getMonthDay();
    if (getMonthDay() == 1) buf += "st";
    else if (getMonthDay() == 2) buf += "nd";
    else if (getMonthDay() == 3) buf += "rd";
    else buf += "th";
    buf += " of ";
    buf += Months[getMonth() - 1];
    buf += " ";
    buf += getYear();
    return buf;
  }
  if (!x) {
    String buf = "";
    buf += timeClient.getHours();
    buf += ":";
    if (timeClient.getMinutes() < 10) buf +="0";
    buf += timeClient.getMinutes();
    return buf;
  }
}
