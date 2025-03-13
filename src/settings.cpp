#include "settings.h"

// Version definition
const float Software_VER = 1.04;

// WiFi Settings
const char* softAPpassword = "12345678";

// Global settings instance with default values
DeviceSettings settings = {
    "esp32-AirUpDownController",  // DeviceName
    "",                   // wifissid
    "12345678",                   // wifipassword
    25,                           // RelayCompressorPin
    26,                           // RelayFreePin
    27,                           // SensorPin
    5,                            // MaxSecForStatus
    false,                        // AutoStart
    412,                          // SensorValueAt0PSI
    3182,                         // SensorValueAt50PSI
    "1234",                       // BTPinCode
    2.0,                          // MessureWait
    0.5,                          // Precision
    false                         // SimulationMode
}; 