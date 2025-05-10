#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

#include <Arduino.h>

class PressureSensor {
public:
    PressureSensor(int pin = 34);
    void begin();
    float readFilteredPressure(); // Returns filtered pressure in PSI
    int readRawADC() const;       // Returns last raw ADC value
    float readVoltage() const;    // Returns last calculated real voltage (after divider compensation)

private:
    // Pin
    int sensorPin;

    // Calibration constants
    static constexpr float ADC_REF_VOLTAGE = 3.3;
    static constexpr int ADC_RESOLUTION = 4095;
    static constexpr float CAL_PRESSURE_LOW = 0.0;
    static constexpr float CAL_PRESSURE_HIGH = 50.0;
    static constexpr float SENSOR_MAX_MPA = 0.5;
    static constexpr float SENSOR_MAX_PSI = 72.5;
    static constexpr float SENSOR_MIN_V = 0.5;
    static constexpr float SENSOR_MAX_V = 4.5;
    static constexpr int NUM_SAMPLES = 30;
    static constexpr float ALPHA = 0.3;

    // State
    float filteredPressure;
    int lastRawADC;
    float lastVoltage;

    // Helpers
    float exponentialMovingAverage(float newValue, float oldValue, float alpha);
    float voltageToRealPressure(float voltage);
};

#endif // PRESSURE_SENSOR_H 