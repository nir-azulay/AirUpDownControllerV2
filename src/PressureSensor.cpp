/**
 * @file PressureSensor.cpp
 * @brief Implementation of the PressureSensor class for reading and processing pressure values
 * 
 * This file contains the implementation of the PressureSensor class which handles:
 * - Reading pressure values from hardware sensors
 * - Simulating pressure values for testing
 * - Smoothing pressure readings using exponential moving average
 * - Converting between pressure units (PSI, Bar)
 */

#include "PressureSensor.h"
#include "settings.h"

// ExponentialMovingAverage implementation
ExponentialMovingAverage::ExponentialMovingAverage(float smoothingFactor) 
    : alpha(smoothingFactor), currentEMA(0.0f), initialized(false) {}

float ExponentialMovingAverage::update(float newValue) {
    if (!initialized) {
        currentEMA = newValue;
        initialized = true;
    } else {
        currentEMA = alpha * newValue + (1.0f - alpha) * currentEMA;
    }
    return currentEMA;
}

float ExponentialMovingAverage::getCurrentEMA() const {
    return currentEMA;
}

void ExponentialMovingAverage::reset() {
    initialized = false;
    currentEMA = 0.0f;
}

// PressureSensor implementation
PressureSensor::PressureSensor() 
    : ema(0.1f),  // Default smoothing factor
      isSimulationMode(settings.SimulationMode),
      currentPressure(0.0f),
      operationMode(static_cast<int>(Stop)),
      numSamples(30) {}

void PressureSensor::setSimulationMode(bool enabled) {
    isSimulationMode = enabled;
}

void PressureSensor::setOperationMode(int mode) {
    operationMode = mode;
}

float PressureSensor::getCurrentPressure() const {
    return currentPressure;
}

float PressureSensor::readPressure() {
    float rawPressure;
    if (isSimulationMode) {
        rawPressure = simulatePressure();
    } else {
        rawPressure = readRawPressure();
    }
    
    currentPressure = ema.update(rawPressure);
    return currentPressure;
}

float PressureSensor::readRawPressure() {
    for (int i = 0; i < numSamples; ++i) {
        int raw = analogRead(settings.SensorPin);
        ema.update(static_cast<float>(raw));
        delay(10); // Small delay to allow ADC to settle
    }
    float emaRaw = ema.getCurrentEMA();
    return convertRawToPSI(emaRaw);
}

float PressureSensor::simulatePressure() {
    static float simulatedPressure = 0.0f;
    const float changeRate = 0.1f;  // PSI per update
    
    // Simulate pressure changes based on operation mode
    switch (static_cast<OperationMode>(operationMode)) {
        case Inflate:
            simulatedPressure += changeRate;
            break;
        case Deflate:
            simulatedPressure = max(0.0f, simulatedPressure - changeRate);
            break;
        case Stop:
        default:
            // Pressure remains constant
            break;
    }
    
    return simulatedPressure;
}

float PressureSensor::convertRawToPSI(int rawValue) {
    // Convert raw value to PSI using calibration values
    float range = settings.SensorValueAt50PSI - settings.SensorValueAt0PSI;
    float onePSI = range / 50.0f;
    float zeroPSI = settings.SensorValueAt0PSI;
    float pressure = (rawValue - zeroPSI) / onePSI;
    
    return (pressure < 1.0f) ? 0.0f : pressure;
}

void PressureSensor::reset() {
    ema.reset();
    currentPressure = 0.0f;
    operationMode = static_cast<int>(Stop);
}

float PressureSensor::convertToBars(float psi) {
    return psi * 0.0689476f;  // 1 PSI = 0.0689476 bar
} 