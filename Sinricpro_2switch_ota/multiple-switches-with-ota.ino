/*
 * Example for how to use SinricPro with multiple switches and OTA updates:
 * - setup 2 switch devices
 * - handle OTA updates
 * 
 * If you encounter any issues:
 * - check the readme.md at https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md
 * - ensure all dependent libraries are installed
 *   - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#arduinoide
 *   - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#dependencies
 * - open serial monitor and check whats happening
 * - check full user documentation at https://sinricpro.github.io/esp8266-esp32-sdk
 * - visit https://github.com/sinricpro/esp8266-esp32-sdk/issues and check for existing issues or open a new one
 */

// Uncomment the following line to enable serial debug output
// #define SINRICPRO_NOSSL // Uncomment if you have memory limitation issues.
// #define ENABLE_DEBUG

// Your firmware version. Must be above SinricPro.h. Do not rename this.
#define FIRMWARE_VERSION "1.0.2"  

#ifdef ENABLE_DEBUG
   #define DEBUG_ESP_PORT Serial
   #define NODEBUG_WEBSOCKETS
   #define NDEBUG
#endif 

#include <Arduino.h>
#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include "ESP8266OTAHelper.h"
#elif defined(ESP32) 
  #include <WiFi.h>
  #include "ESP32OTAHelper.h"
#elif defined(ARDUINO_ARCH_RP2040) 
  #include <WiFi.h>
  #include "ESP8266OTAHelper.h"
#endif

#include "SemVer.h"
#include "SinricPro.h"
#include "SinricProSwitch.h"

#define WIFI_SSID         "YOUR_WIFI_SSID"
#define WIFI_PASS         "YOUR_WIFI_PASSWORD"
#define APP_KEY           "YOUR_APP_KEY"
#define APP_SECRET        "YOUR_APP_SECRET"

#define SWITCH_ID_1       "YOUR_SWITCH_ID_1"
#define RELAYPIN_1        2

#define SWITCH_ID_2       "YOUR_SWITCH_ID_2"
#define RELAYPIN_2        1

#define BAUD_RATE         115200                // Change baudrate to your need

bool onPowerState1(const String &deviceId, bool &state) {
 Serial.printf("Device 1 turned %s\r\n", state?"on":"off");
 digitalWrite(RELAYPIN_1, state ? LOW:HIGH);
 return true; // request handled properly
}

bool onPowerState2(const String &deviceId, bool &state) {
 Serial.printf("Device 2 turned %s\r\n", state?"on":"off");
 digitalWrite(RELAYPIN_2, state ? LOW:HIGH);
 return true; // request handled properly
}

bool handleOTAUpdate(const String& url, int major, int minor, int patch, bool forceUpdate) {
  Version currentVersion  = Version(FIRMWARE_VERSION);
  Version newVersion      = Version(String(major) + "." + String(minor) + "." + String(patch));
  bool updateAvailable    = newVersion > currentVersion;

  Serial.print("URL: ");
  Serial.println(url.c_str());
  Serial.print("Current version: ");
  Serial.println(currentVersion.toString());
  Serial.print("New version: ");
  Serial.println(newVersion.toString());
  if (forceUpdate) Serial.println("Enforcing OTA update!");

  // Handle OTA update based on forceUpdate flag and update availability
  if (forceUpdate || updateAvailable) {
    if (updateAvailable) {
      Serial.println("Update available!");
    }

    String result = startOtaUpdate(url);
    if (!result.isEmpty()) {
      SinricPro.setResponseMessage(std::move(result));
      return false;
    } 
    return true;
  } else {
    String result = "Current version is up to date.";
    SinricPro.setResponseMessage(std::move(result));
    Serial.println(result);
    return false;
  }
}

// setup function for WiFi connection
void setupWiFi() {
  Serial.printf("\r\n[Wifi]: Connecting");

  #if defined(ESP8266)
    WiFi.setSleepMode(WIFI_NONE_SLEEP); 
    WiFi.setAutoReconnect(true);
  #elif defined(ESP32)
    WiFi.setSleep(false); 
    WiFi.setAutoReconnect(true);
  #endif

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }

  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
}

// setup function for SinricPro
void setupSinricPro() {
  // add devices and callbacks to SinricPro
  pinMode(RELAYPIN_1, OUTPUT);
  pinMode(RELAYPIN_2, OUTPUT);
    
  SinricProSwitch& mySwitch1 = SinricPro[SWITCH_ID_1];
  mySwitch1.onPowerState(onPowerState1);
  
  SinricProSwitch& mySwitch2 = SinricPro[SWITCH_ID_2];
  mySwitch2.onPowerState(onPowerState2);
  
  // setup SinricPro
  SinricPro.onConnected([](){ 
    Serial.printf("Connected to SinricPro\r\n"); 
    Serial.printf("Current firmware version: %s\r\n", FIRMWARE_VERSION);
  }); 
  SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
  SinricPro.restoreDeviceStates(true); // Restore the last known state from the server
  SinricPro.onOTAUpdate(handleOTAUpdate); // Register OTA update handler
  
  SinricPro.begin(APP_KEY, APP_SECRET);
}

// main setup function
void setup() {
  Serial.begin(BAUD_RATE); 
  Serial.printf("\r\n\r\n");
  Serial.printf("Starting MultiSwitch with OTA Updates (Version: %s)\r\n", FIRMWARE_VERSION);
  setupWiFi();
  setupSinricPro();
}

void loop() {
  SinricPro.handle();
}
