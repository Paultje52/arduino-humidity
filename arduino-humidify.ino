/*
  This file is part of arduino-humidify which is released under MIT License.
  See file "LICENSE" or go to https://github.com/Paultje52/arduino-humidity/blob/main/LICENSE for full license details.

  Github: https://github.com/Paultje52/arduino-humidity
  Written by: Paultje52
*/

// All the libraries
#include <dht.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// Pins
#define DHT_PIN 2
#define dehumidifier 4
#define humidifier 5
#define BACKLIGHT_PIN 8

// Initialize objects
dht DHT;
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  // Serial stuff
  Serial.begin(9600);
  Serial.println("Time\tTemperature\tHumidity");

  // Pins
  pinMode(BACKLIGHT_PIN, OUTPUT);

  // LCD
  lcd.init();
  lcd.backlight();

  // Relays
  pinMode(dehumidifier, INPUT);
  pinMode(humidifier, INPUT);
}

// Timed variables
unsigned long lastSensorRead = millis();
unsigned long updateDisplayTime = millis();
unsigned long updateScreenBlink = millis();
unsigned long postDataTime = millis();
unsigned long lastHumidChange = millis();

// Other variables for in the loop
bool screenBacklightState = false;
bool blinkScreen = false;
int stopScreenBlinkAfterXTimes = -1;

// Statusses
bool humidifying = false;
bool dehumidifying = false;

// Other constant variables
int sensorReadsPerSecond = 10;
int screenUpdatesPerSecond = 2;
int screenBlinksPerSecond = 10*2;
int postDataPerMinute = 4;
int humidChangesPerMinute = 2;
double x = 0;

// The loop, where the magic happens
void loop() {
  // Sensor read
  if (millis()-lastSensorRead > (1000/sensorReadsPerSecond)) {
    DHT.read11(DHT_PIN);
    lastSensorRead = millis();
  }

  // Sensor values
  float temp = getTemp();
  float humid = getHumid();

  // Display updates
  if (millis()-updateDisplayTime > (1000/screenUpdatesPerSecond)) {
    
    // Print temperature
    lcd.setCursor(0, 0);
    lcd.print(String(temp));
    lcd.print((char)223);
    lcd.print("C");

    // Print humidity
    lcd.setCursor(13, 0);
    lcd.print(String((int) humid));
    lcd.print("%");

    // Print status
    if (humidifying) {
      lcd.setCursor(2, 1);
      lcd.print("Humidifying");
      
    } else if (dehumidifying) {
      lcd.setCursor(1, 1);
      lcd.print("Dehumidifying");
      
    } else {
      lcd.setCursor(6, 1);
      lcd.print("Idle");
      
    }
    
    updateDisplayTime = millis();
  }
  
  // Post data updates
  if (millis()-postDataTime > 60/postDataPerMinute * 1000 ) {
    // Serial print the data for the plotter
    x += 0.25;
    Serial.print(x);
    Serial.println("\t"+String(temp)+"\t"+String(humid));
    
    postDataTime = millis();
  }
  
  // Update screen blink
  if (millis()-updateScreenBlink > (1000/screenBlinksPerSecond)) {   
    // Turn the screen on or off
    if (blinkScreen && (stopScreenBlinkAfterXTimes > 0 || stopScreenBlinkAfterXTimes == -1) ) {

      screenBacklightState = !screenBacklightState;
      digitalWrite(BACKLIGHT_PIN, screenBacklightState);

      if (stopScreenBlinkAfterXTimes > 0) stopScreenBlinkAfterXTimes -= 1;

    } else digitalWrite(BACKLIGHT_PIN, true);
    // Reset
    updateScreenBlink = millis();
  }

  // Humid actions
  if (millis()-lastHumidChange > 60/humidChangesPerMinute * 1000 ) {
    
    if (((int)humid) < 40) {
      if (humidifying) return;
      
      // Start humidifier
      pinMode(humidifier, OUTPUT);
      pinMode(dehumidifier, INPUT);
      dehumidifying = false;
      humidifying = true;
      startScreenBlinking();

    } else if (((int)humid) > 55) {
      if (dehumidifying) return;
      
      // Start dehumidifier
      pinMode(humidifier, INPUT);
      pinMode(dehumidifier, OUTPUT);
      humidifying = false;
      dehumidifying = true;
      startScreenBlinking();

    } else {
      if (humidifying || dehumidifying) startScreenBlinking();
      
      pinMode(humidifier, INPUT);
      pinMode(dehumidifier, INPUT);
      humidifying = false;
      dehumidifying = false;
    }

    lastHumidChange = millis();
  }
}

// Function to start screen blinking (And stopping after 5 blinks)
void startScreenBlinking() {
  blinkScreen = true;
  stopScreenBlinkAfterXTimes = 5;
  
  // Clear screen
  lcd.clear();
}

// Function to read the temperature from the sensor and cancel out spikes
float lastTemp = 0;
float getTemp() {
  float temp = DHT.temperature;
  if (temp < -5 || temp > 50) return getTemp();
  
  if (isnan(temp)) return lastTemp;
  lastTemp = (lastTemp*9+temp)/10;

  return lastTemp;
}

// Function to read the humidity from the sensor and cancel out spikes
float lastHumid = 0;
float getHumid() {
  float humid = DHT.humidity;
  if (humid < -5 || humid > 100) return getHumid();
  
  if (isnan(humid)) return lastHumid;
  lastHumid = (lastHumid*9+humid)/10;

  return lastHumid;
}
