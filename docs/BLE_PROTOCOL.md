# BLE Communication Protocol Documentation

## Device Information
- Device Name: Dynamic (configurable in settings)
- Service UUID: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
- MTU Size: 517 bytes

## Characteristics

### 1. Notification Characteristic
- UUID: `beb5483e-36e1-4688-b7f5-ea07361b26a8`
- Properties: NOTIFY
- Description: Used by the device to send status updates and settings to the client
- Notifications are enabled using the 0x2902 descriptor

### 2. Command Characteristic
- UUID: `e3223119-9445-4e96-a4a1-85358c4046a2`
- Properties: READ, WRITE
- Description: Used by the client to send commands to the device

## Message Formats

### Device to Client Messages (Notifications)

#### 1. Status Update
```json
{
    "Pressure": "current_pressure",
    "operationMode": "operation_mode"
}
```
- `current_pressure`: Float value representing current pressure in PSI (with 1 decimal precision)
- `operationMode`: Integer representing operation mode (1: Stop, 2: Inflate, 3: Deflate)

#### 2. Settings Update
```json
{
    "operationMode": 4,
    "DeviceName": "string",
    "wifissid": "string",
    "wifipassword": "string",
    "RelayCompressorPin": "number",
    "RelayFreePin": "number",
    "SensorPin": "number",
    "AutoStart": "boolean",
    "MaxSecForStatus": "number",
    "SensorValueAt0PSI": "number",
    "SensorValueAt50PSI": "number",
    "MessureWait": "number",
    "Precision": "number",
    "BTPinCode": "string",
    "CurrentPressure": "number",
    "Software_VER": "string",
    "IP": "string"
}
```

### Client to Device Commands

Commands are sent as JSON objects through the Command Characteristic:

#### 1. Stop Command
```json
{
    "command": 1
}
```

#### 2. Start/Set Target Pressure Command
```json
{
    "command": 2,
    "targetPressure": number
}
```
- `targetPressure`: Desired pressure value in PSI

#### 3. Update Settings Command
```json
{
    "command": 3,
    "DeviceName": "string",
    "wifissid": "string",
    "wifipassword": "string",
    "RelayCompressorPin": number,
    "RelayFreePin": number,
    "SensorPin": number,
    "AutoStart": boolean,
    "MaxSecForStatus": number,
    "SensorValueAt0PSI": number,
    "SensorValueAt50PSI": number,
    "MessureWait": number,
    "Precision": number,
    "BTPinCode": "string"
}
```

## Connection Flow

1. Device advertises using the configured device name
2. Client connects to the device
3. Client enables notifications on the Notification Characteristic
4. Device sends current settings upon first connection
5. Device sends periodic status updates through notifications
6. Client can send commands at any time through the Command Characteristic

## Important Notes

1. All JSON messages must end with a closing brace `}`
2. The device will automatically send settings on first connection
3. Status updates are sent:
   - Periodically based on pressure changes
   - When operation mode changes
   - After each pressure reading
4. After settings update (command 3), the device will restart
5. Pressure values are in PSI units
6. The device supports chunked BLE messages for larger payloads
7. Pressure readings have a configurable precision (default 0.5 PSI)
8. Pressure measurement wait time is configurable (default 2 seconds)

## Error Handling

- If a command is malformed or cannot be parsed, it will be ignored
- If target pressure is unreachable, the device will continue trying to reach it
- Connection loss will trigger automatic disconnection handling and advertising restart
- If WiFi connection fails, device will create an access point with the configured device name

## Example Integration Code (Pseudocode)

```python
# Example connection and command sending
def connect_to_device():
    device = BLE.scan_for_service("4fafc201-1fb5-459e-8fcc-c5c9c331914b")
    device.connect()
    notify_char = device.get_characteristic("beb5483e-36e1-4688-b7f5-ea07361b26a8")
    command_char = device.get_characteristic("e3223119-9445-4e96-a4a1-85358c4046a2")
    
    # Enable notifications
    notify_char.enable_notifications(callback=on_notification)
    
    return device, command_char

def send_target_pressure(command_char, pressure):
    command = {
        "command": 2,
        "targetPressure": pressure
    }
    command_char.write(json.dumps(command))

def on_notification(data):
    status = json.loads(data)
    if "Pressure" in status:
        # Handle pressure update
        current_pressure = float(status["Pressure"])
        operation_mode = int(status["operationMode"])
    else:
        # Handle settings update
        settings = status
``` 