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
    412.000f,                     // SensorValueAt0PSI (float)
    3182.000f,                    // SensorValueAt50PSI (float)
    "1234",                       // BTPinCode
    2.0,                          // MessureWait
    0.5,                          // Precision
    false,                        // SimulationMode
    1                              // PressureCalcMethod (default to 1)
}; 