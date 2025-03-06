#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include "settings.h"

// Function Declarations

// Setup Functions
void setup();
void setupWiFi();
void setupWebServer();
void setupBlueTooth();

// BLE Functions
void BLESync();
void handleBLEMessage(String Message);
void SendSettingsToMobile();
void SendStatusToMobile();

// Air Control Functions
void readCurrentPressure();
void OperateAir();
void StopAir();
void InflateAir();
void DeflateAir();
float readPressureFromSensor();
double calculateDelatFactor();

// Settings Functions
void loadSettingsfile();
void saveSettingFile();
void printSettingsFile();

// Web Server Functions
void notFound(AsyncWebServerRequest *request);
void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);
void handleDoUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void printProgress(size_t prg, size_t sz);
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void deleteFile(fs::FS &fs, const String &path);
void saveSettings(AsyncWebServerRequest *request);

// JSON/Data Functions
String getSettingsParams(const String &var);
String getSettingsXML();
String getPressureJSON();
String getSettingsJSON();
String processor_update(const String &var);

// System Functions
void restart();
void loop();

#endif // MAIN_H