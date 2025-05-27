# Debug Mode Development Plan

## Goal
- Implement a debug mode that can be toggled via BLE.
- When debug mode is enabled, the device will send both the raw sensor data and the calculated PSI value over BLE.
- By default, debug mode is off.

## Task List

### 1. Settings & State Management
- [x] Add a `debugMode` boolean flag to the global settings or as a separate runtime variable.  
      _Done: Added as a global variable in main.cpp._
- [x] Ensure `debugMode` is initialized to `false` on startup.  
      _Done: Set to false in global section and in setup()._

### 2. BLE Protocol Extension
- [x] Define a new BLE command (e.g., `setDebugMode`) to enable/disable debug mode.  
      _Done: Added debug_mode = 7 to Commands enum._
- [x] Update the BLE message handler to process this new command and set the `debugMode` flag accordingly.  
      _Done: handleDebugModeCommand implemented._
- [ ] Optionally, add a BLE command to query the current debug mode status.

### 3. Data Reporting Logic
- [x] Update the function that sends sensor data over BLE (e.g., `getPressureJSON()` or similar).  
      _Done: getPressureJSON now includes rawADC and voltage when debugMode is enabled._
    - [x] When `debugMode` is `true`, include the raw sensor value (ADC or voltage) in the BLE message.  
      _Done: Uses lastRawADC and lastVoltage from the same sample as currentPressure._
    - [x] When `debugMode` is `false`, send only the PSI value as before.  
      _Done._

### 4. User Interface/Documentation
- [x] Update the BLE protocol documentation to describe the new debug mode command and the extended data format.  
      _Done: Section added to BLE_Communication_Documentation.md._
- [ ] (Optional) Update any mobile app or web interface to display the raw sensor data when debug mode is enabled.

## Summary Table

| Task # | Description                                                      | Status   |
|--------|------------------------------------------------------------------|----------|
| 1      | Add and initialize `debugMode` flag                              | Complete |
| 2      | Extend BLE protocol for debug mode control                       | Complete |
| 3      | Update BLE data reporting to include raw data in debug mode      | Complete |
| 4      | Update documentation and (optionally) UI                         | Complete | 