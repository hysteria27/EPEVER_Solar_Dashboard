# EPEVER Solar Dashboard

An ESP32-based BLE Wi-Fi provisioning system for solar power monitoring.

## Features
- BLE Wi-Fi provisioning via official Espressif provisioning protocol
- One-button reset (GPIO 0 BOOT button)
- Secure connection with proof of possession (PoP)
- Non-blocking IP address polling
- Automatic credential persistence

## Hardware Requirements
- ESP32 Development Board
- USB Cable for programming
- Access to BOOT button (GPIO 0)

## Installation

### Prerequisites
- Arduino IDE installed
- ESP32 board package added to Arduino IDE

### Steps
1. Open `EPEVER_Solar_Dashboard.ino` in Arduino IDE
2. Install required libraries via Library Manager:
   - `WiFiProv` (built-in with ESP32 core)
   - `WiFi` (built-in with ESP32 core)
3. Select your board: **Tools → Board → esp32 → Your ESP32 Board**
4. Select COM port: **Tools → Port → COMX**
5. Upload the sketch: **Sketch → Upload** or press `Ctrl+U`

## Configuration

Edit the following variables in the code:

```cpp
const char* service_name = "PROV_ESP32_SETUP";  // BLE device name
const char* pop = "mysecretkey";                 // Proof of possession (security key)
const int BUTTON_PIN = 0;                        // GPIO pin for reset button
```

## Usage

### First-Time Setup
1. Upload the sketch to your ESP32
2. Open Serial Monitor (baud rate: 115200)
3. Use the official **ESP BLE Provisioning** app (iOS/Android) to connect
4. Scan for `PROV_ESP32_SETUP` in the app
5. Enter the proof of possession: `mysecretkey`
6. Provide your Wi-Fi credentials
7. The device will connect and store credentials

### Reset Wi-Fi Credentials
1. Press and hold the BOOT button (GPIO 0) for 3 seconds
2. The device will clear saved credentials and restart
3. Device is ready for re-provisioning

## Serial Monitor Output

```
Starting BLE Wi-Fi Provisioning...
Provisioning Started! Please connect with the app.
Wi-Fi Credentials Received.
Successfully Provisioned Wi-Fi Credentials.
Device Connected to Wi-Fi!
IP Address: 192.168.x.x
Connected! IP: 192.168.x.x
```

## Troubleshooting

- **No BLE device found**: Try resetting the board and restarting the provisioning app
- **Won't connect to Wi-Fi**: Double-check SSID and password in the provisioning app
- **Credentials not saved**: Ensure the sketch uploaded successfully

## License
MIT License - Feel free to use and modify for your projects

## Contributing
Pull requests and issues are welcome!
