#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
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
void handleStopCommand();
void handleStartCommand(DynamicJsonDocument &doc);
void handleSettingsCommand(DynamicJsonDocument &doc);
void handleSimulationCommand(DynamicJsonDocument &doc);
void handleLogLevelCommand(DynamicJsonDocument &doc);
void handleGetSettingsCommand();
void SendSettingsToMobile();
void SendStatusToMobile();
void SendTargetReachedNotification();

// Air Control Functions
void readCurrentPressure();
void OperateAir();
void StopAir();
void InflateAir();
void DeflateAir();
float readPressureFromSensor();
double calculateDelatFactor();

// Settings Functions
bool normalizeSettings();
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
void scheduleRestart(int delayMs);

// Logging levels
#define LOG_LEVEL_NONE    0
#define LOG_LEVEL_ERROR   1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_INFO    3
#define LOG_LEVEL_DEBUG   4
#define LOG_LEVEL_VERBOSE 5

// Default log level
#define DEFAULT_LOG_LEVEL LOG_LEVEL_DEBUG

// Current log level (can be changed at runtime)
extern int currentLogLevel;

// Logging macros
#define LOG_ERROR(tag, message)   if(currentLogLevel >= LOG_LEVEL_ERROR)   { Serial.print("[ERROR]["); Serial.print(tag); Serial.print("] "); Serial.println(message); }
#define LOG_WARNING(tag, message) if(currentLogLevel >= LOG_LEVEL_WARNING) { Serial.print("[WARN]["); Serial.print(tag); Serial.print("] "); Serial.println(message); }
#define LOG_INFO(tag, message)    if(currentLogLevel >= LOG_LEVEL_INFO)    { Serial.print("[INFO]["); Serial.print(tag); Serial.print("] "); Serial.println(message); }
#define LOG_DEBUG(tag, message)   if(currentLogLevel >= LOG_LEVEL_DEBUG)   { Serial.print("[DEBUG]["); Serial.print(tag); Serial.print("] "); Serial.println(message); }
#define LOG_VERBOSE(tag, message) if(currentLogLevel >= LOG_LEVEL_VERBOSE) { Serial.print("[VERB]["); Serial.print(tag); Serial.print("] "); Serial.println(message); }

// Function to set log level
void setLogLevel(int level);

#endif // MAIN_H