#include <Arduino.h>
#define Software_VER 1.00        // Current version of BG Buddy, displayed at startup

struct DeviceSettings{
    String DeviceName = "esp32-AirUpDownController";
    String wifissid = "";
    String wifipassword =  "12345678";
    int RelayCompressorPin = 25;
    int RelayFreePin = 26;
    int SensorPin = 27;
    int MaxSecForStatus = 5;
    bool AutoStart = false;
    int SensorValueAt0PSI = 428;
    int SensorValueAt50PSI = 3182;    
    String BTPinCode = "1234";    
    double MessureWait = 2;  
};

enum OperationMode {
  Stop = 1,
  Inflare = 2,
  Deflate = 3
};
const char* softAPpassword =  "12345678";
