#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>

const char* ssid     = "DESKTOP-02G309U 3929";//Specify wifi username
const char* password = "123456789"; //specify wifi password

const char* host = "api.thingspeak.com";  //From ThingSpeak. Remain constant
const char* APIKey = "OZ16MG4XL8UAMRW0";  //From ThingSpeak Channel

//sensor code
#include <Wire.h>
#include <Adafruit_INA219.h>
Adafruit_INA219 ina219;

// Utrasonic code

const int trigPin = 13;//D7
const int echoPin = 14;//D5;
#define Relay_Pin 2 //D4

//define sound velocity in cm/uS
#define SOUND_VELOCITY 000.034
//tank capacity in litres
#define TANK_CAPACITY 2000;


long duration;
float distance_cm;
float Capacity_litres;

//Motion Sensor Code
// Pin configuration
const int pirPin = 12; // PIR sensor OUT pin

// Variables
int count = 0; // Object count
unsigned long lastDetectionTime = 0; // Time of last detection

// Constants
const unsigned long resetTime = 60000; // Reset time in milliseconds (1 minute)

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");

  while (!Serial) {
      // will pause Zero, Leonardo, etc until serial console opens
      delay(1);
  }
 
  uint32_t currentFrequency;
    
  Serial.println("Hello!");
  
  // Initialize the INA219.
  // By default the initialization will use the largest range (32V, 2A).  However
  // you can call a setCalibration function to change this range (see comments).
  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }
  // To use a slightly lower 32V, 1A range (higher precision on amps):
  //ina219.setCalibration_32V_1A();
  // Or to use a lower 16V, 400mA range (higher precision on volts and amps):
  //ina219.setCalibration_16V_400mA();
 
  Serial.println("Measuring voltage and current with INA219 ...");

  //Utrasonic
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  pinMode(Relay_Pin,OUTPUT);

  //Motion sensor pin
 pinMode(pirPin,INPUT);

}

void loop() {
  // put your main code here, to run repeatedly:
   // put your main code here, to run repeatedly:
  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;
  float power_mW = 0;
 
  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW = ina219.getPower_mW();
  loadvoltage = busvoltage + (shuntvoltage / 1000);
  
  Serial.println("Bus Voltage:   "); 
  Serial.println(busvoltage); 
  Serial.println(" V");
  Serial.println("Shunt Voltage: "); 
  Serial.println(shuntvoltage); 
  Serial.println(" mV");
  Serial.println("Load Voltage:  "); 
  Serial.print(loadvoltage); 
  Serial.println(" V");
  Serial.print("Current:       "); 
  Serial.print(current_mA); 
  Serial.println(" mA");
  Serial.print("Power:         "); 
  Serial.print(power_mW); 
  Serial.println(" mW");
  Serial.println("");
 
  //delay(5000);

  //Utrasonic 
    // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  distance_cm = duration * SOUND_VELOCITY/2;

  //Calculate tank capacity
  Capacity_litres=(distance_cm*2000)/1194;

  //Control
  if(distance_cm > 716){
    digitalWrite(Relay_Pin,HIGH);
  }
  else{
    digitalWrite(Relay_Pin,LOW);
  }
 
  // Prints the distance on the Serial Monitor
  Serial.print("Distance (m): ");
  Serial.println(distance_cm);
  Serial.print("Tank Capacity (Litres): ");
  Serial.println(Capacity_litres);
  Serial.println("Done");
  //delay(1000);

  //Motion Loop
  int pirState = digitalRead(pirPin);

  // Check for a change in PIR sensor state
  if (pirState == HIGH) {
    unsigned long currentTime = millis();

    // Check if enough time has passed since the last detection
    if (currentTime - lastDetectionTime >= resetTime) {
      // Reset the count if 1 minute has elapsed
      count = 0;
      Serial.println("Count reset");
    }

    // Increment count and update last detection time
    count++;
    lastDetectionTime = currentTime;

    Serial.print("Count: ");
    Serial.println(count); // Print count to the serial monitor (optional)

    delay(5000); // Delay to avoid multiple counts for the same object
  }

  // Create URL for ThingSpeak update
  String url = "http://";
  url += host;
  url += "/update?api_key=";
  url += APIKey;
  url += "&field1=";
  url += String(Capacity_litres);
  url += "&field2=";
  url += String(loadvoltage);
  url += "&field3=";
  url += String(busvoltage);
  url += "&field4=";
  url += String(current_mA);
  url += "&field5=";
  url += String(power_mW);
  url += "&field6=";
  url += String(count);

   // Send HTTP GET request to ThingSpeak
  WiFiClient client;
  HTTPClient http;

  if (http.begin(client, url)) {
    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
      Serial.println("Sensor data sent to ThingSpeak successfully");
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("Unable to connect to ThingSpeak");
  }

  // Wait for 15 seconds before sending the next update
  delay(15000);
  

}
