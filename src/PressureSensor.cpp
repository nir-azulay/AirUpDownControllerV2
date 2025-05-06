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
#include "../include/main.h"

// PressureSensor implementation
PressureSensor::PressureSensor(int pin)
    : sensorPin(pin), filteredPressure(0), lastRawADC(0), lastVoltage(0) {}

void PressureSensor::begin() {
    analogReadResolution(12); // Set ADC to 12-bit resolution (0-4095)
    delay(1000); // Let the sensor and capacitors stabilize
}

float PressureSensor::readFilteredPressure() {
    int samples[PressureSensor::NUM_SAMPLES];
    int validSamples = 0;

    // Take multiple samples
    for (int i = 0; i < PressureSensor::NUM_SAMPLES; i++) {
        int sample = analogRead(sensorPin);
        if (sample > 0) {
            samples[validSamples++] = sample;
        }
        delayMicroseconds(20);
    }

    if (validSamples < PressureSensor::NUM_SAMPLES / 2) {
        lastRawADC = 0;
        lastVoltage = 0;
        return 0;
    }

    // Sort the valid samples
    for (int i = 0; i < validSamples - 1; i++) {
        for (int j = i + 1; j < validSamples; j++) {
            if (samples[j] < samples[i]) {
                int temp = samples[i];
                samples[i] = samples[j];
                samples[j] = temp;
            }
        }
    }

    // Get the median value
    float medianADC;
    if (validSamples % 2 == 1) {
        medianADC = samples[validSamples / 2];
    } else {
        medianADC = (samples[validSamples / 2 - 1] + samples[validSamples / 2]) / 2.0f;
    }
    lastRawADC = (int)medianADC;

    // Convert to voltage at ADC pin
    float voltageAtADC = medianADC * (PressureSensor::ADC_REF_VOLTAGE / PressureSensor::ADC_RESOLUTION);
    // Calculate real sensor voltage (before voltage divider)
    float realVoltage = voltageAtADC * 2.0;
    lastVoltage = realVoltage;

    // Convert voltage to pressure using calibration curve
    float pressure = voltageToRealPressure(realVoltage);

    // Debug output
    Serial.print(medianADC, 0); Serial.print(",");
    Serial.print(realVoltage, 3); Serial.print(",");
    Serial.print(pressure, 1); Serial.println();

    return pressure;
}

int PressureSensor::readRawADC() const {
    return lastRawADC;
}

float PressureSensor::readVoltage() const {
    return lastVoltage;
}

float PressureSensor::exponentialMovingAverage(float newValue, float oldValue, float alpha) {
    return alpha * newValue + (1 - alpha) * oldValue;
}

float PressureSensor::voltageToRealPressure(float voltage) {
    float pressure = PressureSensor::CAL_PRESSURE_LOW + ((voltage - PressureSensor::CAL_VOLTAGE_LOW) / PressureSensor::VOLTAGE_PER_PSI);
    if (pressure < 0) {
        pressure = 0;
    } else if (pressure > PressureSensor::SENSOR_MAX_PSI) {
        pressure = PressureSensor::SENSOR_MAX_PSI;
    }
    return pressure;
} 