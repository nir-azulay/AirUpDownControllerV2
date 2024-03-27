#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

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

// Consts Declartions

// Global variables
double currentPressure = 0;
double targetPressure = 0;
OperationMode operationMode = Stop;

DeviceSettings settings; // <- global settingsuration object


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
      firstTimeConnceted=true;
      //SendSettingsToMobile();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class CharacteristicCallBack: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pChar) override { 
    String pChar2_value_string= "";
    std::string pChar2_value_stdstr =  pChar->getValue();
    pChar2_value_string += String(pChar2_value_stdstr.c_str());
    Serial.println("pChar2: " + pChar2_value_string) + " " + pChar2_value_string.charAt(pChar2_value_string.length()-1); 

    incompleteBLEMessage+=pChar2_value_string;

    if (pChar2_value_string.charAt(pChar2_value_string.length()-1)=='}')
    {
        handleBLEMessage(incompleteBLEMessage);
        incompleteBLEMessage="";
    }
    
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
    String status = {settings.DeviceName + ";" + settings.wifissid + ";" + settings.wifipassword + ";" + settings.RelayCompressorPin + ";" + settings.RelayFreePin + ";" + settings.SensorPin + ";" + settings.AutoStart + ";" + settings.MaxSecForStatus + ";" + settings.SensorValueAt0PSI + ";" + settings.SensorValueAt50PSI + ";" + settings.BTPinCode + ";" + settings.MessureWait + ";" + settings.Precision + ";" +  VERSION_SHORT};
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
    XML += "<Precision>" + String(settings.Precision) + "</Precision>";    
    XML += "<BTPinCode>" + settings.BTPinCode + "</BTPinCode>";
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
    JSON += "}";

    Serial.println("getPressureJSON: " + JSON);
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
    JSON += "'AutoStart': '" + String(settings.AutoStart) + "',";
    JSON += "'MaxSecForStatus': '" + String(settings.MaxSecForStatus) + "',";
    JSON += "'SensorValueAt0PSI': '" + String(settings.SensorValueAt0PSI) + "',";
    JSON += "'SensorValueAt50PSI': '" + String(settings.SensorValueAt50PSI) + "',";
    JSON += "'MessureWait': '" + String(settings.MessureWait) + "',";
    JSON += "'Precision': '" + String(settings.Precision) + "',";    
    JSON += "'BTPinCode': '" + settings.BTPinCode + "',";
    JSON += "'CurrentPressure': '" + String(currentPressure) + "',";
    JSON += "'Software_VER': '" + String(VERSION_SHORT) + "',";
    JSON += "'IP': '" + currentIP + "'";    
    JSON += "}";

    Serial.println("getSettingsJSON: " + JSON);
    return JSON;
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
        else if (paramName == "Precision")
            settings.Precision = p->value().toDouble();
    }


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

        WiFi.softAP(settings.DeviceName.c_str(), softAPpassword);
        IPAddress IP = WiFi.softAPIP();
        Serial.println("");
        Serial.println("WiFi not connected!");
        Serial.println("Setup Access point named: " + settings.DeviceName);
        Serial.println("password for access point: " + String(softAPpassword));
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
    settings.MaxSecForStatus = doc["MaxSecForStatus"].as<int>();
    settings.SensorValueAt0PSI = doc["SensorValueAt0PSI"].as<int>();
    settings.SensorValueAt50PSI = doc["SensorValueAt50PSI"].as<int>();
    settings.BTPinCode = doc["BTPinCode"].as<String>();
    settings.MessureWait = doc["MessureWait"].as<double>();
    settings.Precision = doc["Precision"].as<double>();
    

    
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
    doc["Precision"] = settings.Precision;    
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

    Serial.println("File Content:");
    while (file.available())
    {
        Serial.write(file.read());
    }
    file.close();
}

void setupBlueTooth()
{
     // Create the BLE Device
  BLEDevice::init(settings.DeviceName.c_str());
  BLEDevice::setMTU(517);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  //pServer->updatePeerMTU(pServer->getConnId(),200);
  pServer->setCallbacks(new MyBLEServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHAR1_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );                   

  pCharacteristic_2 = pService->createCharacteristic(
                      CHAR2_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  
                    );  


  // Create a BLE Descriptor
  
  pDescr = new BLEDescriptor((uint16_t)0x2901);
  pDescr->setValue("A very interesting variable");
  pCharacteristic->addDescriptor(pDescr);
  
  pBLE2902 = new BLE2902();
  pBLE2902->setNotifications(true);
  
  
  // Add all Descriptors here
  pCharacteristic->addDescriptor(pBLE2902);
  pCharacteristic_2->addDescriptor(new BLE2902());
  
  // After defining the desriptors, set the callback functions
  pCharacteristic_2->setCallbacks(new CharacteristicCallBack());
  
  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter  
  BLEDevice::startAdvertising();
 
  Serial.println("Waiting a client connection to notify...");

}

#pragma endregion settings

void setup()
{

    Serial.begin(115200);
    printSettingsFile();
    loadSettingsfile();
    setupWiFi();
    setupWebServer();
    client.setInsecure(); // skip verification
    Serial.print("setup() running on core ");Serial.println(xPortGetCoreID());    
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

void BLESync()
{
    if (deviceConnected) {
        if (firstTimeConnceted)
        {
            firstTimeConnceted=false;
            pCharacteristic->setValue(getSettingsJSON().c_str());
            pCharacteristic->notify();

        }

        if (BLEBufferValue.length()>0)
        {
        pCharacteristic->setValue(BLEBufferValue.c_str());
        pCharacteristic->notify();
        BLEBufferValue= "";
        }
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}

void loop()
{
    if (upgradeInProgress) return;
    //Serial.print("loop: ");
    // skip x tiks to avoid reading presure every loop
    if (tikingCounter == 0)
        readCurrentPressure();

    tikingCounter++;

    
    BLESync();

    if (operationMode != Stop)
        OperateAir();
    else
        StopAir();

    delay(1000*settings.MessureWait);

    if (tikingCounter > calculateDelatFactor())
        tikingCounter = 0;

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
    if (pressure_pascal<1) pressure_pascal = 0;
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
    SendStatusToMobile();
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

    if ((targetPressure - currentPressure)>settings.Precision)
        InflateAir();
    else if ((currentPressure - targetPressure )>settings.Precision)
        DeflateAir();
    else
        StopAir();

    Serial.println(" OperateAir finished");
}

void SendStatusToMobile()
{


        Serial.print(" SendStatusToMobile started ");

        BLEBufferValue = getPressureJSON();

        Serial.print("SendStatusToMobile: ");
        Serial.print(BLEBufferValue);
        Serial.println(" SendStatusToMobile finished");
}

void SendSettingsToMobile()
{

    Serial.print(" SendSettingsToMobile started ");
    {
        BLEBufferValue =  getSettingsJSON();

        BLESync();
        
        Serial.print(BLEBufferValue);
        Serial.println(" SendSettingsToMobile finished");
    }
}


void handleBLEMessage(String Message)
{
    Serial.print("handleBLEMessage started ");
    String sMessage = Message;

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, sMessage);

    Serial.println("lodaing json from app : " + sMessage);
    int  cmd = doc["command"].as<int>();
    switch (cmd)
    {
        case 1: //Commands.stop:
            StopAir();            
            break;
        case 2: //Commands.start:
            targetPressure = doc["targetPressure"].as<int>();
            Serial.println("targetPressure : " + String(targetPressure));
            OperateAir();
            break; 
        case 3: //Commands.setting:      
            settings.DeviceName = doc["DeviceName"].as<String>();
            settings.wifissid = doc["wifissid"].as<String>();
            settings.wifipassword = doc["wifipassword"].as<String>();
            settings.RelayCompressorPin = doc["RelayCompressorPin"].as<int>();
            settings.RelayFreePin = doc["RelayFreePin"].as<int>();
            settings.SensorPin = doc["SensorPin"].as<int>();
            settings.AutoStart = (bool)doc["AutoStart"];
            settings.MaxSecForStatus = doc["MaxSecForStatus"].as<int>();
            settings.SensorValueAt0PSI = doc["SensorValueAt0PSI"].as<int>();
            settings.SensorValueAt50PSI = doc["SensorValueAt50PSI"].as<int>();
            settings.MessureWait = doc["MessureWait"].as<double>();
            settings.Precision = doc["Precision"].as<double>();                    
            settings.BTPinCode = doc["BTPinCode"] | "1234";
            saveSettingFile();
            ESP.restart();                                                
            break;
    }
    Serial.println(" handleBLEMessage finished");
}

double calculateDelatFactor()
{
    // every PSI is 1 5 sec

    double factor = abs(targetPressure - currentPressure) * 5;
    // Serial.print(" calculateDelatFactor: ");
    // Serial.print(factor);

    if (factor == 0 || factor < 1)
        factor = 5;

    if (factor > settings.MaxSecForStatus)
        factor = settings.MaxSecForStatus;

    Serial.print(" calculateDelatFactor: ");
    Serial.println(factor);

    return factor;
}