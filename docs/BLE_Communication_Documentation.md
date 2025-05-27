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