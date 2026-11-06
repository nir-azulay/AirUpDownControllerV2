#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <WiFi.h>
#include <ArduinoJson.h>
#include <Update.h>
#include "main.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include "settings.h"
#include <version.h>

int backupProgress = 0;
bool wifiInitialized = false;
int tikingCounter = 0;
volatile bool upgradeInProgress = false;
volatile unsigned long lastUpdateChunkMs = 0;
volatile bool pendingSaveSettings = false;
volatile bool pendingRestart = false;
volatile unsigned long pendingRestartAtMs = 0;
volatile bool pendingBleWifiScan = false;
volatile bool pendingStartWifiScan = false;
volatile unsigned long pendingStartWifiScanAtMs = 0;
String currentIP = "";


#pragma region webServer declaration

#define U_PART U_SPIFFS
unsigned long timer1;
unsigned long timer2;
String filelist = "";
const char *PARAM = "file";

AsyncWebServer server(80);
size_t content_len;
File file;
bool opened = false;
#pragma endregion webServer declaration

// Global variables
float currentPressure = 0;
double targetPressure = 0;
OperationMode operationMode = Stop;
bool isSimulationMode = false;
bool pressureValid = true;
bool valveSessionActive = false;
unsigned long valveSessionStartMs = 0;
unsigned long sessionProgressMs = 0;
float sessionBaselinePressure = -1;
volatile unsigned long valveCommandEpoch = 0;
volatile bool measureNow = false;
bool sessionHasReading = false;
const float kStablePsi = 3.0f;
const unsigned long kNoProgressMs = 60000;
const float kMinProgressPsi = 0.5f;
bool debugMode = false; // Debug mode flag, default off
int lastRawADC = 0; // Store last filtered ADC value
float lastVoltage = 0.0f; // Store last calculated voltage

// Logging
int currentLogLevel = DEFAULT_LOG_LEVEL;

void setLogLevel(int level) {
    if (level >= LOG_LEVEL_NONE && level <= LOG_LEVEL_VERBOSE) {
        currentLogLevel = level;
        Serial.print("Log level set to: ");
        switch (level) {
            case LOG_LEVEL_NONE:    Serial.println("NONE"); break;
            case LOG_LEVEL_ERROR:   Serial.println("ERROR"); break;
            case LOG_LEVEL_WARNING: Serial.println("WARNING"); break;
            case LOG_LEVEL_INFO:    Serial.println("INFO"); break;
            case LOG_LEVEL_DEBUG:   Serial.println("DEBUG"); break;
            case LOG_LEVEL_VERBOSE: Serial.println("VERBOSE"); break;
        }
    } else {
        Serial.println("Invalid log level. Use values between 0 (NONE) and 5 (VERBOSE)");
    }
}

// settings is now defined in settings.cpp

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* pCharacteristic_2 = NULL;
BLEDescriptor *pDescr;
BLE2902 *pBLE2902;

bool deviceConnected = false;
bool oldDeviceConnected = false;
bool firstTimeConnceted = true;
bool bleAuthorized = true;
String BLEBufferValue = "";
String incompleteBLEMessage = "";


// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR1_UUID          "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR2_UUID          "e3223119-9445-4e96-a4a1-85358c4046a2"


class MyBLEServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      firstTimeConnceted = true;
      bleAuthorized = settings.AccessCode.length() == 0;
      LOG_INFO("BLE", "Client connected");
      // Enable multiple connections
      BLEDevice::startAdvertising();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      bleAuthorized = false;
      LOG_INFO("BLE", "Client disconnected");
      
      // Clear incomplete message on disconnect
      incompleteBLEMessage = "";
      
      // Restart advertising
      BLEDevice::startAdvertising();
    }
};

class CharacteristicCallBack: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pChar) override { 
    LOG_DEBUG("BLE", "CharacteristicCallBack::onWrite - Starting to process received value");
    
    std::string rxValue = pChar->getValue();
    if (rxValue.length() > 0) {
      String pChar2_value_string = String(rxValue.c_str());
      
      LOG_DEBUG("BLE", "Received value length: " + String(pChar2_value_string.length()));
      LOG_VERBOSE("BLE", "Received value: " + pChar2_value_string);

      incompleteBLEMessage += pChar2_value_string;

      // Process all complete JSON messages (from '{' to '}') in buffer
      int closeIdx = incompleteBLEMessage.indexOf('}');
      while (closeIdx >= 0) {
        int openIdx = incompleteBLEMessage.indexOf('{');
        if (openIdx >= 0 && openIdx < closeIdx) {
          String completeMsg = incompleteBLEMessage.substring(openIdx, closeIdx + 1);
          LOG_INFO("BLE", "Complete message received: " + completeMsg);
          handleBLEMessage(completeMsg);
        }
        incompleteBLEMessage = incompleteBLEMessage.substring(closeIdx + 1);
        closeIdx = incompleteBLEMessage.indexOf('}');
      }
    } else {
      LOG_WARNING("BLE", "Received empty value");
    }
    
    LOG_DEBUG("BLE", "CharacteristicCallBack::onWrite - Finished processing value");
  }
};

#pragma region WebServer
//======================

String processor_update(const String &var)
{
    Serial.println(var);
    if (var == "list")
    {
        return filelist;
    }
    return String();
}

void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
    lastUpdateChunkMs = millis();
    upgradeInProgress = true;

    if (!index)
    {
        if (request && request->client()) {
            request->client()->setRxTimeout(60);
            request->client()->setAckTimeout(60000);
        }
        // NOTE: WiFi modem sleep must stay enabled while Bluetooth is on —
        // ESP-IDF aborts ("Should enable WiFi modem sleep when both WiFi and
        // Bluetooth are enabled") if WIFI_PS_NONE is set with BT running.
        if (Update.isRunning()) {
            Update.abort();
        }
        content_len = request->contentLength();
        // if filename includes spiffs, update the spiffs partition
        int cmd = (filename.indexOf("spiffs") > -1) ? U_PART : U_FLASH;
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd))
        {
            Update.printError(Serial);
        }
    }
    if (len > 0)
    {
        // NOTE: each 64 KB written triggers a flash block erase in here
        // (~300 ms) during which this AsyncTCP handler cannot ACK; the apps
        // pace the upload (3 ms per 1436-byte chunk, 25 ms per 64 KB).
        size_t written = Update.write(data, len);
        if (written != len)
        {
            Update.printError(Serial);
            size_t totalSz = Update.size() > 0 ? Update.size() : (content_len > 0 ? content_len : 1);
            Serial.printf("Progress: %u%%\n", (unsigned int)((Update.progress() * 100) / totalSz));
        }
    }

    if (final)
    {
        if (!Update.end(true))
        {
            Update.printError(Serial);
            upgradeInProgress = false;
            AsyncResponseStream *response = request->beginResponseStream("text/plain");
            response->setCode(500);
            response->addHeader("Access-Control-Allow-Origin", "*");
            response->print("Update failed");
            request->send(response);
        }
        else
        {
            AsyncResponseStream *response = request->beginResponseStream("text/plain");
            response->addHeader("Access-Control-Allow-Origin", "*");
            response->print("Ok");
            request->send(response);
            Serial.println("Update complete");
            Serial.flush();
            scheduleRestart(1000);
        }
    }
}

void handleDoUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    static FILE *uploadFp = nullptr;
    String cleanName = filename;
    if (cleanName.startsWith("/")) {
        cleanName = cleanName.substring(1);
    }
    String fullPath = String("/spiffs/") + cleanName;

    if (!index)
    {
        if (request && request->client()) {
            request->client()->setRxTimeout(60);
            request->client()->setAckTimeout(60000);
        }
        content_len = request->contentLength();
        Serial.printf("UploadStart: %s (%u bytes)\n", cleanName.c_str(), (unsigned int)content_len);
        if (uploadFp) {
            fclose(uploadFp);
            uploadFp = nullptr;
        }
        unlink(fullPath.c_str());
        uploadFp = fopen(fullPath.c_str(), "w");
        if (!uploadFp) {
            Serial.println("- failed to open file for writing");
            return;
        }
        setvbuf(uploadFp, NULL, _IONBF, 0);
    }

    if (uploadFp && len > 0)
    {
        if (fwrite(data, 1, len, uploadFp) != len) {
            Serial.println("- failed to write");
        }
    }

    if (final)
    {
        if (uploadFp) {
            fflush(uploadFp);
            fclose(uploadFp);
            uploadFp = nullptr;
        }
        opened = false;
        Serial.println("---------------");
        Serial.println("Upload complete");
        listDir(SPIFFS, "/", 0);
        AsyncResponseStream *response = request->beginResponseStream("text/plain");
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->print("Ok");
        request->send(response);
    }
}

void printProgress(size_t prg, size_t sz)
{
    size_t denom = (content_len > 0) ? content_len : (sz > 0 ? sz : 1);
    int progress = ((prg * 100) / denom);

    if (progress % 10 == 0 && progress != backupProgress)
    {
        backupProgress = progress;
        Serial.printf("Progress: %d%%\n", progress);
    }
}

void notFound(AsyncWebServerRequest *request)
{
    if (request->method() == HTTP_OPTIONS)
    {
        AsyncWebServerResponse *response = request->beginResponse(200);
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        response->addHeader("Access-Control-Allow-Headers", "*");
        request->send(response);
        return;
    }
    if (request->url().startsWith("/"))
    {
        bool isJsonOrText = request->url().endsWith(".json") || request->url().endsWith(".txt");
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, request->url(), String(), !isJsonOrText);
        if (response) {
            response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate");
            response->addHeader("Access-Control-Allow-Origin", "*");
            request->send(response);
        } else {
            request->send(404, "text/plain", "Not found");
        }
    }
    else
    {
        request->send(404);
    }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    int i = 0;
    String partlist;
    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        root.close();
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            if (levels)
            {
                listDir(fs, file.name(), levels - 1);
            }
        }
        else
        {
            i++;
            String fullPath = String(file.name());
            if (!fullPath.startsWith("/")) {
                fullPath = "/" + fullPath;
            }
            String st_after_symb = fullPath.substring(fullPath.indexOf("/") + 1);
            size_t fsize = file.size();
            String sizeStr = (fsize < 1024) ? (String(fsize) + " B") : (String(fsize / 1024.0f, 1) + " KB");

            partlist += String("<tr><td>") + String(i) + String("</td><td><a href='") + fullPath + String("' target='_blank' style='color:#007c91;font-weight:600;text-decoration:none;'>") + st_after_symb + String("</a></td><td>") + sizeStr + String("</td><td><button type='button' class='btndel' onclick=\"deletef('") + fullPath + String("')\">Delete</button></td></tr>");
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
    filelist = String("<table><tbody><tr><th>#</th><th>File Name</th><th>Size</th><th>Action</th></tr>") + partlist + String("</tbody></table>");
}

void deleteFile(fs::FS &fs, const String &path)
{
    Serial.printf("Deleting file: %s\r\n", path);
    if (fs.remove(path))
    {
        Serial.println("- file deleted");
    }
    else
    {
        Serial.println("- delete failed");
    }
    listDir(SPIFFS, "/", 0);
}

// WebServer server(80);
String getSettingsParams(const String &var)
{
    Serial.println(var);
    String status = settings.DeviceName + ";" + settings.wifissid + ";" + settings.wifipassword + ";" + String(settings.RelayCompressorPin) + ";" + String(settings.RelayFreePin) + ";" + String(settings.SensorPin) + ";" + String(settings.SimulationMode) + ";" + String(settings.MaxSecForStatus) + ";" + String(settings.SensorValueAt0PSI, 3) + ";" + String(settings.SensorValueAt50PSI, 3) + ";" + settings.AccessCode + ";" + String(settings.MessureWait) + ";" + String(settings.Precision) + ";" + String(settings.PressureCalcMethod) + ";0;" + VERSION_SHORT + ";" + String(settings.MaxPressurePSI, 1) + ";" + String(settings.MaxValveOpenSec) + ";" + String(settings.SensorFullScalePSI, 1) + ";" + settings.AccessPointPassword;
    Serial.println("getsetSetting: " + status);
    return status;
}

String getSettingsXML()
{

    String XML = "<settings>";
    XML += "<DeviceName>" + settings.DeviceName + "</DeviceName>";
    XML += "<wifissid>" + settings.wifissid + "</wifissid>";
    XML += "<wifipassword>" + settings.wifipassword + "</wifipassword>";
    XML += "<RelayCompressorPin>" + String(settings.RelayCompressorPin) + "</RelayCompressorPin>";
    XML += "<RelayFreePin>" + String(settings.RelayFreePin) + "</RelayFreePin>";
    XML += "<SensorPin>" + String(settings.SensorPin) + "</SensorPin>";
    XML += "<MaxSecForStatus>" + String(settings.MaxSecForStatus) + "</MaxSecForStatus>";
    XML += "<SensorValueAt0PSI>" + String(settings.SensorValueAt0PSI, 3) + "</SensorValueAt0PSI>";
    XML += "<SensorValueAt50PSI>" + String(settings.SensorValueAt50PSI, 3) + "</SensorValueAt50PSI>";
    XML += "<MessureWait>" + String(settings.MessureWait) + "</MessureWait>";
    XML += "<Precision>" + String(settings.Precision) + "</Precision>";    
    XML += "<CurrentPressure>" + String(currentPressure) + "</CurrentPressure>";
    XML += "<Software_VER>" + String(VERSION_SHORT) + "</Software_VER>";
    XML += "</settings>";

    Serial.println("getSettingsXML: " + XML);
    return XML;
}

String getPressureJSON()
{
    String JSON = "{";
    JSON += "Pressure: '" + String(currentPressure,1) + "',";
    JSON += "operationMode: '" + String(operationMode) + "'";
    if (debugMode) {
        JSON += ", rawADC: '" + String(lastRawADC) + "'";
        JSON += ", voltage: '" + String(lastVoltage, 3) + "'";
    }
    JSON += "}";

    LOG_DEBUG("PRESSURE", "getPressureJSON: " + JSON);
    return JSON;
}

String getSettingsJSON()
{
    String JSON = "{";
    JSON += "operationMode: 4 ," ;    
    JSON += "'DeviceName': '" + settings.DeviceName + "',";
    JSON += "'wifissid': '" + settings.wifissid + "',";
    JSON += "'wifipassword': '" + settings.wifipassword + "',";
    JSON += "'RelayCompressorPin': '" + String(settings.RelayCompressorPin) + "',";
    JSON += "'RelayFreePin': '" + String(settings.RelayFreePin) + "',";
    JSON += "'SensorPin': '" + String(settings.SensorPin) + "',";
    JSON += "'MaxSecForStatus': '" + String(settings.MaxSecForStatus) + "',";
    JSON += "'SensorValueAt0PSI': '" + String(settings.SensorValueAt0PSI, 3) + "',";
    JSON += "'SensorValueAt50PSI': '" + String(settings.SensorValueAt50PSI, 3) + "',";
    JSON += "'MessureWait': '" + String(settings.MessureWait) + "',";
    JSON += "'Precision': '" + String(settings.Precision) + "',";    
    JSON += "'CurrentPressure': '" + String(currentPressure) + "',";
    JSON += "'Software_VER': '" + String(VERSION_SHORT) + "',";
    JSON += "'IP': '" + currentIP + "',";    
    JSON += "'SimulationMode': '" + String(isSimulationMode) + "',";
    JSON += "'PressureCalcMethod': '" + String(settings.PressureCalcMethod) + "',";
    JSON += "'SensorFullScalePSI': '" + String(settings.SensorFullScalePSI, 1) + "',";
    JSON += "'MaxPressurePSI': '" + String(settings.MaxPressurePSI, 1) + "',";
    JSON += "'MaxValveOpenSec': '" + String(settings.MaxValveOpenSec) + "',";
    JSON += "'AccessPointPassword': '" + settings.AccessPointPassword + "',";
    JSON += "'HasAccessCode': '" + String(settings.AccessCode.length() > 0 ? 1 : 0) + "'";
    JSON += "}";

    return JSON;
}

void scheduleRestart(int delayMs) {
    static bool restartScheduled = false;
    if (!restartScheduled) {
        restartScheduled = true;
        LOG_INFO("SYSTEM", "Scheduling restart in " + String(delayMs) + " ms");
        
        // הקצאת זיכרון עבור הפרמטר שיועבר למשימה
        int* pDelay = new int(delayMs);
        
        // יצירת טיימר שיאתחל את המערכת לאחר ההשהיה שהוגדרה
        xTaskCreate(
            [](void* parameter) {
                int delay = *((int*)parameter);
                vTaskDelay(delay / portTICK_PERIOD_MS);
                // שחרור הזיכרון שהוקצה
                delete (int*)parameter;
                ESP.restart();
                vTaskDelete(NULL);
            },
            "restart_task",
            2048,
            pDelay,  // העברת המצביע לזיכרון שהוקצה
            1,
            NULL
        );
    }
}

void saveSettings(AsyncWebServerRequest *request)
{
    Serial.println("saveSettings");

    int paramsNr = request->params();
    Serial.println(paramsNr);

    bool clearAccessCode = false;
    for (int i = 0; i < paramsNr; i++)
    {
        AsyncWebParameter *p = request->getParam(i);
        String paramName = p->name();
        String val = p->value();
        val.trim();
        if (paramName == "DeviceName")
            settings.DeviceName = val;
        else if (paramName == "wifissid")
            settings.wifissid = val;
        else if (paramName == "wifipassword")
            settings.wifipassword = val;
        else if (paramName == "RelayCompressorPin")
            settings.RelayCompressorPin = val.toInt();
        else if (paramName == "RelayFreePin")
            settings.RelayFreePin = val.toInt();
        else if (paramName == "SensorPin")
            settings.SensorPin = val.toInt();
        else if (paramName == "MaxSecForStatus")
            settings.MaxSecForStatus = val.toInt();
        else if (paramName == "SensorValueAt0PSI")
            settings.SensorValueAt0PSI = val.toFloat();
        else if (paramName == "SensorValueAt50PSI")
            settings.SensorValueAt50PSI = val.toFloat();
        else if (paramName == "AccessCode")
            settings.AccessCode = val;
        else if (paramName == "ClearAccessCode")
        {
            if (val == "true" || val == "1")
                clearAccessCode = true;
        }
        else if (paramName == "MessureWait")
            settings.MessureWait = val.toDouble();
        else if (paramName == "Precision")
            settings.Precision = val.toDouble();
        else if (paramName == "SimulationMode")
        {
            settings.SimulationMode = (val.equalsIgnoreCase("true") || val == "1");
            isSimulationMode = settings.SimulationMode;
            LOG_INFO("WEB", "Simulation mode: " + String(isSimulationMode ? "enabled" : "disabled"));
            if (isSimulationMode) {
                currentPressure = 0;
            }
        }
        else if (paramName == "PressureCalcMethod")
        {
            settings.PressureCalcMethod = val.toInt();
            LOG_INFO("WEB", "Pressure calculation method set to: " + String(settings.PressureCalcMethod));
        }
        else if (paramName == "MaxPressurePSI")
            settings.MaxPressurePSI = val.toFloat();
        else if (paramName == "MaxValveOpenSec")
            settings.MaxValveOpenSec = val.toInt();
        else if (paramName == "SensorFullScalePSI")
            settings.SensorFullScalePSI = val.toFloat();
        else if (paramName == "AccessPointPassword")
            settings.AccessPointPassword = val;
    }
    if (clearAccessCode) {
        settings.AccessCode = "";
    }


    normalizeSettings();
    pendingSaveSettings = true;
    pendingRestart = true;
    pendingRestartAtMs = millis() + 800;

    AsyncResponseStream *response = request->beginResponseStream("text/plain");
    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->print("Ok");
    request->send(response);

    Serial.println("Settings queued for save and restart");
}

void setupWebServer()
{
    Serial.println("---- setupWebServer ----");

    timer1 = millis();
    timer2 = millis();
    listDir(SPIFFS, "/", 0);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/index.html", String(), false); });

    server.on("/index.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/index.css", "text/css"); });

    server.on("/index.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/index.js", "text/javascript"); });

    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/update.html", String(), false); });

    server.on("/update.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/update.css", "text/css"); });

    server.on("/update.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/update.js", "text/javascript"); });

    server.on("/filesystem", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/fs.html", "text/html"); });

    server.on("/fs.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/fs.css", "text/css"); });

    server.on("/fs.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/fs.js", "text/javascript"); });

    server.on("/filelist", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  AsyncResponseStream *response = request->beginResponseStream("text/html");
                  response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate");
                  response->print(filelist);
                  request->send(response);
              });

    server.on("/configurations", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/configurations.html", "text/html"); });

    server.on("/settingsValues", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  AsyncResponseStream *response = request->beginResponseStream("text/plain");
                  response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate");
                  response->addHeader("Access-Control-Allow-Origin", "*");
                  response->print(getSettingsParams("STATE"));
                  request->send(response);
              });

    server.on("/scanWifi", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  bool forceRescan = request->hasParam("rescan");
                  int n = WiFi.scanComplete();
                  if (forceRescan && n != WIFI_SCAN_RUNNING) {
                      pendingStartWifiScan = true;
                      pendingStartWifiScanAtMs = millis() + 120;
                      n = WIFI_SCAN_RUNNING;
                  } else if (n == WIFI_SCAN_FAILED) {
                      pendingStartWifiScan = true;
                      pendingStartWifiScanAtMs = millis() + 120;
                      n = WIFI_SCAN_RUNNING;
                  }

                  AsyncResponseStream *response = request->beginResponseStream("application/json");
                  response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate");
                  if (n == WIFI_SCAN_RUNNING || n < 0) {
                      response->print("{\"scanning\":true,\"networks\":[]}");
                      request->send(response);
                      return;
                  }

                  response->print("{\"scanning\":false,\"networks\":[");
                  bool first = true;
                  for (int i = 0; i < n; i++) {
                      String ssid = WiFi.SSID(i);
                      ssid.trim();
                      if (ssid.length() == 0) continue;
                      bool duplicate = false;
                      for (int j = 0; j < i; j++) {
                          String prev = WiFi.SSID(j);
                          prev.trim();
                          if (prev == ssid) {
                              duplicate = true;
                              break;
                          }
                      }
                      if (duplicate) continue;

                      ssid.replace("\\", "\\\\");
                      ssid.replace("\"", "\\\"");
                      if (!first) response->print(",");
                      first = false;
                      response->printf("{\"ssid\":\"%s\",\"rssi\":%d}", ssid.c_str(), (int)WiFi.RSSI(i));
                  }
                  response->print("]}");
                  request->send(response);
              });

    server.on("/configurations.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/configurations.css", "text/css"); });

    server.on("/configurations.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/configurations.js", "text/javascript"); });

    server.on("/about", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/about.html", "text/html"); });

    server.on("/profile.jpg", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/profile.jpg", "image/jpeg"); });

    server.on("/updateSettings", HTTP_GET, [](AsyncWebServerRequest *request)
              { saveSettings(request); });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print("<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Rebooting</title><style>body{background:#00acc1;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Arial,sans-serif;color:#333;text-align:center;margin:0;padding:20px;}.card{background:#fff;max-width:420px;margin:40px auto 16px;padding:30px;border-radius:8px;box-shadow:0 4px 12px rgba(0,0,0,0.15);}h2{color:#00acc1;margin-top:0;}.btn{background:#00acc1;color:#fff;border:none;border-radius:6px;padding:10px 20px;font-size:14px;font-weight:600;text-transform:uppercase;cursor:pointer;margin-top:16px;}.copyright{color:#e0f7fa;font-size:12px;margin-top:12px;}</style><script>setTimeout(function(){window.location.href='/';},3000);</script></head><body><div class='card'><h2>Rebooting Device</h2><p>Device will reboot in 2 seconds...</p><button type='button' class='btn' onclick=\"location.href='/'\">Main Page</button></div><div class='copyright'>&copy; 2026 Nir Azulay. All rights reserved.</div></body></html>");
    request->send(response);
    
    // שימוש בפונקציה החדשה לתזמון אתחול מחדש
    scheduleRestart(1000);
    });

    server.on(
        "/doUpdate", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
        {
            handleDoUpdate(request, filename, index, data, len, final);
        });

    server.on(
        "/doUpload", HTTP_POST, [](AsyncWebServerRequest *request)
        { opened = false; },
        [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
        {
            handleDoUpload(request, filename, index, data, len, final);
        });

    server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM)) {
      inputMessage = request->getParam(PARAM)->value();
      inputParam = PARAM;

      deleteFile(SPIFFS, inputMessage);

      Serial.println("-inputMessage-");
      Serial.print("File=");
      Serial.println(inputMessage);
      Serial.println(" has been deleted");
            
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    AsyncResponseStream *response = request->beginResponseStream("text/plain");
    response->print("OK");
    request->send(response); });

    server.onNotFound(notFound);
    server.begin();
    Update.onProgress(printProgress);
}
//=========================
#pragma endregion WebServer

#pragma region WiFi
//=====================

// setup Wifi connection
void setupWiFi()
{
    int maxRetry = 5;
    int retryCounter = 0;
    bool staConnected = false;

    String ssid = settings.wifissid;
    ssid.trim();

    if (ssid.length() > 0)
    {
        Serial.println("Attempting WiFi STA connection to: " + ssid);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), settings.wifipassword.c_str());
        while (WiFi.status() != WL_CONNECTED && retryCounter < maxRetry)
        {
            delay(2000);
            Serial.println("Connecting to WiFi..");
            retryCounter++;
        }
        staConnected = (WiFi.status() == WL_CONNECTED);
    }

    if (staConnected)
    {
        Serial.println("Connected to the WiFi network");
        Serial.println("");
        Serial.println("WiFi connected.");
        Serial.println("IP address: ");
        IPAddress IP = WiFi.localIP();
        Serial.println(IP.toString());
        currentIP = IP.toString();
        wifiInitialized = true;
    }
    else
    {
        // Always fall back to SoftAP if Wi-Fi is not configured or failed to connect
        WiFi.disconnect(false, true);
        WiFi.mode(WIFI_OFF);
        delay(100);
        WiFi.mode(WIFI_AP_STA);

        String apName = settings.DeviceName;
        apName.trim();
        if (apName.length() == 0) {
            apName = "esp32-AirUpDownController";
        }

        String apPass = settings.AccessPointPassword;
        if (apPass.length() < 8) {
            apPass = "12345678";
        }

        bool apStarted = WiFi.softAP(apName.c_str(), apPass.c_str());
        if (!apStarted) {
            // Ultimate fallback if custom name/password was rejected by driver
            WiFi.softAP("esp32-AirUpDownController", "12345678");
        }

        IPAddress IP = WiFi.softAPIP();
        Serial.println("");
        Serial.println("WiFi not connected! Falling back to Access Point.");
        Serial.println("Setup Access point named: " + apName);
        Serial.println("Use the access-point password saved on the configuration page.");
        Serial.println("browse to the following IP: " + IP.toString());
        currentIP = IP.toString();
    }

    // Start an initial background Wi-Fi scan (120ms/ch) so SSIDs are ready for the configuration page
    WiFi.scanNetworks(true, false, false, 120);
}

//========================
#pragma endregion WiFi

#pragma region settings
//=====================

void restart()
{
    while (true)
    {
        ESP.restart();
    }
}

static bool isKnownSettingKey(const char *key)
{
    static const char *const kKnownKeys[] = {
        "DeviceName",
        "wifissid",
        "wifipassword",
        "RelayCompressorPin",
        "RelayFreePin",
        "SensorPin",
        "MaxSecForStatus",
        "SensorValueAt0PSI",
        "SensorValueAt50PSI",
        "AccessCode",
        "MessureWait",
        "Precision",
        "SimulationMode",
        "PressureCalcMethod",
        "MaxPressurePSI",
        "MaxValveOpenSec",
        "SensorFullScalePSI",
        "AccessPointPassword"
    };
    const size_t count = sizeof(kKnownKeys) / sizeof(kKnownKeys[0]);
    for (size_t i = 0; i < count; i++) {
        if (strcmp(key, kKnownKeys[i]) == 0)
            return true;
    }
    return false;
}

// Runtime validation and auto-repair of in-memory settings.
// Returns true if any field was out-of-range/inconsistent and was corrected.
bool normalizeSettings()
{
    bool fixed = false;

    settings.DeviceName.trim();
    if (settings.DeviceName.length() == 0 || settings.DeviceName.length() > 32 || settings.DeviceName.indexOf(';') >= 0) {
        settings.DeviceName = "esp32-AirUpDownController";
        fixed = true;
    }

    if (settings.RelayCompressorPin <= 0 || settings.RelayCompressorPin > 39) {
        settings.RelayCompressorPin = 16;
        fixed = true;
    }

    if (settings.RelayFreePin <= 0 || settings.RelayFreePin > 39 || settings.RelayFreePin == settings.RelayCompressorPin) {
        settings.RelayFreePin = 17;
        fixed = true;
    }

    // SensorPin must be on ADC1 (GPIO 32..39) because ADC2 cannot be used with Wi-Fi
    if (settings.SensorPin < 32 || settings.SensorPin > 39) {
        LOG_WARNING("SETTINGS", "Invalid or ADC2 SensorPin (" + String(settings.SensorPin) + "), auto-fixing to GPIO34");
        settings.SensorPin = 34;
        fixed = true;
    }

    if (settings.MaxSecForStatus < 1 || settings.MaxSecForStatus > 300) {
        settings.MaxSecForStatus = 10;
        fixed = true;
    }

    if (isnan(settings.SensorValueAt0PSI) || isnan(settings.SensorValueAt50PSI) ||
        settings.SensorValueAt0PSI < 0.0f || settings.SensorValueAt50PSI <= 0.0f ||
        settings.SensorValueAt50PSI == settings.SensorValueAt0PSI) {
        settings.SensorValueAt0PSI = 410.0f;
        settings.SensorValueAt50PSI = 3400.0f;
        fixed = true;
    }

    if (settings.AccessCode.length() > 32) {
        settings.AccessCode = settings.AccessCode.substring(0, 32);
        fixed = true;
    }

    if (isnan(settings.MessureWait) || settings.MessureWait < 0.2 || settings.MessureWait > 60.0) {
        settings.MessureWait = 1.0;
        fixed = true;
    }

    if (isnan(settings.Precision) || settings.Precision < 0.1 || settings.Precision > 10.0) {
        settings.Precision = 0.5;
        fixed = true;
    }

    if (settings.PressureCalcMethod != 1 && settings.PressureCalcMethod != 2) {
        settings.PressureCalcMethod = 1;
        fixed = true;
    }

    if (isnan(settings.MaxPressurePSI) || settings.MaxPressurePSI <= 0.0f || settings.MaxPressurePSI > 200.0f) {
        settings.MaxPressurePSI = 50.0f;
        fixed = true;
    }

    if (settings.MaxValveOpenSec < 10 || settings.MaxValveOpenSec > 3600) {
        settings.MaxValveOpenSec = 120;
        fixed = true;
    }

    if (isnan(settings.SensorFullScalePSI) || settings.SensorFullScalePSI < 0.0f || settings.SensorFullScalePSI > 300.0f) {
        settings.SensorFullScalePSI = 50.0f;
        fixed = true;
    }

    if (settings.AccessPointPassword.length() < 8 || settings.AccessPointPassword.length() > 63 ||
        settings.AccessPointPassword.indexOf(';') >= 0) {
        settings.AccessPointPassword = "12345678";
        fixed = true;
    }

    isSimulationMode = settings.SimulationMode;
    return fixed;
}

static StaticJsonDocument<2048> settingsJsonDoc;

// setup settings file
void loadSettingsfile()
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    settingsJsonDoc.clear();
    bool needsRepair = false;
    bool parsedOk = false;

    if (SPIFFS.exists("/settings.json")) {
        File file = SPIFFS.open("/settings.json", FILE_READ);
        if (file) {
            file.setBufferSize(256);
            DeserializationError error = deserializeJson(settingsJsonDoc, file);
            file.close();
            if (!error && settingsJsonDoc.is<JsonObject>()) {
                parsedOk = true;
            } else {
                LOG_WARNING("SETTINGS", "Corrupted /settings.json (" + String(error.c_str()) + "), attempting recovery");
            }
        }
    }

    if (!parsedOk && SPIFFS.exists("/settings.json.bak")) {
        File bak = SPIFFS.open("/settings.json.bak", FILE_READ);
        if (bak) {
            bak.setBufferSize(256);
            DeserializationError error = deserializeJson(settingsJsonDoc, bak);
            bak.close();
            if (!error && settingsJsonDoc.is<JsonObject>()) {
                LOG_WARNING("SETTINGS", "Recovered settings from /settings.json.bak");
                parsedOk = true;
                needsRepair = true;
            }
        }
    }

    if (!parsedOk) {
        LOG_WARNING("SETTINGS", "No valid settings file found, writing default /settings.json");
        normalizeSettings();
        saveSettingFile();
        return;
    }

    JsonObject obj = settingsJsonDoc.as<JsonObject>();

    // Check for extra / deprecated keys in the JSON file
    for (JsonPair kv : obj) {
        if (!isKnownSettingKey(kv.key().c_str())) {
            LOG_WARNING("SETTINGS", String("Removing unknown/deprecated setting key: ") + kv.key().c_str());
            needsRepair = true;
        }
    }

    // Migrate legacy BTPinCode if AccessCode was not yet present and BLESecurityEnabled was true
    if (!obj.containsKey("AccessCode") && obj.containsKey("BTPinCode") && obj["BLESecurityEnabled"].as<bool>()) {
        settings.AccessCode = obj["BTPinCode"].as<String>();
        needsRepair = true;
    }

    // Check if any required key is missing and load present values
    static const char *const kRequiredKeys[] = {
        "DeviceName", "wifissid", "wifipassword", "RelayCompressorPin",
        "RelayFreePin", "SensorPin", "MaxSecForStatus", "SensorValueAt0PSI",
        "SensorValueAt50PSI", "AccessCode", "MessureWait", "Precision",
        "SimulationMode", "PressureCalcMethod", "MaxPressurePSI",
        "MaxValveOpenSec", "SensorFullScalePSI", "AccessPointPassword"
    };
    for (size_t i = 0; i < sizeof(kRequiredKeys) / sizeof(kRequiredKeys[0]); i++) {
        if (!obj.containsKey(kRequiredKeys[i])) {
            LOG_WARNING("SETTINGS", String("Missing setting key '") + kRequiredKeys[i] + "', filling default");
            needsRepair = true;
        }
    }

    if (obj.containsKey("DeviceName") && obj["DeviceName"].is<const char*>())
        settings.DeviceName = obj["DeviceName"].as<String>();
    if (obj.containsKey("wifissid") && obj["wifissid"].is<const char*>())
        settings.wifissid = obj["wifissid"].as<String>();
    if (obj.containsKey("wifipassword") && obj["wifipassword"].is<const char*>())
        settings.wifipassword = obj["wifipassword"].as<String>();
    if (obj.containsKey("RelayCompressorPin"))
        settings.RelayCompressorPin = obj["RelayCompressorPin"].as<int>();
    if (obj.containsKey("RelayFreePin"))
        settings.RelayFreePin = obj["RelayFreePin"].as<int>();
    if (obj.containsKey("SensorPin"))
        settings.SensorPin = obj["SensorPin"].as<int>();
    if (obj.containsKey("MaxSecForStatus"))
        settings.MaxSecForStatus = obj["MaxSecForStatus"].as<int>();
    if (obj.containsKey("SensorValueAt0PSI"))
        settings.SensorValueAt0PSI = obj["SensorValueAt0PSI"].as<float>();
    if (obj.containsKey("SensorValueAt50PSI"))
        settings.SensorValueAt50PSI = obj["SensorValueAt50PSI"].as<float>();
    if (obj.containsKey("AccessCode") && obj["AccessCode"].is<const char*>())
        settings.AccessCode = obj["AccessCode"].as<String>();
    if (obj.containsKey("MessureWait"))
        settings.MessureWait = obj["MessureWait"].as<double>();
    if (obj.containsKey("Precision"))
        settings.Precision = obj["Precision"].as<double>();
    if (obj.containsKey("SimulationMode"))
        settings.SimulationMode = obj["SimulationMode"].as<bool>();
    if (obj.containsKey("PressureCalcMethod"))
        settings.PressureCalcMethod = obj["PressureCalcMethod"].as<int>();
    if (obj.containsKey("MaxPressurePSI"))
        settings.MaxPressurePSI = obj["MaxPressurePSI"].as<float>();
    if (obj.containsKey("MaxValveOpenSec"))
        settings.MaxValveOpenSec = obj["MaxValveOpenSec"].as<int>();
    if (obj.containsKey("SensorFullScalePSI"))
        settings.SensorFullScalePSI = obj["SensorFullScalePSI"].as<float>();
    if (obj.containsKey("AccessPointPassword") && obj["AccessPointPassword"].is<const char*>())
        settings.AccessPointPassword = obj["AccessPointPassword"].as<String>();

    if (normalizeSettings()) {
        needsRepair = true;
    }

    Serial.println("DEBUG: Loaded SensorValueAt0PSI: " + String(settings.SensorValueAt0PSI, 3));
    Serial.println("DEBUG: Loaded SensorValueAt50PSI: " + String(settings.SensorValueAt50PSI, 3));
    LOG_INFO("SETTINGS", "Loaded simulation mode: " + String(isSimulationMode ? "enabled" : "disabled"));
    LOG_INFO("SETTINGS", "Loaded PressureCalcMethod: " + String(settings.PressureCalcMethod));

    if (needsRepair) {
        LOG_INFO("SETTINGS", "Auto-fixing settings.json at runtime and saving clean configuration");
        saveSettingFile();
    }
}

void saveSettingFile()
{
    Serial.println("saveSettingFile");
    settingsJsonDoc.clear();

    settingsJsonDoc["DeviceName"] = settings.DeviceName;
    settingsJsonDoc["wifissid"] = settings.wifissid;
    settingsJsonDoc["wifipassword"] = settings.wifipassword;
    settingsJsonDoc["RelayCompressorPin"] = settings.RelayCompressorPin;
    settingsJsonDoc["RelayFreePin"] = settings.RelayFreePin;
    settingsJsonDoc["SensorPin"] = settings.SensorPin;
    settingsJsonDoc["MaxSecForStatus"] = settings.MaxSecForStatus;
    settingsJsonDoc["SensorValueAt0PSI"] = settings.SensorValueAt0PSI;
    settingsJsonDoc["SensorValueAt50PSI"] = settings.SensorValueAt50PSI;
    settingsJsonDoc["AccessCode"] = settings.AccessCode;
    settingsJsonDoc["MessureWait"] = settings.MessureWait;
    settingsJsonDoc["Precision"] = settings.Precision;
    settingsJsonDoc["SimulationMode"] = settings.SimulationMode;
    settingsJsonDoc["PressureCalcMethod"] = settings.PressureCalcMethod;
    settingsJsonDoc["MaxPressurePSI"] = settings.MaxPressurePSI;
    settingsJsonDoc["MaxValveOpenSec"] = settings.MaxValveOpenSec;
    settingsJsonDoc["SensorFullScalePSI"] = settings.SensorFullScalePSI;
    settingsJsonDoc["AccessPointPassword"] = settings.AccessPointPassword;

    String jsonOutput;
    serializeJsonPretty(settingsJsonDoc, jsonOutput);
    if (jsonOutput.length() < 20) {
        Serial.println("- failed to serialize settings JSON");
        return;
    }

    // Remove any old/backup files first using POSIX unlink so no old values or conflicting SPIFFS entries remain
    unlink("/spiffs/settings.json.tmp");
    unlink("/spiffs/settings.json.bak");
    unlink("/spiffs/settings.json");

    FILE *f = fopen("/spiffs/settings.json", "w");
    if (!f) {
        Serial.println("- failed to open /spiffs/settings.json for writing");
        return;
    }

    setvbuf(f, NULL, _IONBF, 0);
    size_t written = fwrite(jsonOutput.c_str(), 1, jsonOutput.length(), f);
    fflush(f);
    fclose(f);

    listDir(SPIFFS, "/", 0);
    Serial.printf("finish to save settings file (%u bytes)\n", (unsigned int)written);
}

void printSettingsFile()
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    File file = SPIFFS.open("/settings.json");
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.println("Settings file is present. Passwords are not printed.");
    file.close();
}

void setupBlueTooth()
{
    LOG_INFO("BLE", "Setting up Bluetooth...");
    
    // Create the BLE Device
    BLEDevice::init(settings.DeviceName.c_str());
    BLEDevice::setMTU(517);
    LOG_DEBUG("BLE", "BLE Device initialized with name: " + settings.DeviceName);

    LOG_INFO("BLE", "Application access code is " + String(settings.AccessCode.length() == 0 ? "not set" : "set"));

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyBLEServerCallbacks());
    LOG_DEBUG("BLE", "BLE Server created with callbacks");

    // Create the BLE Service
    BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID));
    LOG_DEBUG("BLE", "BLE Service created with UUID: " + String(SERVICE_UUID));

    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
        BLEUUID(CHAR1_UUID),
        BLECharacteristic::PROPERTY_NOTIFY
    );
    LOG_DEBUG("BLE", "Notification characteristic created with UUID: " + String(CHAR1_UUID));

    pCharacteristic_2 = pService->createCharacteristic(
        BLEUUID(CHAR2_UUID),
        BLECharacteristic::PROPERTY_READ   |
        BLECharacteristic::PROPERTY_WRITE  |
        BLECharacteristic::PROPERTY_WRITE_NR
    );
    LOG_DEBUG("BLE", "Command characteristic created with UUID: " + String(CHAR2_UUID));

    // Create a BLE Descriptor
    pDescr = new BLEDescriptor((uint16_t)0x2901);
    pDescr->setValue("Air Controller Status");
    pCharacteristic->addDescriptor(pDescr);
    
    pBLE2902 = new BLE2902();
    pBLE2902->setNotifications(true);
    
    // Add all Descriptors here
    pCharacteristic->addDescriptor(pBLE2902);
    pCharacteristic_2->addDescriptor(new BLE2902());
    LOG_DEBUG("BLE", "Descriptors added to characteristics");
    
    // After defining the descriptors, set the callback functions
    pCharacteristic_2->setCallbacks(new CharacteristicCallBack());
    LOG_DEBUG("BLE", "Callbacks set for command characteristic");
    
    // Start the service
    pService->start();
    LOG_DEBUG("BLE", "BLE Service started");

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);  // Enable scan response
    pAdvertising->setMinPreferred(0x06);  // Improve connection rate
    pAdvertising->setMaxPreferred(0x12);
    BLEDevice::startAdvertising();
    LOG_INFO("BLE", "BLE advertising started");
 
    LOG_INFO("BLE", "BLE Server is ready for connections");
}

#pragma endregion settings

void setup()
{
    Serial.begin(115200);
    LOG_INFO("SYSTEM", "Starting Air Controller v" + String(VERSION_SHORT));
    
    debugMode = false; // Ensure debug mode is off on startup
    LOG_DEBUG("SYSTEM", "Loading settings file");
    printSettingsFile();
    loadSettingsfile();
    
    // Ensure simulation mode is properly initialized from settings
    isSimulationMode = settings.SimulationMode;
    LOG_INFO("SYSTEM", "Simulation mode initialized: " + String(isSimulationMode ? "enabled" : "disabled"));
    
    LOG_INFO("WIFI", "Setting up WiFi connection");
    setupWiFi();
    
    LOG_INFO("WEB", "Setting up web server");
    setupWebServer();
    
    LOG_DEBUG("SYSTEM", "Setup running on core " + String(xPortGetCoreID()));
    
    LOG_INFO("BLE", "Initializing Bluetooth");
    setupBlueTooth();
    
    // initiate relays Legs
    LOG_DEBUG("GPIO", "Configuring GPIO pins");
    pinMode(settings.RelayCompressorPin, OUTPUT);
    pinMode(settings.RelayFreePin, OUTPUT);
    pinMode(settings.SensorPin, INPUT);
    // Relays on this board are active high. Hold both off until a command.
    digitalWrite(settings.RelayCompressorPin, LOW);
    digitalWrite(settings.RelayFreePin, LOW);

    // clear memory
    targetPressure = 0;
    currentPressure = 0;

    LOG_INFO("SYSTEM", "Setup complete");
}



static void notifyBLEPayload(const String &payload)
{
    if (!deviceConnected || pCharacteristic == nullptr)
        return;

    const size_t maxChunk = 180;
    size_t totalLen = payload.length();
    if (totalLen <= maxChunk) {
        pCharacteristic->setValue(payload.c_str());
        pCharacteristic->notify();
        return;
    }

    for (size_t offset = 0; offset < totalLen; offset += maxChunk) {
        String chunk = payload.substring(offset, min(offset + maxChunk, totalLen));
        pCharacteristic->setValue(chunk.c_str());
        pCharacteristic->notify();
        delay(15);
    }
}

void sendWifiNetworksToMobile()
{
    if (!deviceConnected || pCharacteristic == nullptr)
        return;
    int n = WiFi.scanComplete();
    if (n < 0)
        return;

    String listStr = "";
    int added = 0;
    for (int i = 0; i < n && added < 15; i++) {
        String ssid = WiFi.SSID(i);
        ssid.trim();
        if (ssid.length() == 0) continue;
        ssid.replace("\"", "");
        ssid.replace("'", "");
        ssid.replace("\\", "");
        ssid.replace("{", "");
        ssid.replace("}", "");
        ssid.replace(";", "");
        ssid.replace("|", "");
        if (ssid.length() == 0) continue;

        bool duplicate = false;
        for (int j = 0; j < i; j++) {
            String prev = WiFi.SSID(j);
            prev.trim();
            if (prev == ssid) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) continue;

        if (added > 0) listStr += ";";
        listStr += ssid + "|" + String((int)WiFi.RSSI(i));
        added++;
    }

    String json = String("{\"type\":\"wifi_list\",\"networks\":\"") + listStr + "\"}";
    LOG_INFO("BLE", "Sending Wi-Fi list to mobile: " + json);
    notifyBLEPayload(json);
}

void BLESync()
{
    LOG_VERBOSE("BLE", "BLESync started");
    
    if (deviceConnected) {
        LOG_VERBOSE("BLE", "Device is connected");
        
        if (firstTimeConnceted) {
            firstTimeConnceted = false;
            if (settings.AccessCode.length() == 0 || bleAuthorized) {
                LOG_INFO("BLE", "First connection - sending settings");
                String settingsJson = getSettingsJSON();
                notifyBLEPayload(settingsJson);
            } else {
                LOG_INFO("BLE", "First connection - access code required");
                BLEBufferValue = "";
                notifyBLEPayload("{\"type\":\"auth\",\"ok\":false,\"required\":true}");
                return;
            }
        }

        if (settings.AccessCode.length() > 0 && !bleAuthorized) {
            BLEBufferValue = "";
            return;
        }

        if (BLEBufferValue.length() > 0) {
            LOG_DEBUG("BLE", "Sending status update: " + BLEBufferValue);
            notifyBLEPayload(BLEBufferValue);
            
            BLEBufferValue = "";
            LOG_DEBUG("BLE", "Status update sent");
        }
    }
    
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        LOG_INFO("BLE", "Device disconnected, restarting advertising");
        pServer->startAdvertising(); // restart advertising
        oldDeviceConnected = deviceConnected;
    }
    
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        LOG_INFO("BLE", "New device connected");
        oldDeviceConnected = deviceConnected;
    }
    
    LOG_VERBOSE("BLE", "BLESync finished");
}

void loop()
{
    if (upgradeInProgress) {
        unsigned long lastChunk = lastUpdateChunkMs;
        unsigned long now = millis();
        if (lastChunk > 0 && (long)(now - lastChunk) > 20000L) {
            if (Update.isRunning()) {
                Update.abort();
            }
            upgradeInProgress = false;
        } else {
            delay(20);
            return;
        }
    }

    if (pendingSaveSettings) {
        pendingSaveSettings = false;
        normalizeSettings();
        saveSettingFile();
    }

    if (pendingRestart && (long)(millis() - pendingRestartAtMs) >= 0) {
        pendingRestart = false;
        LOG_INFO("SYSTEM", "Restarting after settings save");
        Serial.flush();
        ESP.restart();
        return;
    }

    if (pendingStartWifiScan && (long)(millis() - pendingStartWifiScanAtMs) >= 0) {
        pendingStartWifiScan = false;
        int n = WiFi.scanComplete();
        if (n != WIFI_SCAN_RUNNING) {
            if (n >= 0) WiFi.scanDelete();
            WiFi.scanNetworks(true, false, false, 120);
        }
    }

    if (pendingBleWifiScan && !pendingStartWifiScan) {
        int n = WiFi.scanComplete();
        if (n >= 0) {
            pendingBleWifiScan = false;
            sendWifiNetworksToMobile();
        }
    }

    static unsigned long lastExecutionTime = 0;
    static unsigned long lastBLEDisconnectTime = 0;
    static unsigned long lastBLENotifyTime = 0;
    unsigned long currentTime = millis();
    unsigned long interval = 1000 * settings.MessureWait;

    // תן לשרת האינטרנט זמן לעבד בקשות
    yield();

    // Skip if not enough time has passed
    if (currentTime - lastExecutionTime < interval) {
        // בדוק אם צריך לטפל בניתוק BLE
        if (!deviceConnected && oldDeviceConnected && 
            (currentTime - lastBLEDisconnectTime >= 500)) {
            LOG_INFO("BLE", "Device disconnected, restarting advertising");
            pServer->startAdvertising(); // restart advertising
            oldDeviceConnected = deviceConnected;
            lastBLEDisconnectTime = currentTime;
        }
        return;
    }

    lastExecutionTime = currentTime;
    
    // skip x tiks to avoid reading pressure every loop
    if (tikingCounter == 0 || measureNow) {
        measureNow = false;
        LOG_VERBOSE("PRESSURE", "Reading current pressure");
        readCurrentPressure();
    }

    tikingCounter++;

    LOG_VERBOSE("BLE", "Syncing BLE status");
    BLESync();

    if (operationMode != Stop || valveSessionActive) {
        LOG_VERBOSE("AIR", "Operating air system");
        OperateAir();
    } else {
        LOG_VERBOSE("AIR", "System in stop mode");
        StopAir();
        
        // In simulation mode, we still need to update the pressure even in stop mode
        // but don't change the pressure value
        if (isSimulationMode) {
            LOG_VERBOSE("PRESSURE", "Simulation mode - reading pressure in stop mode");
            currentPressure = readPressureFromSensor(); // This won't change the pressure in stop mode
        } else {
            currentPressure = readPressureFromSensor();
        }
        
        SendStatusToMobile();
    }

    LOG_VERBOSE("SYSTEM", "Loop execution completed");

    if (tikingCounter > calculateDelatFactor())
        tikingCounter = 0;

    // תן לשרת האינטרנט זמן לעבד בקשות בסוף הלולאה
    yield();
    
    delay(10);
}

float applyPressureLimits(float pressure) {
    if (pressure < 0)
        pressure = 0;
    if (settings.SensorFullScalePSI > 0 && pressure > settings.SensorFullScalePSI * 1.25f) {
        pressure = settings.SensorFullScalePSI;
    }
    pressureValid = true;
    return pressure;
}

float calculatePressureMethod1(int sensorVal) {
    lastRawADC = sensorVal;
    lastVoltage = (float)sensorVal * (3.3f / 4095.0f);

    float spanPsi = settings.SensorFullScalePSI > 0.0f ? settings.SensorFullScalePSI : 50.0f;
    float range = settings.SensorValueAt50PSI - settings.SensorValueAt0PSI;
    if (range == 0.0f) {
        pressureValid = true;
        return 0;
    }

    float pressure = (sensorVal - settings.SensorValueAt0PSI) / (range / spanPsi);
    if (pressure < 1.0f) pressure = 0.0f;
    return applyPressureLimits(pressure);
}

float calculatePressureMethod2(int sensorVal) {
    lastRawADC = sensorVal;
    float voltageAtADC = (float)sensorVal * (3.3f / 4095.0f);
    lastVoltage = voltageAtADC;
    float sensorVoltage = voltageAtADC * 2.0f;

    float spanPsi = settings.SensorFullScalePSI > 0.0f ? settings.SensorFullScalePSI : 50.0f;
    float v0 = settings.SensorValueAt0PSI;
    float vFull = settings.SensorValueAt50PSI;
    if (vFull == v0) {
        pressureValid = true;
        return 0;
    }

    float pressure = (sensorVoltage - (v0 * 2.0f)) * (spanPsi / ((vFull - v0) * 2.0f));
    return applyPressureLimits(pressure);
}

float readPressureFromSensor()
{
    LOG_DEBUG("PRESSURE", "Reading pressure from sensor");

    if (isSimulationMode) {
        LOG_DEBUG("PRESSURE", "Using simulation mode");
        float pressureChange = 0;
        if (operationMode == Inflate) {
            pressureChange = 0.5;
            LOG_DEBUG("PRESSURE", "Simulation: Inflating, pressure change: +0.5 PSI");
        } else if (operationMode == Deflate) {
            pressureChange = -0.3;
            LOG_DEBUG("PRESSURE", "Simulation: Deflating, pressure change: -0.3 PSI");
        }
        currentPressure += pressureChange;
        if (currentPressure < 0) currentPressure = 0;
        if (currentPressure > settings.MaxPressurePSI) currentPressure = settings.MaxPressurePSI;
        pressureValid = true;

        // Simulate rawADC and voltage based on currentPressure
        float range = settings.SensorValueAt50PSI - settings.SensorValueAt0PSI;
        float onePSI = range / 50.0f;
        float zeroPSI = settings.SensorValueAt0PSI;
        lastRawADC = static_cast<int>(zeroPSI + (currentPressure * onePSI));
        lastVoltage = (float)lastRawADC * (3.3f / 4095.0f); // Assuming 3.3V ref, 12-bit ADC

        LOG_INFO("PRESSURE", "Simulated pressure: " + String(currentPressure) + " PSI");
        return currentPressure;
    }

    // Read 50 samples from the sensor
    const int NUM_SAMPLES = 25;
    int samples[NUM_SAMPLES];
    
    LOG_DEBUG("PRESSURE", "Reading " + String(NUM_SAMPLES) + " samples from sensor");
    
    // Collect samples
    for(int i = 0; i < NUM_SAMPLES; i++) {
        if (upgradeInProgress || pendingSaveSettings || pendingRestart) {
            return currentPressure;
        }
        samples[i] = analogRead(settings.SensorPin);
        delay(2); // Small delay between readings
    }
    
    // Sort samples
    for(int i = 0; i < NUM_SAMPLES-1; i++) {
        for(int j = i + 1; j < NUM_SAMPLES; j++) {
            if(samples[i] > samples[j]) {
                int temp = samples[i];
                samples[i] = samples[j];
                samples[j] = temp;
            }
        }
    }
    
    // Calculate average of middle 30% (35% to 65%)
    int startIdx = NUM_SAMPLES * 0.35;
    int endIdx = NUM_SAMPLES * 0.65;
    long sum = 0;
    
    for(int i = startIdx; i < endIdx; i++) {
        sum += samples[i];
    }
    
    int sensorVal = sum / (endIdx - startIdx);
    LOG_DEBUG("PRESSURE", "Filtered sensor value: " + String(sensorVal));

    float pressure_pascal = 0;
    if (settings.PressureCalcMethod == 2) {
        pressure_pascal = calculatePressureMethod2(sensorVal);
    } else {
        pressure_pascal = calculatePressureMethod1(sensorVal);
    }
    float pressure_bar = pressure_pascal / 14.7;

    LOG_INFO("PRESSURE", "Measured pressure: " + String(pressure_pascal) + " PSI (" + String(pressure_bar) + " BAR)");
    return pressure_pascal;
}

void waitForPressureToSettle()
{
    unsigned long startTime = millis();
    unsigned long waitMs = (unsigned long)(1000.0 * settings.MessureWait);
    while (millis() - startTime < waitMs) {
        if (upgradeInProgress || pendingSaveSettings || pendingRestart) {
            return;
        }
        yield();
        delay(10);
    }
}

void refuseAir(const char *reason) {
    LOG_ERROR("AIR", String("Stopped: ") + reason);
    valveCommandEpoch++;
    valveSessionActive = false;
    StopAir();
    SendStatusToMobile();
}

void readCurrentPressure()
{
    LOG_DEBUG("PRESSURE", "Starting pressure measurement");

    if (isSimulationMode || operationMode == Stop) {
        currentPressure = readPressureFromSensor();
        LOG_INFO("PRESSURE", "Current pressure: " + String(currentPressure) + " PSI");
        SendStatusToMobile();
        return;
    }

    unsigned long epoch = valveCommandEpoch;
    OperationMode backupCurrentValveState = operationMode;
    LOG_DEBUG("PRESSURE", "Saving current operation mode: " + String(backupCurrentValveState));

    StopAir();
    waitForPressureToSettle();
    if (valveCommandEpoch != epoch || pendingSaveSettings || pendingRestart || upgradeInProgress)
        return;

    currentPressure = readPressureFromSensor();

    const int maxStableReads = 3;
    for (int attempt = 0; attempt < maxStableReads; attempt++) {
        if (valveCommandEpoch != epoch || pendingSaveSettings || pendingRestart || upgradeInProgress)
            return;
        float nextPressure = readPressureFromSensor();
        if (fabs(nextPressure - currentPressure) <= kStablePsi) {
            currentPressure = nextPressure;
            break;
        }
        currentPressure = nextPressure;
        waitForPressureToSettle();
    }

    if (valveCommandEpoch != epoch)
        return;

    sessionHasReading = true;
    operationMode = backupCurrentValveState;
    if (operationMode == Inflate)
        InflateAir();
    else if (operationMode == Deflate)
        DeflateAir();
    else
        StopAir();

    LOG_INFO("PRESSURE", "Current pressure: " + String(currentPressure) + " PSI");
    SendStatusToMobile();
}

void StopAir()
{
    LOG_DEBUG("AIR", "Stopping air flow");
    operationMode = Stop;
    digitalWrite(settings.RelayCompressorPin, LOW);
    digitalWrite(settings.RelayFreePin, LOW);
}

void DeflateAir()
{
    LOG_INFO("AIR", "Deflating air");
    operationMode = Deflate;
    digitalWrite(settings.RelayCompressorPin, LOW);
    digitalWrite(settings.RelayFreePin, HIGH);
}

void InflateAir()
{
    LOG_INFO("AIR", "Inflating air");
    operationMode = Inflate;
    digitalWrite(settings.RelayFreePin, LOW);
    digitalWrite(settings.RelayCompressorPin, HIGH);
}

void OperateAir()
{
    LOG_DEBUG("AIR", "Operating air system - Target: " + String(targetPressure) + " PSI, Current: " + String(currentPressure) + " PSI");

    if (valveSessionActive && settings.MaxValveOpenSec > 0 &&
        millis() - valveSessionStartMs >= (unsigned long)settings.MaxValveOpenSec * 1000UL) {
        refuseAir("valve open time exceeded");
        return;
    }

    if ((targetPressure - currentPressure) > settings.Precision) {
        LOG_INFO("AIR", "Pressure too low, inflating");
        InflateAir();
    } else if ((currentPressure - targetPressure) > settings.Precision) {
        LOG_INFO("AIR", "Pressure too high, deflating");
        DeflateAir();
    } else if (operationMode != Stop || valveSessionActive) {
        LOG_INFO("AIR", "Pressure at target, stopping");
        valveSessionActive = false;
        StopAir();
        SendTargetReachedNotification();
    }
}

void SendStatusToMobile()
{
    LOG_DEBUG("BLE", "Preparing status update for mobile");
    BLEBufferValue = getPressureJSON();
    LOG_VERBOSE("BLE", "Status update prepared: " + BLEBufferValue);
}

void SendSettingsToMobile()
{
    LOG_INFO("BLE", "Sending settings to mobile");
    BLEBufferValue = getSettingsJSON();
    LOG_VERBOSE("BLE", "Settings to send: " + BLEBufferValue);
    
    BLESync();
    
    LOG_INFO("BLE", "Settings sent to mobile");
}

void handleDebugModeCommand(DynamicJsonDocument &doc);
void handleAuthCommand(DynamicJsonDocument &doc);
void handleScanWifiCommand(DynamicJsonDocument &doc);

void sendAuthState(bool ok)
{
    if (!deviceConnected || pCharacteristic == nullptr)
        return;
    bool required = settings.AccessCode.length() > 0 && !ok;
    String json = String("{\"type\":\"auth\",\"ok\":") + (ok ? "true" : "false") + ",\"required\":" + (required ? "true" : "false") + "}";
    pCharacteristic->setValue(json.c_str());
    pCharacteristic->notify();
}

bool accessGranted()
{
    return settings.AccessCode.length() == 0 || bleAuthorized;
}

void handleBLEMessage(String Message)
{
    LOG_INFO("BLE", "Processing BLE message");
    String sMessage = Message;

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, sMessage);
    
    if (error) {
        LOG_ERROR("BLE", "Failed to parse JSON: " + String(error.c_str()));
        return;
    }

    if (!doc.containsKey("command")) {
        LOG_ERROR("BLE", "Missing 'command' field in message");
        return;
    }
    
    int cmd = doc["command"].as<int>();
    LOG_INFO("BLE", "Command received: " + String(cmd));

    if (cmd == 8) {
        handleAuthCommand(doc);
        return;
    }

    if (!accessGranted() && cmd != 1) {
        LOG_WARNING("BLE", "Command rejected until the access code is accepted");
        sendAuthState(false);
        return;
    }
    
    if (cmd == 1) {
        // Stop command
        handleStopCommand();
    }
    else if (cmd == 2) {
        // Start command
        handleStartCommand(doc);
    }
    else if (cmd == 3) {
        // Settings command
        handleSettingsCommand(doc);
    }
    else if (cmd == 4) {
        // Simulation command
        handleSimulationCommand(doc);
    }
    else if (cmd == 5) {
        // Log level command
        handleLogLevelCommand(doc);
    }
    else if (cmd == 6) {
        // Get settings command
        handleGetSettingsCommand();
    }
    else if (cmd == 7) {
        // Debug mode command
        handleDebugModeCommand(doc);
    }
    else if (cmd == 9) {
        // Scan Wi-Fi networks command
        handleScanWifiCommand(doc);
    }
    else {
        // Unknown command
        LOG_WARNING("BLE", "Unknown command received: " + String(cmd));
    }
    
    LOG_INFO("BLE", "BLE message processing completed");
}

void handleStopCommand() {
    LOG_INFO("BLE", "Stop command received");
    valveCommandEpoch++;
    valveSessionActive = false;
    StopAir();
    SendStatusToMobile();
    BLESync();
}

void handleStartCommand(DynamicJsonDocument &doc) {
    if (!doc.containsKey("targetPressure")) {
        LOG_ERROR("BLE", "Missing 'targetPressure' field for start command");
        return;
    }
    float requested = doc["targetPressure"].as<float>();
    float limit = settings.MaxPressurePSI > 0 ? settings.MaxPressurePSI : 50.0f;
    if (settings.SensorFullScalePSI > 0 && settings.SensorFullScalePSI < limit)
        limit = settings.SensorFullScalePSI;
    if (requested <= 0 || requested > limit) {
        LOG_ERROR("BLE", "Rejected target pressure: " + String(requested));
        refuseAir("target pressure is outside the allowed range");
        return;
    }
    targetPressure = requested;
    valveCommandEpoch++;
    valveSessionStartMs = millis();
    sessionProgressMs = millis();
    sessionBaselinePressure = -1;
    valveSessionActive = true;
    sessionHasReading = true;
    tikingCounter = 1;
    LOG_INFO("BLE", "Start command received with target pressure: " + String(targetPressure));
    OperateAir();
    SendStatusToMobile();
    BLESync();
}

void handleSettingsCommand(DynamicJsonDocument &doc) {
    LOG_INFO("BLE", "Settings update command received");
    if (doc.containsKey("DeviceName") && doc["DeviceName"].is<const char*>()) {
        String name = doc["DeviceName"].as<String>();
        name.trim();
        settings.DeviceName = name;
    }
    if (doc.containsKey("wifissid") && doc["wifissid"].is<const char*>()) {
        String ssid = doc["wifissid"].as<String>();
        ssid.trim();
        settings.wifissid = ssid;
        if (ssid.length() == 0) {
            settings.wifipassword = "";
        }
    }
    if (doc.containsKey("wifipassword") && doc["wifipassword"].is<const char*>()) {
        String pwd = doc["wifipassword"].as<String>();
        pwd.trim();
        settings.wifipassword = pwd;
    }
    if (doc.containsKey("RelayCompressorPin"))
        settings.RelayCompressorPin = doc["RelayCompressorPin"].as<int>();
    if (doc.containsKey("RelayFreePin"))
        settings.RelayFreePin = doc["RelayFreePin"].as<int>();
    if (doc.containsKey("SensorPin"))
        settings.SensorPin = doc["SensorPin"].as<int>();
    if (doc.containsKey("MaxSecForStatus"))
        settings.MaxSecForStatus = doc["MaxSecForStatus"].as<int>();
    if (doc.containsKey("SensorValueAt0PSI"))
        settings.SensorValueAt0PSI = doc["SensorValueAt0PSI"].as<float>();
    if (doc.containsKey("SensorValueAt50PSI"))
        settings.SensorValueAt50PSI = doc["SensorValueAt50PSI"].as<float>();
    
    // Debug: Print the received sensor values
    Serial.println("DEBUG: Received SensorValueAt0PSI: " + String(settings.SensorValueAt0PSI, 3));
    Serial.println("DEBUG: Received SensorValueAt50PSI: " + String(settings.SensorValueAt50PSI, 3));
    if (doc.containsKey("MessureWait"))
        settings.MessureWait = doc["MessureWait"].as<double>();
    if (doc.containsKey("Precision"))
        settings.Precision = doc["Precision"].as<double>();
    if (doc.containsKey("PressureCalcMethod"))
        settings.PressureCalcMethod = doc["PressureCalcMethod"].as<int>();
    if (doc.containsKey("SensorFullScalePSI"))
        settings.SensorFullScalePSI = doc["SensorFullScalePSI"].as<float>();
    if (doc.containsKey("MaxPressurePSI"))
        settings.MaxPressurePSI = doc["MaxPressurePSI"].as<float>();
    if (doc.containsKey("MaxValveOpenSec"))
        settings.MaxValveOpenSec = doc["MaxValveOpenSec"].as<int>();
    if (doc.containsKey("AccessPointPassword") && doc["AccessPointPassword"].is<const char*>()) {
        String apPwd = doc["AccessPointPassword"].as<String>();
        apPwd.trim();
        if (apPwd.length() >= 8 && apPwd.indexOf(';') < 0)
            settings.AccessPointPassword = apPwd;
    }
    if (doc.containsKey("ClearAccessCode") && (doc["ClearAccessCode"].as<bool>() || doc["ClearAccessCode"].as<String>() == "true" || doc["ClearAccessCode"].as<String>() == "1")) {
        settings.AccessCode = "";
    } else if (doc.containsKey("AccessCode") && doc["AccessCode"].is<const char*>()) {
        String code = doc["AccessCode"].as<String>();
        code.trim();
        if (code.length() > 0)
            settings.AccessCode = code;
    }

    // Check if SimulationMode is included in the settings update
    if (doc.containsKey("SimulationMode")) {
        if (doc["SimulationMode"].is<bool>()) {
            settings.SimulationMode = doc["SimulationMode"].as<bool>();
        } else {
            String simStr = doc["SimulationMode"].as<String>();
            settings.SimulationMode = (simStr.equalsIgnoreCase("true") || simStr == "1");
        }
        isSimulationMode = settings.SimulationMode;
        LOG_INFO("BLE", "Simulation mode updated through settings: " + String(isSimulationMode ? "enabled" : "disabled"));
        
        // Reset initial pressure if simulation mode is enabled
        if (isSimulationMode) {
            currentPressure = 0;
        }
    }
    
    normalizeSettings();
    LOG_INFO("BLE", "Saving settings and restarting");
    pendingSaveSettings = true;
    pendingRestart = true;
    pendingRestartAtMs = millis() + 600;
}

void handleSimulationCommand(DynamicJsonDocument &doc) {
    if (!doc.containsKey("enabled")) {
        LOG_ERROR("BLE", "Missing 'enabled' field for simulation command");
        return;
    }
    isSimulationMode = doc["enabled"].as<bool>();
    settings.SimulationMode = isSimulationMode;  // Update the settings struct
    LOG_INFO("BLE", "Simulation mode: " + String(isSimulationMode ? "enabled" : "disabled"));
    
    // Save the setting to file on main loop
    LOG_INFO("BLE", "Queueing simulation mode setting save");
    pendingSaveSettings = true;
    
    if (isSimulationMode) {
        currentPressure = 0; // Reset initial pressure in simulation mode
    }
    digitalWrite(settings.RelayCompressorPin, LOW);
    digitalWrite(settings.RelayFreePin, LOW);
}

void handleLogLevelCommand(DynamicJsonDocument &doc) {
    if (!doc.containsKey("level")) {
        LOG_ERROR("BLE", "Missing 'level' field for log level command");
        return;
    }
    int level = doc["level"].as<int>();
    LOG_INFO("BLE", "Setting log level to: " + String(level));
    setLogLevel(level);
}

void handleGetSettingsCommand() {
    LOG_INFO("BLE", "Get settings command received");
    SendSettingsToMobile();
    if (WiFi.scanComplete() >= 0) {
        sendWifiNetworksToMobile();
    }
}

void handleScanWifiCommand(DynamicJsonDocument &doc) {
    bool forceRescan = doc.containsKey("rescan") ? doc["rescan"].as<bool>() : true;
    int n = WiFi.scanComplete();
    if (!forceRescan && n >= 0) {
        sendWifiNetworksToMobile();
        return;
    }
    if (n != WIFI_SCAN_RUNNING) {
        pendingStartWifiScan = true;
        pendingStartWifiScanAtMs = millis() + 80;
    }
    pendingBleWifiScan = true;
}

void handleAuthCommand(DynamicJsonDocument &doc) {
    String given = doc["accessCode"] | "";
    if (given.length() > 32)
        given = given.substring(0, 32);
    if (settings.AccessCode.length() == 0 || given == settings.AccessCode) {
        bleAuthorized = true;
        LOG_INFO("BLE", "Access code accepted");
        sendAuthState(true);
        SendSettingsToMobile();
    } else {
        bleAuthorized = false;
        LOG_WARNING("BLE", "Access code rejected");
        sendAuthState(false);
    }
}

void handleDebugModeCommand(DynamicJsonDocument &doc) {
    if (!doc.containsKey("enabled")) {
        LOG_ERROR("BLE", "Missing 'enabled' field for debug_mode command");
        return;
    }
    debugMode = doc["enabled"].as<bool>();
    LOG_INFO("BLE", "Debug mode set to: " + String(debugMode ? "enabled" : "disabled"));
    // Optionally, send a response with the current debug mode status
}

double calculateDelatFactor()
{
    // every PSI is 1 5 sec

    double delta = targetPressure - currentPressure;
    int multiplier = 5;
    if (delta <0) multiplier = 10;
    
    double factor = abs(delta) * multiplier;

    if (factor == 0 || factor < 1)
        factor = 5;

    if (factor > settings.MaxSecForStatus)
        factor = settings.MaxSecForStatus;

    //Serial.print(" calculateDelatFactor: ");
    //Serial.println(factor);

    return factor;
}

// Add this new function to send target pressure reached notification
void SendTargetReachedNotification() {
    LOG_INFO("BLE", "Target pressure reached, sending notification");
    
    String JSON = "{";
    JSON += "'type': 'target_reached',";
    JSON += "'targetPressure': '" + String(targetPressure) + "',";
    JSON += "'currentPressure': '" + String(currentPressure, 1) + "'";
    JSON += "}";
    
    LOG_DEBUG("BLE", "Target reached notification: " + JSON);
    
    if (deviceConnected) {
        pCharacteristic->setValue(JSON.c_str());
        pCharacteristic->notify();
        LOG_INFO("BLE", "Target reached notification sent");
    } else {
        LOG_WARNING("BLE", "Device not connected, couldn't send target reached notification");
    }
}