#include "settings.h"

// Version definition
const float Software_VER = 2.00;

// Global settings instance with default values
DeviceSettings settings = {
    "esp32-AirUpDownController",  // DeviceName
    "",                           // wifissid
    "",                           // wifipassword
    16,                           // RelayCompressorPin (ESP32 Relay X2 relay 1, GPIO16, active high)
    17,                           // RelayFreePin (ESP32 Relay X2 relay 2, GPIO17, active high)
    34,                           // SensorPin (GPIO34, ADC1). Relays stay on 16 and 17.
    5,                            // MaxSecForStatus
    310.227f,                     // SensorValueAt0PSI: ADC counts at 0.25 V (0.5 V through the divider)
    2792.045f,                    // SensorValueAt50PSI: ADC counts at full scale (4.5 V through the divider)
    "",                           // AccessCode
    2.0,                          // MessureWait
    0.5,                          // Precision
    false,                        // SimulationMode
    1,                            // PressureCalcMethod (default to 1)
    50.0f,                        // MaxPressurePSI
    120,                          // MaxValveOpenSec
    50.0f,                        // SensorFullScalePSI
    "12345678"                    // AccessPointPassword
}; 