#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLESecurity.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <Update.h>
#include "main.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include "settings.h"
#include <version.h>

int backupProgress = 0;
bool wifiInitialized = false;
int tikingCounter = 0;
bool upgradeInProgress=false;
String currentIP= "";


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

WiFiClientSecure client;
#pragma endregion webServer declaration

#pragma region WiFi declaration
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3 * 60 * 60);
#pragma endregion WiFi declaration

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
      LOG_INFO("BLE", "Client connected");
      // Enable multiple connections
      BLEDevice::startAdvertising();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
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
      LOG_VERBOSE("BLE", "Last character: " + String(pChar2_value_string.charAt(pChar2_value_string.length()-1)));

      incompleteBLEMessage += pChar2_value_string;

      // Check if message is complete (ends with })
      if (pChar2_value_string.indexOf("}") >= 0) {
        LOG_INFO("BLE", "Complete message received: " + incompleteBLEMessage);
        handleBLEMessage(incompleteBLEMessage);
        incompleteBLEMessage = "";
      } else {
        LOG_DEBUG("BLE", "Partial message received, waiting for more data");
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
    Serial.println("Update");
    upgradeInProgress=true;

    if (!index)
    {
        content_len = request->contentLength();
        // if filename includes spiffs, update the spiffs partition
        int cmd = (filename.indexOf("spiffs") > -1) ? U_PART : U_FLASH;
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd))
        {
            Update.printError(Serial);
        }
    }
    if (Update.write(data, len) != len)
    {
        Update.printError(Serial);
        Serial.printf("Progress: %d%%\n", (Update.progress() * 100) / Update.size());
        printProgress(Update.progress(), Update.size());
    }

    if (final)
    {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Ok");
        response->addHeader("Refresh", "30");
        response->addHeader("Location", "/");
        request->send(response);
        if (!Update.end(true))
        {
            Update.printError(Serial);
        }
        else
        {
            Serial.println("Update complete");
            Serial.flush();
            ESP.restart();
        }
    }
    upgradeInProgress=false;
}

void handleDoUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    if (!index)
    {
        content_len = request->contentLength();
        Serial.printf("UploadStart: %s\n", filename.c_str());
    }

    if (opened == false)
    {
        opened = true;
        file = SPIFFS.open(String("/") + filename, FILE_WRITE);
        if (!file)
        {
            Serial.println("- failed to open file for writing");
            return;
        }
    }

    if (file.write(data, len) != len)
    {
        Serial.println("- failed to write");
        return;
    }

    if (final)
    {

        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Ok");
        response->addHeader("Refresh", "20");
        response->addHeader("Location", "/filesystem");
        request->send(response);
        file.close();
        opened = false;
        Serial.println("---------------");
        Serial.println("Upload complete");
        listDir(SPIFFS, "/", 0);
    }
}

void printProgress(size_t prg, size_t sz)
{
    int progress = ((prg * 100) / content_len);

    if (progress % 10 == 0 && progress != backupProgress)
    {
        backupProgress = progress;
        Serial.printf("Progress: %d%%\n", progress);
    }
}

void notFound(AsyncWebServerRequest *request)
{
    if (request->url().startsWith("/"))
    {
        request->send(SPIFFS, request->url(), String(), true);
    }
    else
    {
        request->send(404);
    }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    // filelist = "";
    int i = 0;
    String partlist;
    // Serial.printf("Listing directory: %s\r\n", dirname);
    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {

        if (file.isDirectory())
        {
            //  Serial.print("  DIR : ");
            // Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.name(), levels - 1);
            }
        }
        else
        {

            i++;
            String st_after_symb = String(file.name()).substring(String(file.name()).indexOf("/") + 1);

            partlist += String("<tr><td>") + String(i) + String("</td><td>") + String("<a href='") + String(file.name()) + String("'>") + st_after_symb + String("</td><td>") + String(file.size() / 1024) + String("</td><td>") + String("<input type='button' class='btndel' onclick=\"deletef('") + String(file.name()) + String("')\" value='X'>") + String("</td></tr>");
            filelist = String("<table><tbody><tr><th>#</th><th>File name</th><th>Size(KB)</th><th></th></tr>") + partlist + String(" </tbody></table>");
        }
        file = root.openNextFile();
    }
    filelist = String("<table><tbody><tr><th>#</th><th>File name</th><th>Size(KB)</th><th></th></tr>") + partlist + String(" </tbody></table>");
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
    String status = settings.DeviceName + ";" + settings.wifissid + ";;" + String(settings.RelayCompressorPin) + ";" + String(settings.RelayFreePin) + ";" + String(settings.SensorPin) + ";" + String(settings.SimulationMode) + ";" + String(settings.MaxSecForStatus) + ";" + String(settings.SensorValueAt0PSI, 3) + ";" + String(settings.SensorValueAt50PSI, 3) + ";" + settings.BTPinCode + ";" + String(settings.MessureWait) + ";" + String(settings.Precision) + ";" + String(settings.PressureCalcMethod) + ";" + String(settings.BLESecurityEnabled ? "1" : "0") + ";" + VERSION_SHORT + ";" + String(settings.MaxPressurePSI, 1) + ";" + String(settings.MaxValveOpenSec) + ";" + String(settings.SensorFullScalePSI, 1) + ";" + settings.AccessPointPassword;
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
    XML += "<BTPinCode>" + settings.BTPinCode + "</BTPinCode>";
    XML += "<BLESecurityEnabled>" + String(settings.BLESecurityEnabled ? "1" : "0") + "</BLESecurityEnabled>";
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
    JSON += "'BTPinCode': '" + settings.BTPinCode + "',";
    JSON += "'BLESecurityEnabled': '" + String(settings.BLESecurityEnabled ? "1" : "0") + "',";
    JSON += "'CurrentPressure': '" + String(currentPressure) + "',";
    JSON += "'Software_VER': '" + String(VERSION_SHORT) + "',";
    JSON += "'IP': '" + currentIP + "',";    
    JSON += "'SimulationMode': '" + String(isSimulationMode) + "',";
    JSON += "'PressureCalcMethod': '" + String(settings.PressureCalcMethod) + "'";
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

    for (int i = 0; i < paramsNr; i++)
    {
        AsyncWebParameter *p = request->getParam(i);
        String paramName = p->name();
        if (paramName == "DeviceName")
            settings.DeviceName = p->value();
        else if (paramName == "wifissid")
            settings.wifissid = p->value();
        else if (paramName == "wifipassword")
        {
            if (p->value().length() > 0)
                settings.wifipassword = p->value();
        }
        else if (paramName == "RelayCompressorPin")
            settings.RelayCompressorPin = p->value().toInt();
        else if (paramName == "RelayFreePin")
            settings.RelayFreePin = p->value().toInt();
        else if (paramName == "SensorPin")
            settings.SensorPin = p->value().toInt();
        else if (paramName == "MaxSecForStatus")
            settings.MaxSecForStatus = p->value().toInt();
        else if (paramName == "SensorValueAt0PSI")
            settings.SensorValueAt0PSI = p->value().toFloat();
        else if (paramName == "SensorValueAt50PSI")
            settings.SensorValueAt50PSI = p->value().toFloat();
        else if (paramName == "BTPinCode")
            settings.BTPinCode = p->value();
        else if (paramName == "MessureWait")
            settings.MessureWait = p->value().toDouble();
        else if (paramName == "Precision")
            settings.Precision = p->value().toDouble();
        else if (paramName == "SimulationMode")
        {
            String sSimulationMode = p->value();
            settings.SimulationMode = sSimulationMode.equalsIgnoreCase("true");
            isSimulationMode = settings.SimulationMode;
            LOG_INFO("WEB", "Simulation mode: " + String(isSimulationMode ? "enabled" : "disabled"));
            
            // Reset initial pressure if simulation mode is enabled
            if (isSimulationMode) {
                currentPressure = 0;
            }
        }
        else if (paramName == "PressureCalcMethod")
        {
            settings.PressureCalcMethod = p->value().toInt();
            LOG_INFO("WEB", "Pressure calculation method set to: " + String(settings.PressureCalcMethod));
        }
        else if (paramName == "BLESecurityEnabled")
        {
            String sBLESecurity = p->value();
            settings.BLESecurityEnabled = sBLESecurity.equalsIgnoreCase("true") || sBLESecurity == "1";
            LOG_INFO("WEB", "BLE Security: " + String(settings.BLESecurityEnabled ? "enabled" : "disabled"));
        }
        else if (paramName == "MaxPressurePSI")
        {
            float limit = p->value().toFloat();
            if (limit > 0)
                settings.MaxPressurePSI = limit;
        }
        else if (paramName == "MaxValveOpenSec")
        {
            int seconds = p->value().toInt();
            if (seconds >= 10)
                settings.MaxValveOpenSec = seconds;
        }
        else if (paramName == "SensorFullScalePSI")
        {
            float fullScale = p->value().toFloat();
            if (fullScale >= 0)
                settings.SensorFullScalePSI = fullScale;
        }
        else if (paramName == "AccessPointPassword")
        {
            String apPassword = p->value();
            if (apPassword.length() >= 8 && apPassword.indexOf(';') < 0)
                settings.AccessPointPassword = apPassword;
        }
    }


    saveSettingFile();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Ok");
    response->addHeader("Refresh", "30");
    response->addHeader("Location", "/");
    request->send(response);

    Serial.println("Update complete");
    Serial.flush();
    
    // שימוש בפונקציה החדשה לתזמון אתחול מחדש
    scheduleRestart(1000);
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
              { request->send(SPIFFS, "/fs.html", String(), false, processor_update); });

    server.on("/fs.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/fs.css", "text/css"); });

    server.on("/fs.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/fs.js", "text/javascript"); });

    server.on("/filelist", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/plain", filelist.c_str()); });

    server.on("/configurations", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/configurations.html", String(), false, getSettingsParams); });

    server.on("/configurations.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/configurations.css", "text/css"); });

    server.on("/configurations.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/configurations.js", "text/javascript"); });

    server.on("/updateSettings", HTTP_GET, [](AsyncWebServerRequest *request)
              {   
    request->send(200, "text/plain", "OK");
    saveSettings(request); });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    request->send(200, "text/html", "<HTML><BODY><script>setTimeout(function() { window.location.href = '/'; }, 2000);</script>Device will reboot in 2 seconds </BODY></HTML>");
    
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
    request->send(200, "text/plain", "OK"); });

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
    int maxRetry = 2;
    int retryCounter = 0;

    WiFi.begin(settings.wifissid.c_str(), settings.wifipassword.c_str());
    while (WiFi.status() != WL_CONNECTED && retryCounter < maxRetry)
    {
        delay(2000);
        Serial.println("Connecting to WiFi..");
        retryCounter++;
    }
    if (WiFi.status() == WL_CONNECTED)
    {

        Serial.println("Connected to the WiFi network");
        // Print local IP address and start web server
        Serial.println("");
        Serial.println("WiFi connected.");
        Serial.println("IP address: ");
        IPAddress IP = WiFi.localIP();
        Serial.println(IP.toString());
        currentIP = IP.toString();
        /*
            tft.setCursor(1, 140);
            tft.println("IP Adresss:");
            tft.setTextColor(ILI9341_GREENYELLOW);
            tft.println(WiFi.localIP());
         */
        wifiInitialized = true;
    }
    else
    {

        WiFi.softAP(settings.DeviceName.c_str(), settings.AccessPointPassword.c_str());
        IPAddress IP = WiFi.softAPIP();
        Serial.println("");
        Serial.println("WiFi not connected!");
        Serial.println("Setup Access point named: " + settings.DeviceName);
        Serial.println("Use the access-point password saved on the configuration page.");
        Serial.println("browse to the following IP: " + IP.toString());
        currentIP = IP.toString();
    }
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

// setup settings file
void loadSettingsfile()
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

    StaticJsonDocument<2048> doc;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.println(F("Failed to read file, using default settings"));
        file.close();
        return;
    }

    // Copy values from the JsonDocument to the Config
    settings.DeviceName = doc["DeviceName"].as<String>();
    settings.wifissid = doc["wifissid"].as<String>();
    settings.wifipassword = doc["wifipassword"].as<String>();
    settings.RelayCompressorPin = doc["RelayCompressorPin"].as<int>();
    settings.RelayFreePin = doc["RelayFreePin"].as<int>();
    settings.SensorPin = doc["SensorPin"].as<int>();
    settings.MaxSecForStatus = doc["MaxSecForStatus"].as<int>();
    settings.SensorValueAt0PSI = doc["SensorValueAt0PSI"].as<float>();
    settings.SensorValueAt50PSI = doc["SensorValueAt50PSI"].as<float>();
    
    // Debug: Print loaded sensor values
    Serial.println("DEBUG: Loaded SensorValueAt0PSI: " + String(settings.SensorValueAt0PSI, 3));
    Serial.println("DEBUG: Loaded SensorValueAt50PSI: " + String(settings.SensorValueAt50PSI, 3));
    
    settings.BTPinCode = doc["BTPinCode"].as<String>();
    settings.MessureWait = doc["MessureWait"].as<double>();
    settings.Precision = doc["Precision"].as<double>();
    
    // Load simulation mode setting if it exists
    if (doc.containsKey("SimulationMode")) {
        settings.SimulationMode = doc["SimulationMode"].as<bool>();
        isSimulationMode = settings.SimulationMode;
        LOG_INFO("SETTINGS", "Loaded simulation mode: " + String(isSimulationMode ? "enabled" : "disabled"));
    }
    
    // Load PressureCalcMethod if it exists
    if (doc.containsKey("PressureCalcMethod")) {
        settings.PressureCalcMethod = doc["PressureCalcMethod"].as<int>();
        LOG_INFO("SETTINGS", "Loaded PressureCalcMethod: " + String(settings.PressureCalcMethod));
    }
    
    if (doc.containsKey("BLESecurityEnabled")) {
        settings.BLESecurityEnabled = doc["BLESecurityEnabled"].as<bool>();
        LOG_INFO("SETTINGS", "Loaded BLESecurityEnabled: " + String(settings.BLESecurityEnabled ? "enabled" : "disabled"));
    }
    if (doc.containsKey("MaxPressurePSI") && doc["MaxPressurePSI"].as<float>() > 0)
        settings.MaxPressurePSI = doc["MaxPressurePSI"].as<float>();
    if (doc.containsKey("MaxValveOpenSec") && doc["MaxValveOpenSec"].as<int>() >= 10)
        settings.MaxValveOpenSec = doc["MaxValveOpenSec"].as<int>();
    if (doc.containsKey("SensorFullScalePSI"))
        settings.SensorFullScalePSI = doc["SensorFullScalePSI"].as<float>();
    if (doc.containsKey("AccessPointPassword")) {
        String apPassword = doc["AccessPointPassword"].as<String>();
        if (apPassword.length() >= 8)
            settings.AccessPointPassword = apPassword;
    }
    bool sensorPinMigrated = false;
    if (settings.SensorPin == 27) {
        settings.SensorPin = 34;
        sensorPinMigrated = true;
        LOG_WARNING("SETTINGS", "Sensor pin 27 is ADC2. Using GPIO34 instead.");
    }
    
    // Close the file (Curiously, File's destructor doesn't close the file)
    file.close();
    if (sensorPinMigrated)
        saveSettingFile();
}

void saveSettingFile()
{
    Serial.println("saveSettingFile");
    DynamicJsonDocument doc(2048);

    doc["DeviceName"] = settings.DeviceName;
    doc["wifissid"] = settings.wifissid;
    doc["wifipassword"] = settings.wifipassword;
    doc["RelayCompressorPin"] = settings.RelayCompressorPin;
    doc["RelayFreePin"] = settings.RelayFreePin;
    doc["SensorPin"] = settings.SensorPin;
    doc["MaxSecForStatus"] = settings.MaxSecForStatus;
    doc["SensorValueAt0PSI"] = settings.SensorValueAt0PSI;
    doc["SensorValueAt50PSI"] = settings.SensorValueAt50PSI;
    doc["MessureWait"] = settings.MessureWait;
    doc["Precision"] = settings.Precision;    
    doc["BTPinCode"] = settings.BTPinCode;
    doc["SimulationMode"] = settings.SimulationMode;
    doc["PressureCalcMethod"] = settings.PressureCalcMethod;
    doc["BLESecurityEnabled"] = settings.BLESecurityEnabled;
    doc["MaxPressurePSI"] = settings.MaxPressurePSI;
    doc["MaxValveOpenSec"] = settings.MaxValveOpenSec;
    doc["SensorFullScalePSI"] = settings.SensorFullScalePSI;
    doc["AccessPointPassword"] = settings.AccessPointPassword;

    if (SPIFFS.exists("/settings.json.bak"))
        SPIFFS.remove("/settings.json.bak");
    SPIFFS.rename("/settings.json", "/settings.json.bak");

    File settingsFile = SPIFFS.open("/settings.json", FILE_WRITE);

    if (!settingsFile)
    {
        Serial.println("- failed to open settings file for writing");
        return;
    }
    serializeJson(doc, settingsFile);
    settingsFile.close();
    Serial.println("finish to save settings file");
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

    if (settings.BLESecurityEnabled) {
        LOG_INFO("BLE", "BLE Security ENABLED - configuring passkey pairing");
        BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_MITM);

        uint32_t passkey = (uint32_t)settings.BTPinCode.toInt();
        if (passkey > 999999) passkey = 123456;
        esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
        LOG_INFO("BLE", "Static passkey set to: " + String(passkey));

        BLESecurity *pSecurity = new BLESecurity();
        pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
        pSecurity->setCapability(ESP_IO_CAP_OUT);
        pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    } else {
        LOG_INFO("BLE", "BLE Security DISABLED - open connections allowed");
    }

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

    if (settings.BLESecurityEnabled) {
        pCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM);
        pCharacteristic_2->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM);
        LOG_DEBUG("BLE", "Characteristic access permissions set to require encrypted connection");
    }

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
    client.setInsecure(); // skip verification
    
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



void BLESync()
{
    LOG_VERBOSE("BLE", "BLESync started");
    
    if (deviceConnected) {
        LOG_VERBOSE("BLE", "Device is connected");
        
        if (firstTimeConnceted) {
            LOG_INFO("BLE", "First time connection - sending settings");
            firstTimeConnceted = false;
            
            String settings = getSettingsJSON();
            LOG_DEBUG("BLE", "Settings to send: " + settings);
            
            pCharacteristic->setValue(settings.c_str());
            pCharacteristic->notify();
            LOG_INFO("BLE", "Initial settings sent");
        }

        if (BLEBufferValue.length() > 0) {
            LOG_DEBUG("BLE", "Sending status update: " + BLEBufferValue);
            pCharacteristic->setValue(BLEBufferValue.c_str());
            pCharacteristic->notify();
            
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
        //LOG_VERBOSE("SYSTEM", "Upgrade in progress, skipping loop");
        return;
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
    if (settings.SensorFullScalePSI > 0 && pressure > settings.SensorFullScalePSI * 1.05f) {
        pressureValid = false;
        return pressure;
    }
    pressureValid = true;
    return pressure;
}

float calculatePressureMethod1(int sensorVal) {
    lastRawADC = sensorVal;
    lastVoltage = (float)sensorVal * (3.3f / 4095.0f);

    float spanPsi = settings.SensorFullScalePSI;
    float range = settings.SensorValueAt50PSI - settings.SensorValueAt0PSI;
    if (spanPsi <= 0.0f || range == 0.0f) {
        pressureValid = false;
        return 0;
    }
    if (sensorVal + 150 < settings.SensorValueAt0PSI) {
        pressureValid = false;
        return 0;
    }

    float pressure = (sensorVal - settings.SensorValueAt0PSI) / (range / spanPsi);
    return applyPressureLimits(pressure);
}

float calculatePressureMethod2(int sensorVal) {
    lastRawADC = sensorVal;
    float voltageAtADC = (float)sensorVal * (3.3f / 4095.0f);
    lastVoltage = voltageAtADC;
    float sensorVoltage = voltageAtADC * 2.0f;

    float spanPsi = settings.SensorFullScalePSI;
    float v0 = settings.SensorValueAt0PSI;
    float vFull = settings.SensorValueAt50PSI;
    if (spanPsi <= 0.0f || vFull == v0 || v0 < 0.0f || vFull < 0.0f || v0 > 3.3f || vFull > 3.3f) {
        pressureValid = false;
        return 0;
    }
    if (voltageAtADC < 0.05f) {
        pressureValid = false;
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
        samples[i] = analogRead(settings.SensorPin);
        delay(25); // Small delay between readings
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

    if (isSimulationMode) {
        currentPressure = readPressureFromSensor();
        LOG_INFO("PRESSURE", "Current pressure: " + String(currentPressure) + " PSI");
        SendStatusToMobile();
        return;
    }

    unsigned long epoch = valveCommandEpoch;
    if (valveCommandEpoch != epoch)
        return;
    StopAir();
    if (valveCommandEpoch != epoch)
        return;
    waitForPressureToSettle();

    const int maxStableReads = 5;
    currentPressure = readPressureFromSensor();
    if (valveCommandEpoch != epoch)
        return;
    if (!pressureValid) {
        if (valveSessionActive)
            refuseAir("pressure reading is not usable");
        else
            StopAir();
        return;
    }

    bool stable = false;
    for (int attempt = 0; attempt < maxStableReads; attempt++) {
        waitForPressureToSettle();
        float nextPressure = readPressureFromSensor();
        if (valveCommandEpoch != epoch)
            return;
        if (!pressureValid) {
            if (valveSessionActive)
                refuseAir("pressure reading is not usable");
            else
                StopAir();
            return;
        }
        if (fabs(nextPressure - currentPressure) <= kStablePsi) {
            currentPressure = nextPressure;
            stable = true;
            break;
        }
        currentPressure = nextPressure;
    }

    if (valveCommandEpoch != epoch)
        return;
    if (!stable) {
        if (valveSessionActive)
            refuseAir("pressure reading is unstable");
        else
            StopAir();
        return;
    }

    if (!valveSessionActive)
        StopAir();
    sessionHasReading = true;
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
    if (isSimulationMode)
        digitalWrite(settings.RelayFreePin, LOW);
    else
        digitalWrite(settings.RelayFreePin, HIGH);
}

void InflateAir()
{
    LOG_INFO("AIR", "Inflating air");
    operationMode = Inflate;
    digitalWrite(settings.RelayFreePin, LOW);
    if (isSimulationMode)
        digitalWrite(settings.RelayCompressorPin, LOW);
    else
        digitalWrite(settings.RelayCompressorPin, HIGH);
}

void OperateAir()
{
    LOG_DEBUG("AIR", "Operating air system - Target: " + String(targetPressure) + " PSI, Current: " + String(currentPressure) + " PSI");

    if (!isSimulationMode && settings.SensorFullScalePSI <= 0) {
        refuseAir("sensor full scale is not set");
        return;
    }
    if (!isSimulationMode && valveSessionActive && !sessionHasReading)
        return;
    if (!isSimulationMode && !pressureValid) {
        refuseAir("pressure reading is not usable");
        return;
    }
    if (valveSessionActive && settings.MaxValveOpenSec > 0 &&
        millis() - valveSessionStartMs >= (unsigned long)settings.MaxValveOpenSec * 1000UL) {
        refuseAir("valve open time exceeded");
        return;
    }
    if (valveSessionActive && pressureValid) {
        if (sessionBaselinePressure < 0) {
            sessionBaselinePressure = currentPressure;
            sessionProgressMs = millis();
        } else if (millis() - sessionProgressMs >= kNoProgressMs) {
            float moved = currentPressure - sessionBaselinePressure;
            bool wantsHigher = targetPressure > sessionBaselinePressure + settings.Precision;
            bool wantsLower = targetPressure < sessionBaselinePressure - settings.Precision;
            if ((wantsHigher && moved < kMinProgressPsi) || (wantsLower && moved > -kMinProgressPsi)) {
                refuseAir("pressure is not changing");
                return;
            }
            sessionBaselinePressure = currentPressure;
            sessionProgressMs = millis();
        }
    }
    if (!isSimulationMode && currentPressure > settings.MaxPressurePSI) {
        if (targetPressure + settings.Precision < currentPressure) {
            LOG_INFO("AIR", "Pressure above maximum, deflating");
            DeflateAir();
            return;
        }
        refuseAir("pressure is above the maximum");
        return;
    }

    if ((targetPressure - currentPressure) > settings.Precision) {
        LOG_INFO("AIR", "Pressure too low, inflating");
        InflateAir();
    } else if ((currentPressure - targetPressure) > settings.Precision) {
        LOG_INFO("AIR", "Pressure too high, deflating");
        DeflateAir();
    } else if (operationMode != Stop) {
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

    LOG_INFO("BLE", "JSON from app: " + sMessage);
    
    if (!doc.containsKey("command")) {
        LOG_ERROR("BLE", "Missing 'command' field in message");
        return;
    }
    
    int cmd = doc["command"].as<int>();
    LOG_INFO("BLE", "Command received: " + String(cmd));
    
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
}

void handleStartCommand(DynamicJsonDocument &doc) {
    if (!doc.containsKey("targetPressure")) {
        LOG_ERROR("BLE", "Missing 'targetPressure' field for start command");
        return;
    }
    float requested = doc["targetPressure"].as<float>();
    if (!isSimulationMode && settings.SensorFullScalePSI <= 0) {
        refuseAir("sensor full scale is not set");
        return;
    }
    float limit = settings.MaxPressurePSI;
    if (!isSimulationMode && settings.SensorFullScalePSI > 0 && settings.SensorFullScalePSI < limit)
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
    LOG_INFO("BLE", "Start command received with target pressure: " + String(targetPressure));
    if (isSimulationMode) {
        sessionHasReading = true;
        OperateAir();
    } else {
        sessionHasReading = false;
        measureNow = true;
    }
}

void handleSettingsCommand(DynamicJsonDocument &doc) {
    LOG_INFO("BLE", "Settings update command received");
    settings.DeviceName = doc["DeviceName"].as<String>();
    settings.wifissid = doc["wifissid"].as<String>();
    settings.wifipassword = doc["wifipassword"].as<String>();
    settings.RelayCompressorPin = doc["RelayCompressorPin"].as<int>();
    settings.RelayFreePin = doc["RelayFreePin"].as<int>();
    settings.SensorPin = doc["SensorPin"].as<int>();
    settings.MaxSecForStatus = doc["MaxSecForStatus"].as<int>();
    settings.SensorValueAt0PSI = doc["SensorValueAt0PSI"].as<float>();
    settings.SensorValueAt50PSI = doc["SensorValueAt50PSI"].as<float>();
    
    // Debug: Print the received sensor values
    Serial.println("DEBUG: Received SensorValueAt0PSI: " + String(settings.SensorValueAt0PSI, 3));
    Serial.println("DEBUG: Received SensorValueAt50PSI: " + String(settings.SensorValueAt50PSI, 3));
    settings.MessureWait = doc["MessureWait"].as<double>();
    settings.Precision = doc["Precision"].as<double>();                    
    settings.BTPinCode = doc["BTPinCode"] | "1234";
    if (doc.containsKey("PressureCalcMethod"))
        settings.PressureCalcMethod = doc["PressureCalcMethod"].as<int>();
    
    if (doc.containsKey("BLESecurityEnabled"))
        settings.BLESecurityEnabled = doc["BLESecurityEnabled"].as<bool>();
    
    // Check if SimulationMode is included in the settings update
    if (doc.containsKey("SimulationMode")) {
        settings.SimulationMode = doc["SimulationMode"].as<bool>();
        isSimulationMode = settings.SimulationMode;
        LOG_INFO("BLE", "Simulation mode updated through settings: " + String(isSimulationMode ? "enabled" : "disabled"));
        
        // Reset initial pressure if simulation mode is enabled
        if (isSimulationMode) {
            currentPressure = 0;
        }
    }
    
    LOG_INFO("BLE", "Saving settings and restarting");
    saveSettingFile();
    ESP.restart();
}

void handleSimulationCommand(DynamicJsonDocument &doc) {
    if (!doc.containsKey("enabled")) {
        LOG_ERROR("BLE", "Missing 'enabled' field for simulation command");
        return;
    }
    isSimulationMode = doc["enabled"].as<bool>();
    settings.SimulationMode = isSimulationMode;  // Update the settings struct
    LOG_INFO("BLE", "Simulation mode: " + String(isSimulationMode ? "enabled" : "disabled"));
    
    // Save the setting to file
    LOG_INFO("BLE", "Saving simulation mode setting to file");
    saveSettingFile();
    
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