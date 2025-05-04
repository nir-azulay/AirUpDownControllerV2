#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

#include <Arduino.h>

class ExponentialMovingAverage {
private:
    float alpha;
    float currentEMA;
    bool initialized;

public:
    ExponentialMovingAverage(float smoothingFactor = 0.1);
    float update(float newValue);
    float getCurrentEMA() const;
    void reset();
};

class PressureSensor {
private:
    ExponentialMovingAverage ema;
    const int numSamples = 30;
    bool isSimulationMode;
    float currentPressure;
    int operationMode;  // Store as int, convert in implementation

    float readRawPressure();
    float simulatePressure();
    float convertRawToPSI(int rawValue);

public:
    PressureSensor();
    
    // Configuration methods
    void setSimulationMode(bool enabled);
    void setOperationMode(int mode);  // Accept as int, convert in implementation
    
    // Pressure reading methods
    float getCurrentPressure() const;
    float readPressure();
    
    // Utility methods
    void reset();
    float convertToBars(float psi);
};

#endif // PRESSURE_SENSOR_H 