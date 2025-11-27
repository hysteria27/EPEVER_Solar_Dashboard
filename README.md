# EPEVER Solar Dashboard

An ESP32-based web dashboard for monitoring and controlling an EPEVER MPPT solar charge controller via Modbus RTU over RS485.

## Features
- **Real-Time Monitoring**: Live data display (PV voltage/current, battery voltage, charge current, load power, etc.)
- **Web-Based Control**: Responsive web interface accessible from any device on the network
- **Advanced Settings**: Full control over 16+ voltage protection parameters and battery configuration
- **Smart Write Protocol**: Hybrid Modbus approach (FC16 with FC06 fallback) for maximum compatibility
- **Safety-First Logic**: Intelligent voltage sequence management to prevent invalid register states
- **Persistent Logging**: Minute-by-minute CSV logging to internal LittleFS storage
- **WiFi & AP Fallback**: Connects to WiFi or creates fallback hotspot (EPEVER_Direct)
- **PIN-Protected Settings**: Secure access to configuration changes
- **Real-Time Updates**: WebSocket-based live data push to connected clients
- **NTP Time Sync**: Accurate timestamps for logging and diagnostics
- **Modbus Diagnostics**: Serial monitor verification and detailed error logging

## Hardware Requirements
- **ESP32 Development Board** (any variant with WiFi & UART)
- **RS485 Transceiver Module** (e.g., MAX485 or similar)
- **EPEVER MPPT Charge Controller** (Tracer series, MPPsolar, or compatible)
- **USB Cable** for programming
- Wiring:
  - RS485 DE/RE → GPIO 2
  - RS485 RX → GPIO 16 (Serial2 RX)
  - RS485 TX → GPIO 17 (Serial2 TX)
  - RS485 A/B → Controller A485+/B485-

## Installation

### Prerequisites
- Arduino IDE (v1.8.19 or later)
- ESP32 board package installed
- Required libraries (install via Library Manager):
  - `AsyncTCP` by me-no-dev
  - `ESPAsyncWebServer` by me-no-dev
  - `ArduinoJson` by Benoit Blanchon
  - `ModbusMaster` by Doc Walker
  - (WiFi and LittleFS are built-in with ESP32 core)

### Steps
1. Clone or download this repository
2. Open `Solar_MPPT_Monitor_Web.ino` in Arduino IDE
3. Create/edit `config.h` with your WiFi credentials:
   ```cpp
   #ifndef CONFIG_H
   #define CONFIG_H
   
   const char* ssid = "YourSSID";
   const char* password = "YourPassword";
   const char* ADMIN_PIN = "123456";
   
   #endif
   ```
4. Select board: **Tools → Board → esp32 → ESP32 Dev Module** (or your variant)
5. Select COM port: **Tools → Port → COM##**
6. Upload: **Sketch → Upload** (Ctrl+U)

## Configuration

Edit `config.h` to set:
- WiFi SSID and password
- Admin PIN for settings protection (default: `123456`)
- Fallback AP credentials in `Solar_MPPT_Monitor_Web.ino`:
  ```cpp
  const char* ap_ssid = "EPEVER_Direct";    // AP hotspot name
  const char* ap_pass = "12345678";         // AP password
  ```

## Modbus Settings
Configured in the sketch (RS485 pins and baud rate):
```cpp
#define MODBUS_SLAVE_ID 0x01
#define MODBUS_SERIAL Serial2
#define RX_PIN 16
#define TX_PIN 17
#define RS485_DE_RE_PIN 2
// Baud rate: 115200
```

## Usage

### Web Dashboard
1. Upload the sketch and open Serial Monitor (115200 baud)
2. Note the assigned IP address (or use `epever.local` if mDNS is available)
3. Open a web browser and navigate to: `http://<IP-Address>` or `http://192.168.x.x`
4. View live telemetry, history, and control settings

### Live Dashboard
- Real-time values for PV, battery, load, and controller temperature
- Battery SOC (State of Charge) percentage
- Daily energy generation summary

### History & Logging
- 24-hour line chart of power generation
- Daily summary statistics
- Download CSV logs for external analysis
- Minute-by-minute granularity

### Settings Control
- Battery Type, Capacity, Temperature Compensation
- Voltage Protection: OVD, CLV, OVR, EQV, BST, FLT, BSR
- Load Protection: LVR, UVR, UVW, LVD, DLV
- Equalization and Boost Time settings
- PIN-protected writes with validation

### Initial Connection
1. If WiFi is unavailable, the device creates an AP hotspot named `EPEVER_Direct`
2. Connect to it and access the dashboard at `http://192.168.4.1`

## Serial Monitor Output

```
--- SOLAR MONITOR V9.7 BOOT ---
=== MODBUS DIAGNOSTICS ===
Slave ID: 0x01 | Baud: 115200 | Pins: RX=16 TX=17 DE_RE=2
Testing read (Status register)... ✓ SUCCESS - Got value: 256
→ Modbus connection OK, writes should work!
==========================

WiFi Connected
IP Address: 192.168.1.X

--- CURRENT CONTROLLER SETTINGS ---
Bat Type: 0 | Capacity: 100 | Temp Comp: 0 | Rated Volt: 12
OVD: 16.20 | CLV: 15.30 | OVR: 15.80
...
```

## Troubleshooting

### No Modbus Connection
- Verify RS485 wiring: A485+, B485-, GND connections
- Check MODBUS_SLAVE_ID matches controller (usually 0x01)
- Ensure baud rate is 115200
- Test with external Modbus tool (e.g., ModbusRTU Master)
- Add terminator resistor (120Ω) between A and B if cable is long

### WiFi Connection Issues
- Check SSID and password in `config.h`
- Device will fall back to `EPEVER_Direct` AP if WiFi fails
- Verify ESP32 power supply is stable

### Settings Won't Write
- Confirm PIN is correct (default: `123456`)
- Check Serial Monitor for specific Modbus errors
- Some controllers may require FC06 instead of FC16 (firmware handles this)
- Verify register address is correct for your controller model

### No Web Dashboard Access
- Find IP address in Serial Monitor output
- Try `http://192.168.4.1` if connected to AP hotspot
- Ensure device is on same network as client

## API Endpoints

- `GET /` - Main HTML dashboard
- `GET /api/settings` - Retrieve current controller settings
- `POST /api/settings` - Update controller settings (requires PIN)
- `GET /api/summary` - Daily energy history (JSON)
- `GET /api/day-log?date=YYYY-MM-DD` - CSV log for specific date
- `WS /ws` - WebSocket for real-time data updates

## Version History
See [changelog.md](changelog.md) for detailed version information.

Current version: **V9.7 - Modular Config with Rated Voltage Restore**

## License
MIT License - Feel free to use and modify for your projects

## Contributing
Pull requests, bug reports, and feature requests are welcome!

## Support
For issues or questions, please open an issue on GitHub.
