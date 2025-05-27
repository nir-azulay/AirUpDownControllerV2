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
    bool AutoStart;
    float SensorValueAt0PSI;
    float SensorValueAt50PSI;
    String BTPinCode;
    double MessureWait;
    double Precision;
    bool SimulationMode;
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
    debug_mode = 7
};

// WiFi Settings
extern const char* softAPpassword;

// Global settings instance
extern DeviceSettings settings;

#endif // SETTINGS_H
