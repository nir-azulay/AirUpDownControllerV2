#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

// Version definition
extern const float Software_VER;

// Device Settings Structure
struct DeviceSettings {
    String DeviceName;
    String wifissid;
    String wifipassword;
    int RelayCompressorPin;
    int RelayFreePin;
    int SensorPin;
    int MaxSecForStatus;
    float SensorValueAt0PSI;
    float SensorValueAt50PSI;
    String AccessCode; // Empty means any phone may control the device. Not sent over BLE.
    double MessureWait;
    double Precision;
    bool SimulationMode;
    int PressureCalcMethod; // 1 or 2: select pressure calculation method
    float MaxPressurePSI; // Configuration page only. Not a BLE field.
    int MaxValveOpenSec; // Configuration page only. Not a BLE field.
    float SensorFullScalePSI; // PSI stamped on the sensor. 0 keeps both valves closed. Not a BLE field.
    String AccessPointPassword; // Used when home Wi-Fi does not connect. Not a BLE field.
};

// Operation Modes
enum OperationMode {
    Stop = 1,
    Inflate = 2,
    Deflate = 3
};

// Commands
enum Commands {
    stop = 1,
    start = 2,
    setting = 3,
    debug_mode = 7,
    authenticate = 8
};

// Global settings instance
extern DeviceSettings settings;

#endif // SETTINGS_H
