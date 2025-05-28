## Debug Mode Command (Command 7)

### Description
Enables or disables debug mode on the device. When debug mode is enabled, the device will include raw sensor data (ADC and voltage) in the pressure report sent over BLE.

### Command Structure
- **Command code:** 7
- **JSON Example to Enable Debug Mode:**
```json
{
  "command": 7,
  "enabled": true
}
```
- **JSON Example to Disable Debug Mode:**
```json
{
  "command": 7,
  "enabled": false
}
```

### Response Data Format
When debug mode is enabled, the pressure report JSON sent from the device will include the following fields:

```json
{
  "Pressure": "<float, PSI>",
  "operationMode": "<int>",
  "rawADC": "<int>",
  "voltage": "<float, 3 decimals>"
}
```
- `Pressure`: The calculated pressure in PSI.
- `operationMode`: The current operation mode.
- `rawADC`: The filtered raw ADC value from the sensor (same sample as Pressure).
- `voltage`: The calculated voltage from the same sample (3 decimal places).

When debug mode is disabled, only `Pressure` and `operationMode` are included in the report. 

## Pressure Calculation Methods

The device supports two methods for calculating pressure from the analog sensor. The method can be selected using the `PressureCalcMethod` setting (1 or 2).

### Method 1 (Default)
- Uses a linear mapping between two calibration points (SensorValueAt0PSI and SensorValueAt50PSI), which are the raw ADC values at 0 and 50 PSI.
- Formula:
  ```
  pressure = (sensorVal - SensorValueAt0PSI) / ((SensorValueAt50PSI - SensorValueAt0PSI) / 50.0)
  ```

### Method 2 (Voltage Calibration, for sensors with voltage divider)
- `SensorValueAt0PSI` and `SensorValueAt50PSI` are the voltages (in volts, after the voltage divider) at 0 and 50 PSI.
- The device uses a voltage divider (2 x 10kΩ) to scale the sensor's output (0.5–4.5V) to the ADC input (0–3.3V).
- The method undoes the voltage divider to recover the original sensor voltage, then linearly interpolates between the calibration voltages.
- Formula:
  ```
  voltageAtADC = (sensorVal / 4095.0) * 3.3
  sensorVoltage = voltageAtADC * 2.0
  v0 = SensorValueAt0PSI * 2.0
  v50 = SensorValueAt50PSI * 2.0
  pressure = (sensorVoltage - v0) * (50.0 / (v50 - v0))
  ```
- This method is recommended for sensors with a known voltage output range and a voltage divider.

### Selecting the Method
- Set `PressureCalcMethod` to `1` for the default method (ADC calibration).
- Set `PressureCalcMethod` to `2` for voltage calibration (recommended for sensors with voltage divider and known voltage output). 

## PressureCalcMethod Setting in BLE Settings

The `PressureCalcMethod` field is included in the BLE settings JSON. It allows you to select which method the device uses to calculate pressure from the analog sensor.

- **Field:** `PressureCalcMethod`
- **Type:** integer (1 or 2)
- **Usage:**
    - `1`: Use the default ADC calibration method (raw ADC values at 0 and 50 PSI)
    - `2`: Use voltage calibration (for sensors with voltage divider, see details above)

### Example BLE Settings JSON (with PressureCalcMethod)
```json
{
  "DeviceName": "esp32-AirUpDownController",
  "wifissid": "",
  "wifipassword": "",
  ...
  "PressureCalcMethod": 2
}
```

When you send settings to the device via BLE, you can include `PressureCalcMethod` to select the calculation method. When you request settings from the device, the current value of `PressureCalcMethod` will be included in the response. 