#include <Timer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <PusherClient.h>
#include <stdlib.h>

Timer timer;
PusherClient client;
WiFiClient wifi;

#define ONE_WIRE_BUS 3
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

int MOISTURE_SENSOR = 0; // Pin for the Moisture Sensor
int MAX_TEMP = 85;
int MIN_TEMP = 40;
int MIN_WATER = 300;
int MAX_WATER = 700;
int status = WL_IDLE_STATUS;     // the Wifi radio's status
int moisture; // sensor reading for moisture. 0 - 950

String alertMsg, alertBody; // Buffers for alert messages.

char ssid[] = "ECTO-1";     //  your network SSID (name) 
char pass[] = "WhoUgonnaCall?";    // your network password
char server[] = "[your ngrok url]"; // Enter the url you get when running ngrok
char PUSHER_KEY[]= "[your pusher api key]";
char PUSHER_CHANNEL[]= "garduino";
char tempStr[6]; // buffer for temp incl. decimal point & possible minus sign
char waterLvl[6];

void setupWifiPusher() {
  status = WiFi.begin(ssid, pass);
  if ( status != WL_CONNECTED) { 
    while(true);
  } 
  else {
    if(client.connect(PUSHER_KEY)) {
      client.subscribe(PUSHER_CHANNEL);
      Serial.print("Looks like we connected");
    }
    else {
      while(1) {
        Serial.print("Did not connect");
      }
    }
  }
}

void setup(void)
{
  Serial.begin(9600);
  setupWifiPusher();
  sensors.begin();
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 
  sensors.setResolution(insideThermometer, 9);
  timer.every(10 * 60 * 1000, checkSensors); // 10 minutes
}

void checkTemperature(DeviceAddress deviceAddress)
{
  float temp = sensors.getTempF(deviceAddress);
  dtostrf(temp, 6, 2, tempStr); // Min. 6 chars wide incl. decimal point, 2 digits right of decimal
  if (temp > MAX_TEMP) {
    alertBody = "alert=too_hot&temp=";
  } else if (temp < MIN_TEMP) {
    alertBody = "alert=too_cold&temp=";
  }
  alertMsg = alertBody + tempStr;
  sendStatus(alertMsg);
}

void checkMoisture()
{
  moisture = analogRead(MOISTURE_SENSOR);
  if (moisture < MIN_WATER) {
    alertBody = "alert=water_low&moisture=";
  }
  alertMsg = alertBody + moisture;
  sendStatus(alertMsg);
}

void checkSensors() {
  checkTemperature(insideThermometer); 
  checkMoisture();
}

void loop(void)
{ 
  monitorPusher();
  timer.update();
}

void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void sendStatus(String data) {
  if (wifi.connect(server, 80)) {
    wifi.println("GET /arduino-data?"+ data +" HTTP/1.1");
    wifi.println("Host: garduino-phone.ngrok.com");
    wifi.println("Connection: close");
    wifi.println();
  }
}

void getReadings() {
  float temp = sensors.getTempF(insideThermometer);
  moisture = analogRead(MOISTURE_SENSOR);
  dtostrf(temp, 6, 2, tempStr);
  smsMoist = "moisture=" + moisture;
  smsTemp = "temp=" + tempStr;
  smsMsg = smsMoist + smsTemp;
  sendStatus(smsMsg);

}

void monitorPusher() {
  if (client.connected()) {
    client.bind("status", getReadings);
    client.monitor();
  }
  else {
    Serial.println("Client error connecting.");
  }
}