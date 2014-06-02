#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <PusherClient.h>
#include <stdlib.h>
PusherClient client;
WiFiClient wifi;

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 3

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;

int led = 13;

// Range for temperature sensors
int MAX_TEMP = 85;
int MIN_TEMP = 40;

// Range for water level sensors
int MIN_WATER = 300;
int MAX_WATER = 700;

String alertMsg, alertBody; // Buffers for alert messages.

char ssid[] = "d2ce78";     //  your network SSID (name) 
char pass[] = "262841693";    // your network password
int status = WL_IDLE_STATUS;     // the Wifi radio's status

char server[] = "jreyes.ngrok.com";

char tempStr[6]; // buffer for temp incl. decimal point & possible minus sign
int moisture; // sensor reading for moisture. 0 - 950
char waterLvl[6];

void setup(void)
{
  // start serial port
  Serial.begin(9600);
  
  // Setup Wifi and Pusher Client
  setupWifiPusher();
  
  //Start temp stuff
  Serial.println("Dallas Temperature IC Control Library Demo");

  // locate devices on the bus
  Serial.print("Locating devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  // Search for address
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 

  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();

  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  sensors.setResolution(insideThermometer, 9);

  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC); 
  Serial.println();
 
}

// Individual alert sensor monitors
void checkTemperature(DeviceAddress deviceAddress)
{
  float temp = sensors.getTempF(deviceAddress);
  Serial.print(" Temp F: ");
  Serial.println(temp); 
  
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
  moisture = analogRead(0);
  Serial.print("Moisture Sensor Value:");
  Serial.println(moisture);
  if (moisture < MIN_WATER) {
    alertBody = "alert=water_low&moisture=";
  }
  alertMsg = alertBody + moisture;
  sendStatus(alertMsg);
}

// General Sensor readings. Initiated by Pusher.
void checkSensors() {
  float temp = sensors.getTempF(deviceAddress);
  moisture = analogRead(0);
  dtostrf(temp, 6, 2, tempStr);
  smsMoist = "moisture=" + moisture;
  smsTemp = "temp=" + tempStr;
  smsMsg = smsMoist + smsTemp;
  sendStatus(smsMsg);

}

void loop(void)
{ 
  monitorPusher();
  delay(2000);
  // Check the temperature and send an alert to server if necessary
  checkTemperature(insideThermometer); 
  checkMoisture();
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void checkStats(String data) {
  Serial.print("checkingStats");
}

void setupWifiPusher() {
  // start serial port
  Serial.begin(9600);

  // attempt to connect using WPA2 encryption:
  Serial.println("Attempting to connect to WPA network...");
  status = WiFi.begin(ssid, pass);

  // if you're not connected, stop here:
  if ( status != WL_CONNECTED) { 
    Serial.println("Couldn't get a wifi connection");
    while(true);
  } 
  // if you are connected, print out info about the connection:
  else {
    Serial.println("Connected to network");
    if(client.connect("8cc556d7a9480d4b882f")) {
      client.subscribe("garduino");
      Serial.print("Looks like we connected");
    }
    else {
      while(1) {
        Serial.print("Did not connect");
      }
    }
  }
  Serial.println("\nStarting connection to server...");
}

void sendStatus(String data) {
  // if you get a connection, report back via serial:
  if (wifi.connect(server, 80)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    wifi.println("GET /arduino-data?"+ data +" HTTP/1.1");
    wifi.println("Host: jreyes.ngrok.com");
    wifi.println("Connection: close");
    wifi.println();
  }
}

void monitorPusher() {
  if (client.connected()) {
    client.bind("status", sendReadings);
    client.monitor();
  }
  else {
    Serial.println("Client error connecting.");
  }
}