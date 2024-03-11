#include <Arduino.h>
#include "BluetoothSerial.h"
// #include <SPI.h>
// #include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WebServer.h>
// #include <ESPmDNS.h>
#include <Update.h>
#include "main.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include "settings.h"

int backupProgress = 0;
bool wifiInitialized = false;
int tikingCounter = 0;

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

// AsyncWebServer webServer(80);
// size_t content_len;
WiFiClientSecure client;
#pragma endregion webServer declaration

#pragma region WiFi declaration
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3 * 60 * 60);
#pragma endregion WiFi declaration

// Consts Declartions
#define USE_PIN           // Uncomment this to use PIN during pairing. The pin is specified on the line below
const char *pin = "1234"; // Change this to more secure PIN.
const int RELAY_COMPRESSOR_PIN = 16;
const int RELAY_FREE_PIN = 17;
const int SENSOR_PIN = 32;

const int AIR_PRESSURE_SENSOR_MAX_MPA = 0.5;
double pressureCurrectionFactor = 1;
double voltCurrection = 0.47;
int maxSecForStatus = 10;
//String device_name = "ESP32-BT-AirDownUp-Controller";

// Global variables
double currentPressure = 0;
double targetPressure = 0;
OperationMode operationMode = Stop;

DeviceSettings settings; // <- global settingsuration object

// Debug
bool simulationMode = true;
int iSimulationCounter = 0;

#if defined(CONFIG_CLASSIC_BT_ENABLED)
#define BT_MODE ESP_BT_MODE_CLASSIC_BT
#elif defined(CONFIG_BT_BLE_ENABLED)
#define BT_MODE ESP_BT_MODE_BLE
#else
#define BT_MODE ESP_BT_MODE_BTDM
#endif

// #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
// #error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
// #endif

// #if !defined(CONFIG_BT_SPP_ENABLED)
// #error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
// #endif

BluetoothSerial SerialBT;

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

    Serial.printf("Progress: %d%%\n", progress);

    if (progress % 20 == 0 && progress != backupProgress)
    {
        backupProgress = progress;
        /*
            tft.fillScreen(ILI9341_BLACK);
            tft.setCursor(5, 50);
            tft.setTextSize(1);
            tft.setTextColor(ILI9341_GREEN);
            tft.setFont(&FreeMonoBoldOblique12pt7b);
            tft.println("Firmware Update");
            tft.setTextSize(1);
            tft.setTextColor(ILI9341_YELLOW);
            tft.println("");
            tft.printf("Progress: %d%%\n", progress);
            //tft.drawBitmap(0, 0, canvas.getBuffer(), canvas.width(), canvas.height(), ILI9341_YELLOW, ILI9341_BLACK);
        */
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

    String status = {settings.DeviceName + ";" + settings.wifissid + ";" + settings.wifipassword + ";" + settings.RelayCompressorPin + ";" + settings.RelayFreePin + ";" + settings.SensorPin + ";" + settings.AutoStart + ";" + settings.MaxSecForStatus + ";" + settings.SensorValueAt0PSI + ";" + settings.SensorValueAt50PSI + ";" + settings.BTPinCode + ";" + settings.MessureWait + ";" + Software_VER};
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
    XML += "<AutoStart>" + String(settings.AutoStart) + "</AutoStart>";
    XML += "<MaxSecForStatus>" + String(settings.MaxSecForStatus) + "</MaxSecForStatus>";
    XML += "<SensorValueAt0PSI>" + String(settings.SensorValueAt0PSI) + "</SensorValueAt0PSI>";
    XML += "<SensorValueAt50PSI>" + String(settings.SensorValueAt50PSI) + "</SensorValueAt50PSI>";
    XML += "<MessureWait>" + String(settings.MessureWait) + "</MessureWait>";
    XML += "<BTPinCode>" + settings.BTPinCode + "</BTPinCode>";
    XML += "<CurrentPressure>" + String(currentPressure) + "</CurrentPressure>";
    XML += "<Software_VER>" + String(Software_VER) + "</Software_VER>";
    XML += "</settings>";

    Serial.println("getSettingsXML: " + XML);
    return XML;
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
            settings.wifipassword = p->value();
        else if (paramName == "RelayCompressorPin")
            settings.RelayCompressorPin = p->value().toInt();
        else if (paramName == "RelayFreePin")
            settings.RelayFreePin = p->value().toInt();
        else if (paramName == "SensorPin")
            settings.SensorPin = p->value().toInt();
        else if (paramName == "AutoStart")
        {
            String sAutoStart = p->value();
            settings.AutoStart = sAutoStart.equalsIgnoreCase("true");
        }
        else if (paramName == "MaxSecForStatus")
            settings.MaxSecForStatus = p->value().toInt();
        else if (paramName == "SensorValueAt0PSI")
            settings.SensorValueAt0PSI = p->value().toInt();
        else if (paramName == "SensorValueAt50PSI")
            settings.SensorValueAt50PSI = p->value().toInt();
        else if (paramName == "BTPinCode")
            settings.BTPinCode = p->value().toInt();
        else if (paramName == "MessureWait")
            settings.MessureWait = p->value().toDouble();
    }

    Serial.print("settings.DeviceName=");
    Serial.println(settings.DeviceName);

    saveSettingFile();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Ok");
    response->addHeader("Refresh", "30");
    response->addHeader("Location", "/");
    request->send(response);

    Serial.println("Update complete");
    Serial.flush();
    ESP.restart();
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
    ESP.restart(); });

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
    int maxRetry = 8;
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
        Serial.println(WiFi.localIP());
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

        WiFi.softAP(settings.DeviceName.c_str(), softAPpassword);
        IPAddress IP = WiFi.softAPIP();
        Serial.println("");
        Serial.println("WiFi not connected!");
        Serial.println("Setup Access point named: " + settings.DeviceName);
        Serial.println("password for access point: " + String(softAPpassword));
        Serial.println("browse to the following IP: " + IP.toString());
        /*
            tft.fillScreen(ILI9341_BLACK);
            tft.setCursor(5, 10);

            tft.setTextColor(ILI9341_GREEN);
            tft.setFont(&FreeMonoBold12pt7b);
            tft.setTextSize(1);
            tft.println("Setup WiFi");
            tft.setFont(&FreeMono9pt7b);
            tft.println("");

            tft.setTextColor(ILI9341_WHITE);
            tft.setFont(&FreeMono9pt7b);
            tft.setTextSize(1);
            tft.println("No Wifi COnnection, please connect to the following Access Point:");

            tft.setTextColor(ILI9341_GREENYELLOW);
            tft.setTextSize(1);
            tft.setFont(&FreeMonoBold12pt7b);
            tft.println("   " + settings.DeviceName);

            tft.setTextColor(ILI9341_WHITE);
            tft.setFont(&FreeMono9pt7b);
            tft.setTextSize(1);
            tft.println("the WiFi Password is: ");

            tft.setTextColor(ILI9341_GREENYELLOW);
            tft.setTextSize(1);
            tft.setFont(&FreeMonoBold12pt7b);
            tft.println("      " + String(softAPpassword));

            tft.setTextColor(ILI9341_WHITE);
            tft.setFont(&FreeMono9pt7b);
            tft.setTextSize(1);
            tft.println("after connecting to WiFi, browse to the following URL:");

            tft.setTextColor(ILI9341_GREENYELLOW);
            tft.setFont(&FreeMonoBold12pt7b);
            tft.setTextSize(1);
            tft.println("     " + IP.toString());
         */
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

    StaticJsonDocument<512> doc;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error)
        Serial.println(F("Failed to read file, using default settingsuration"));

    // Copy values from the JsonDocument to the Config
    settings.DeviceName = doc["DeviceName"].as<String>();
    settings.wifissid = doc["wifissid"].as<String>();
    settings.wifipassword = doc["wifipassword"].as<String>();
    settings.RelayCompressorPin = doc["RelayCompressorPin"].as<int>();
    settings.RelayFreePin = doc["RelayFreePin"].as<int>();
    settings.SensorPin = doc["SensorPin"].as<int>();
    settings.AutoStart = (bool)doc["AutoStart"];
    settings.SensorValueAt0PSI = doc["SensorValueAt0PSI"].as<int>();
    settings.SensorValueAt50PSI = doc["SensorValueAt50PSI"].as<int>();
    settings.MessureWait = (double)(doc["MessureWait"].isNull() ? 2 : doc["MessureWait"]);
    settings.BTPinCode = doc["BTPinCode"] | "1234";
    settings.MessureWait = doc["MessureWait"].as<int>() | 2;

    

    // Close the file (Curiously, File's destructor doesn't close the file)
    file.close();
}

void saveSettingFile()
{
    Serial.println("saveSettingFile");
    DynamicJsonDocument doc(1024);

    doc["DeviceName"] = settings.DeviceName;
    doc["wifissid"] = settings.wifissid;
    doc["wifipassword"] = settings.wifipassword;
    doc["RelayCompressorPin"] = settings.RelayCompressorPin;
    doc["RelayFreePin"] = settings.RelayFreePin;
    doc["SensorPin"] = settings.SensorPin;
    doc["AutoStart"] = settings.AutoStart;
    doc["MaxSecForStatus"] = settings.MaxSecForStatus;
    doc["SensorValueAt0PSI"] = settings.SensorValueAt0PSI;
    doc["SensorValueAt50PSI"] = settings.SensorValueAt50PSI;
    doc["MessureWait"] = settings.MessureWait;
    doc["BTPinCode"] = settings.BTPinCode;

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

    Serial.println("File Content:");
    while (file.available())
    {
        Serial.write(file.read());
    }
    file.close();
}
void setupBlueTooth()
{
    // BT sets
    Serial.printf("The device with name \"%s\" is started.\nNow you can pair it with Bluetooth!\n", settings.DeviceName.c_str());
    if (settings.BTPinCode.length()>0)
    {
        //SerialBT.setPin(settings.BTPinCode.c_str());
        Serial.println("Using PIN");
    }
    SerialBT.begin(settings.DeviceName); // Bluetooth device name
    SendSettingsToMobile();
}
//========================
#pragma endregion settings

void setup()
{

    Serial.begin(115200);
    printSettingsFile();
    loadSettingsfile();
    setupWiFi();
    setupWebServer();
    client.setInsecure(); // skip verification
    Serial.print("setup() running on core ");
    Serial.println(xPortGetCoreID());
    setupBlueTooth();
    
    // initiate relays Legs
    pinMode(settings.RelayCompressorPin, OUTPUT);
    pinMode(settings.RelayFreePin, OUTPUT);
    pinMode(settings.SensorPin, INPUT);

    // clear memory
    targetPressure = 0;
    currentPressure = 0;

    Serial.println("finish setup");
}

void loop()
{
    //Serial.print("loop: ");
    // skip x tiks to avoid reading presure every loop
    if (tikingCounter == 0)
        readCurrentPressure();

    tikingCounter++;

    SyncWithBlueToothDevice();

    if (operationMode != Stop)
        OperateAir();
    else
        StopAir();

    delay(1000);

    if (tikingCounter > calculateDelatFactor())
        tikingCounter = 0;

    //Serial.print(" tikingCounter =: ");
    //Serial.println(tikingCounter);
}

float readPressureFromSensor()
{
    Serial.print("readPressureFromSensor Started ");

    int sensorVal = analogRead(settings.SensorPin); // 0 to 4095

    Serial.println("");
    Serial.print("====================== Sensor Value: ");
    Serial.print(sensorVal);
    Serial.println(" ========================");

    float range = settings.SensorValueAt50PSI - settings.SensorValueAt0PSI; // 2894-1345
    float onePSI = range / 50;                                               // 49
    Serial.println("onePSI: " + String(onePSI));
    float zeroPSI = settings.SensorValueAt0PSI; // 400
    float pressure_pascal = (sensorVal - zeroPSI) / onePSI;
    // float zeroPSI =  settings.SensorValueAt0PSI - (onePSI*10); //400
    // float pressure_pascal =settings.SensorCorrectionFactor* (sensorVal - zeroPSI)/onePSI;
    float pressure_bar = pressure_pascal / 14.7;

    Serial.print(" PSI = ");
    Serial.print(pressure_pascal);
    Serial.print(" BAR = ");
    Serial.println(pressure_bar);

    return pressure_pascal;
}

void readCurrentPressure()
{
    Serial.print("readCurrentPressure Started ");

    // in order to check Pressure need to close valve first
    OperationMode backupCurrentValveState = operationMode;

    StopAir();
    // wait for stabilze pressure
    delay(1000 * settings.MessureWait);
    currentPressure = readPressureFromSensor();

    while (abs(readPressureFromSensor() - currentPressure) > 1)
    {
        delay(1000 * settings.MessureWait);
        currentPressure = readPressureFromSensor();
    }

    // restore opration mode
    operationMode = backupCurrentValveState;
    if (operationMode == Inflare)
        InflateAir();
    else if (operationMode == Deflate)
        DeflateAir();
    else
        StopAir();

    Serial.print(" current Pressure: ");
    Serial.println(currentPressure);
}

void StopAir()
{
    //Serial.print(" StopAir Started ");
    operationMode = Stop;
    digitalWrite(settings.RelayCompressorPin, HIGH);
    digitalWrite(settings.RelayFreePin, HIGH);
}

void DeflateAir()
{
    Serial.print(" DeflateAir Started ");
    operationMode = Deflate;
    digitalWrite(settings.RelayCompressorPin, HIGH);
    digitalWrite(settings.RelayFreePin, LOW);
}

void InflateAir()
{
    Serial.print(" InflateAir Started ");
    operationMode = Inflare;
    digitalWrite(settings.RelayCompressorPin, LOW);
    digitalWrite(settings.RelayFreePin, HIGH);
}

void OperateAir()
{
    Serial.print("OperateAir ");
    Serial.print(" Target Pressure: ");
    Serial.print(targetPressure);
    Serial.print(" Current Pressure: ");
    Serial.print(currentPressure);

    if (targetPressure > currentPressure)
        InflateAir();
    else if (targetPressure < currentPressure)
        DeflateAir();
    else
        StopAir();

    Serial.println(" OperateAir finished");
}

void SendStatusToMobile()
{

    if (currentPressure > 0)

        Serial.print(" SendStatusToMobile started ");
    {
        double toSend = currentPressure;
        // if need  to send also stop mode will add - to the value (signal)
        if (operationMode == Stop)
            toSend = toSend * -1;

        //SerialBT.print(toSend, 1);

        Serial.print("SendStatusToMobile: ");
        Serial.print(toSend);
        Serial.println(" SendStatusToMobile finished");
    }
}

void SendSettingsToMobile()
{

    Serial.print(" SendSettingsToMobile started ");
    {
        String  toSend =  getSettingsXML();


        // if need  to send also stop mode will add - to the value (signal)
        SerialBT.println(toSend);

        
        Serial.print(toSend);
        Serial.println(" SendSettingsToMobile dinished");
    }
}


void SyncWithBlueToothDevice()
{

    if (SerialBT.connected())
    {
        // read from application the target Pressure
        if (SerialBT.available())
        {
            Serial.print("SyncWithBlueToothDevice started ");
            String sMessage = SerialBT.readString();
            String sTargetPressure;
            Serial.print("sMessage: ");
            Serial.print(sMessage);
            Serial.print(" first Char: ");
            Serial.print(sMessage.charAt(0));
            switch (sMessage.charAt(0))
            {
                case 'T':
                {
                    sTargetPressure = sMessage.substring(1, sMessage.length());
                    Serial.print(" Target Pressure string: ");
                    Serial.print(sTargetPressure);
                    // if got new target than change setting
                    targetPressure = sTargetPressure.toDouble();
                    Serial.print(" Target Pressure: ");
                    Serial.print(targetPressure);
                    if (targetPressure > 0)
                    {
                        OperateAir();
                    }
                    else
                    {
                        targetPressure = abs(targetPressure);
                        StopAir();
                    }
                    break;
                }
                case '{': // setting starting with wifissid:
                {
                    DynamicJsonDocument doc(1024);
                    deserializeJson(doc, sMessage);

                    Serial.println("lodaing json from app : " + sMessage);

                    settings.DeviceName = doc["DeviceName"].as<String>();
                    settings.wifissid = doc["wifissid"].as<String>();
                    settings.wifipassword = doc["wifipassword"].as<String>();
                    settings.RelayCompressorPin = doc["RelayCompressorPin"].as<int>();
                    settings.RelayFreePin = doc["RelayFreePin"].as<int>();
                    settings.SensorPin = doc["SensorPin"].as<int>();
                    settings.AutoStart = (bool)doc["AutoStart"];
                    settings.SensorValueAt0PSI = doc["SensorValueAt0PSI"].as<int>();
                    settings.SensorValueAt50PSI = doc["SensorValueAt50PSI"].as<int>();
                    settings.MessureWait = (double)(doc["MessureWait"].isNull() ? 2 : doc["MessureWait"]);
                    settings.BTPinCode = doc["BTPinCode"] | "1234";

                    Serial.println("settings.BTPinCode : " + settings.BTPinCode);

                    saveSettingFile();
                    break;
                }
                default:
                {
                    // statements
                    Serial.print("Empty Switch case: ");
                    break;
                }
            }
            Serial.println(" SyncWithBlueToothDevice finished");
        }
        // send current Pressure to application
        //SendStatusToMobile();
        SendSettingsToMobile();
    }
}

double calculateDelatFactor()
{
    // every PSI is 1 5 sec
    // Serial.print("calculateDelatFactor started ");
    // Serial.print("currentPressure: ");
    // Serial.print(currentPressure);
    // Serial.print(" targetPressure: ");
    // Serial.print(targetPressure);

    double factor = abs(targetPressure - currentPressure) * 5;
    // Serial.print(" calculateDelatFactor: ");
    // Serial.print(factor);

    if (factor == 0 || factor < 1)
        factor = 5;

    if (factor > settings.MaxSecForStatus)
        factor = settings.MaxSecForStatus;

    // Serial.print(" calculateDelatFactor: ");
    // Serial.println(factor);

    return factor;
}