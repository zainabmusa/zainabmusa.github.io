/* ARDUINO SENSOR CONNECTION TO NODEMCU
 *  NODEMCU ESP8266 CODE BY NUR ZAINAB MOHAMED MUSA
 */

/*  HTTPS on ESP8266 with follow redirects, chunked encoding support
 *  Version 3.0
 *  Author: Sujay Phadke
 *  Github: @electronicsguy
 *  Copyright (C) 2018 Sujay Phadke <electronicsguy123@gmail.com>
 *  All rights reserved.
 *
 *  Example Arduino program
 */
#include <DHTesp.h>
#include <ESP8266WiFi.h>
#include "HTTPSRedirect.h"
#include "DebugMacros.h"


int sensor_pin = A0;
int val;
int dhtpin = 4 ; // Pin number for DHT22 data pin
signed int moisture_value;
float volts = 0.0;
float temp_init = 0.0;
float temp = 0.0;

DHTesp dht; // Initialize the DHT sensor

// SHINYEI ADDED HERE
//Set variables for PM10 and PM2,5 readings
 unsigned long starttime;
 unsigned long sampletime_ms = 60000; // TIME BETWEEN MEASURES AND UPDATES

unsigned long triggerOnP2;
 unsigned long triggerOffP2;
 unsigned long pulseLengthP2;
 unsigned long durationP2;
 boolean valP2 = HIGH;
 boolean triggerP2 = false;
 float ratioP2 = 0 ;
 float countP2;
 float concLarge;

unsigned long triggerOnP1;
 unsigned long triggerOffP1;
 unsigned long pulseLengthP1;
 unsigned long durationP1;
 boolean valP1 = HIGH;
 boolean triggerP1 = false;
 float ratioP1 = 0;
 float countP1;
 float concSmall;


// Fill ssid and password with your network credentials
const char* ssid ="Arduino Sensor";
const char* password ="Arduino Sensor";
const char* host = "script.google.com";

// Replace with your own script id to make server side changes
const char *GScriptId = "AKfycbym7fBBOhcTq2KXWlM8-9h1f6n5mviIpQbCiEiME03gqVlSnLc";
const int httpsPort = 443;

// echo | openssl s_client -connect script.google.com:443 |& openssl x509 -fingerprint -noout
const char* fingerprint = "6B F3 14 BF 1D 8D BF E0 EA D0 17 F1 A3 06 C6 48 B7 17 DB 0F";
//const uint8_t fingerprint[20] = {};

// Write to Google Spreadsheet
String url = String("/macros/s/") + GScriptId + "/exec?value=Hello";
// Fetch Google Calendar events for 1 week ahead
String url2 = String("/macros/s/") + GScriptId + "/exec?cal";
// Read from Google Spreadsheet
String url3 = String("/macros/s/") + GScriptId + "/exec?read";

String payload_base =  "{\"command\": \"appendRow\", \
                    \"sheet_name\": \"Sheet1\", \
                    \"values\": ";
String payload = "";


HTTPSRedirect* client = nullptr;

////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
digitalWrite (0,LOW); // Sets output to gnd
pinMode (0, OUTPUT); // switches power to DHT on
delay (1000); // delay necessary after power up for DHT to stabilize
//dht.begin(); // initializes DHT
dht.setup(14, DHTesp::DHT22);

//SHINYEI ADDED HERE
 pinMode(7, INPUT); //PM2,5
 pinMode(8, INPUT);// PM10
 Serial.begin(115200);
 delay(6000);

  Serial.flush();
  Serial.println();
  Serial.print("Connecting to wifi: ");
  Serial.println(ssid);
  // flush() is needed to print the above (connecting...) message reliably, 
  // in case the wireless connection doesn't go through
  Serial.flush();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");
  
  Serial.print("Connecting to ");
  Serial.println(host);

  // Try to connect for a maximum of 5 times
  bool flag = false;
  for (int i=0; i<5; i++){
    int retval = client->connect(host, httpsPort);
    if (retval == 1) {
       flag = true;
       break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  if (!flag){
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    return;
  }


  // delete HTTPSRedirect object
  delete client;
  client = nullptr;
}

//SHINYEI ADDED HERE//
void measure(){
 valP1 = digitalRead(13); // Small ( pm2.5)
 valP2 = digitalRead(15); // Large ( pm10 )

//--------PM2,5-------------

if(valP1 == LOW && triggerP1 == false){
 triggerP1 = true;
 triggerOnP1 = micros();
 }

if (valP1 == HIGH && triggerP1 == true){
 triggerOffP1 = micros();
 pulseLengthP1 = triggerOffP1 - triggerOnP1;
 durationP1 = durationP1 + pulseLengthP1;
 triggerP1 = false;
 }

//-----------PM10------------

if(valP2 == LOW && triggerP2 == false){
 triggerP2 = true;
 triggerOnP2 = micros();
 }

if (valP2 == HIGH && triggerP2 == true){
 triggerOffP2 = micros();
 pulseLengthP2 = triggerOffP2 - triggerOnP2;
 durationP2 = durationP2 + pulseLengthP2;
 triggerP2 = false;
 }

//----------Calcolo-----------

if ((millis() - starttime) > sampletime_ms) {

// Integer percentage 0=>100
 ratioP1 = durationP1/(sampletime_ms*10.0); // Integer percentage 0=>100
 ratioP2 = durationP2/(sampletime_ms*10.0);
 countP1 = 1.1*pow(ratioP1,3)-3.8*pow(ratioP1,2)+520*ratioP1+0.62;
 countP2 = 1.1*pow(ratioP2,3)-3.8*pow(ratioP2,2)+520*ratioP2+0.62;
 float PM10count = countP2;
 float PM25count = countP1 - countP2;

//PM10 count to mass concentration conversion
 double r10 = 2.6*pow(10,-6);
 double pi = 3.14159;
 double vol10 = (4/3)*pi*pow(r10,3);
 double density = 1.65*pow(10,12);
 double mass10 = density*vol10;
 double K = 3531.5;
 concLarge = (PM10count)*K*mass10;

//PM2.5 count to mass concentration conversion
 double r25 = 0.44*pow(10,-6);
 double vol25 = (4/3)*pi*pow(r25,3);
 double mass25 = density*vol25;
 concSmall = (PM25count)*K*mass25;


//----Debug of Values on Serial Port----
 Serial.print("PM10Conc: ");
 Serial.print(concLarge);
 Serial.print("ug/m3 , PM2.5Conc: ");
 Serial.print(concSmall);
 Serial.println(" ug/m3");

//Reset Values
 durationP1 = 0;
 durationP2 = 0;
 starttime = millis();

}
}

void loop() {

//storing temperature and humidity values from DHT22 into a variable using DHTesp library

{
  TempAndHumidity measurement = dht.getTempAndHumidity ();
}

Serial.println ("THIS IS THE CURRENT TEMPERATURE");
Serial.println(temp);

  static int error_count = 0;
  static int connect_count = 0;
  const unsigned int MAX_CONNECT = 20;
  static bool flag = false;

 

  if (!flag){
   
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }

  if (client != nullptr){
    if (!client->connected()){
      client->connect(host, httpsPort);
      payload = payload_base + "\"" + "0" + "," + "0" + "," + "0" + "\"}";
      client->POST(url2, host, payload, false);
    }
  }
  else{
    DPRINTLN("Error creating client object!");
    error_count = 5;
  }
  
  if (connect_count > MAX_CONNECT){
    //error_count = 5;
    connect_count = 0;
    flag = false;
    delete client;
    return;
  }

  

  Serial.println("POST append memory data to spreadsheet:");
// POST "temperature" in col2 and humidity in col3
  payload = payload_base + "\"" + concLarge + "," +  concSmall + "\" }";
   
if (client->POST(url, host, payload)){
;
  }
  else{
    ++error_count;
    DPRINT("Error-count while connecting: ");
    DPRINTLN(error_count);
  }

  
  
  if (error_count > 10){
    Serial.println("Halting processor..."); 
    delete client;
    client = nullptr;
    Serial.flush();
    ESP.deepSleep(0);
  }
  
  // In my testing on a ESP-01, a delay of less than 1500 resulted 
  // in a crash and reboot after about 50 loop runs.
  delay(5000);


                          

}
