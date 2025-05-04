# AirUpDownController BLE Communication Documentation

## Overview

This document describes the Bluetooth Low Energy (BLE) communication protocol implemented in the AirUpDownController device. This protocol enables mobile applications to monitor and control air pressure operations wirelessly.

## Device Description

The AirUpDownController is an ESP32-based device designed to control air pressure for inflating and deflating applications. It provides a BLE interface for remote monitoring and control via mobile applications.

## BLE Configuration

### Service and Characteristics

| Component | UUID | Properties | Description |
|-----------|------|------------|-------------|
| **Service** | `4fafc201-1fb5-459e-8fcc-c5c9c331914b` | - | Main service for air controller communication |
| **Status Characteristic** | `beb5483e-36e1-4688-b7f5-ea07361b26a8` | NOTIFY | Sends status updates from device to client |
| **Command Characteristic** | `e3223119-9445-4e96-a4a1-85358c4046a2` | READ, WRITE, WRITE_WITHOUT_RESPONSE | Receives commands from client to device |

### Technical Details

- MTU Size: 517 bytes
- Descriptor: 2901 (User Description) - "Air Controller Status"
- Advertising: Enabled with scan response
- Multiple connections: Supported

## Message Format

All messages use JSON format for data exchange.

### Status Messages (Device to Client)

#### Current Pressure Status
```json
{
  "Pressure": "xx.x",
  "operationMode": "x"
}
```
- `Pressure`: Current pressure in PSI with one decimal precision
- `operationMode`: Operation mode (0 = Stop, 1 = Inflate, 2 = Deflate)

#### Settings Information
```json
{
  "operationMode": 4,
  "DeviceName": "string",
  "wifissid": "string",
  "wifipassword": "string",
  "RelayCompressorPin": "int",
  "RelayFreePin": "int",
  "SensorPin": "int",
  "AutoStart": "bool",
  "MaxSecForStatus": "int",
  "SensorValueAt0PSI": "int",
  "SensorValueAt50PSI": "int",
  "MessureWait": "double",
  "Precision": "double",
  "BTPinCode": "string",
  "CurrentPressure": "double",
  "Software_VER": "string",
  "IP": "string",
  "SimulationMode": "bool"
}
```

#### Target Reached Notification
```json
{
  "type": "target_reached",
  "targetPressure": "xx.x",
  "currentPressure": "xx.x"
}
```

### Command Messages (Client to Device)

#### Stop Operation (Command 1)
```json
{
  "command": 1
}
```

#### Start Operation (Command 2)
```json
{
  "command": 2,
  "targetPressure": xx.x
}
```
- `targetPressure`: Desired pressure in PSI

#### Update Settings (Command 3)
```json
{
  "command": 3,
  "DeviceName": "string",
  "wifissid": "string",
  "wifipassword": "string",
  "RelayCompressorPin": int,
  "RelayFreePin": int,
  "SensorPin": int,
  "AutoStart": bool,
  "MaxSecForStatus": int,
  "SensorValueAt0PSI": int,
  "SensorValueAt50PSI": int,
  "MessureWait": double,
  "Precision": double,
  "BTPinCode": "string",
  "SimulationMode": bool
}
```

#### Toggle Simulation Mode (Command 4)
```json
{
  "command": 4,
  "enabled": bool
}
```

#### Change Log Level (Command 5)
```json
{
  "command": 5,
  "level": int
}
```
- `level`: Log level (0 = NONE, 1 = ERROR, 2 = WARNING, 3 = INFO, 4 = DEBUG, 5 = VERBOSE)

#### Request Current Settings (Command 6)
```json
{
  "command": 6
}
```

## Connection Process

1. Device advertises with the configured device name (from settings)
2. Client connects to the device
3. Upon first connection, device automatically sends current settings
4. Client can then send commands and receive status updates

## Message Handling Details

### Large Message Support

The device supports receiving large messages that may be split into multiple chunks:
- Incoming messages are assembled until a complete JSON message is received
- A message is considered complete when it contains a closing brace `}`
- Partial messages are stored in the `incompleteBLEMessage` buffer

### Status Update Frequency

Status updates are sent:
- Periodically based on the `MessureWait` setting
- When operation mode changes (inflate, deflate, stop)
- When target pressure is reached

### Error Handling

- Missing required fields in commands: Command is ignored
- Invalid JSON messages: Logged as error and ignored
- Connection loss: Device restarts advertising automatically

## Log Levels

The device supports the following log levels which can be adjusted via Command 5:

| Level | Name | Description |
|-------|------|-------------|
| 0 | NONE | No logging |
| 1 | ERROR | Only critical errors |
| 2 | WARNING | Errors and warnings |
| 3 | INFO | General operational information |
| 4 | DEBUG | Detailed information for troubleshooting |
| 5 | VERBOSE | Maximum logging detail |

## Implementation Example

### Mobile App Connection Sequence

1. Scan for devices
2. Connect to device with service UUID `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
3. Subscribe to notifications on characteristic `beb5483e-36e1-4688-b7f5-ea07361b26a8`
4. Receive initial settings message
5. Send commands via characteristic `e3223119-9445-4e96-a4a1-85358c4046a2`
6. Process status updates from notification characteristic

### Common Operations

#### Setting Target Pressure
```json
{"command": 2, "targetPressure": 35.5}
```

#### Stopping Operation
```json
{"command": 1}
```

#### Requesting Current Settings
```json
{"command": 6}
```

## Simulation Mode

The device provides a simulation mode for testing without actual hardware connections:
- Enable/disable via Command 4
- When enabled, the device simulates pressure changes:
  - Inflation: +0.5 PSI per measurement cycle
  - Deflation: -0.3 PSI per measurement cycle
- Pressure values are constrained between 0 and 50 PSI 