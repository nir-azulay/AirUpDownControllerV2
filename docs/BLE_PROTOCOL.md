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
    "IP": "string",
    "SimulationMode": "boolean"
}
```
- All device settings are included in this notification
- The `SimulationMode` field indicates whether simulation mode is currently enabled (true) or disabled (false)
- This notification is sent when the device first connects to a client or when the client sends a Get Settings command

#### 3. Target Pressure Reached Notification
```json
{
    "type": "target_reached",
    "targetPressure": "number",
    "currentPressure": "number"
}
```
- `type`: Always "target_reached" to identify this notification type
- `targetPressure`: The target pressure that was set
- `currentPressure`: The current pressure reading when target was reached
- This notification is sent once when the system reaches the target pressure and stops

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
    "BTPinCode": "string",
    "SimulationMode": boolean
}
```
- All fields are optional - only include the settings you want to update
- After updating settings, the device will restart to apply the changes
- The `SimulationMode` field can be used to enable (true) or disable (false) simulation mode

#### 4. Simulation Mode Command
```json
{
    "command": 4,
    "enabled": boolean
}
```
- `enabled`: true להפעלת מצב סימולציה, false לכיבוי מצב סימולציה
- במצב סימולציה:
  - הלחץ יעלה ב-0.5 PSI בכל קריאה בזמן ניפוח
  - הלחץ ירד ב-0.3 PSI בכל קריאה בזמן שחרור אוויר
  - הערכים מוגבלים לטווח של 0-50 PSI
  - קריאות מהחיישן האמיתי יתעלמו
  - הלחץ ההתחלתי יאופס ל-0 בעת הפעלת הסימולציה
- **Important**: The simulation mode setting is now persistent and will be saved to the device's memory. The setting will be retained even after device restart or power cycle.
- The current simulation mode status is included in the Settings Update notification as the `SimulationMode` field.

#### 5. Log Level Command
```json
{
    "command": 5,
    "level": number
}
```
- `level`: רמת הלוג הרצויה (0-5)
  - 0: NONE - ללא לוגים
  - 1: ERROR - שגיאות בלבד
  - 2: WARNING - שגיאות ואזהרות
  - 3: INFO - מידע כללי (ברירת מחדל)
  - 4: DEBUG - מידע לדיבוג
  - 5: VERBOSE - מידע מפורט מאוד

#### 6. Get Settings Command
```json
{
    "command": 6
}
```
- שליחת פקודה זו תגרום למכשיר לשלוח את כל ההגדרות הנוכחיות
- המכשיר ישלח את ההגדרות בפורמט JSON דרך ה-Notification Characteristic
- אין צורך בפרמטרים נוספים לפקודה זו

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
9. All settings, including simulation mode, are persistent and saved to the device's flash memory

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