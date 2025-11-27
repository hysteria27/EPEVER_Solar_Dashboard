#include "WiFiProv.h"
#include <WiFi.h>

// --- Configuration ---

// 1. BLE Device Name (must start with PROV_ for the official Espressif app)
const char* service_name = "PROV_ESP32_SETUP"; 

// 2. Proof of Possession (PoP) - A simple password for a secure initial BLE connection.
// IMPORTANT: Change this from the default for a real project!
const char* pop = "mysecretkey";

// --- NEW: Button Configuration ---
const int BUTTON_PIN = 0; // GPIO 0 is the "BOOT" button on most dev boards

// --- NEW: Non-Blocking Timer ---
// We remove the delay() from loop() to keep the button responsive
unsigned long last_ip_print = 0;
const long ip_print_interval = 5000; // 5 seconds

// --- Setup ---

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Wi-Fi Provisioning...");

  // --- FORCE CLEAR ALL CREDENTIALS ---
  // WiFi.disconnect(true, true); // Disconnect and erase
  // delay(100);
  // --- END FORCE CLEAR ---

  // --- NEW: Initialize Button Pin ---
  // INPUT_PULLUP means the pin is HIGH by default, and LOW when pressed
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // 1. Initialize Wi-Fi
  WiFi.mode(WIFI_MODE_STA);

  // TEMPORARY CHECK: This will confirm if the device has saved credentials.
  // Serial.print("Credentials present in NVS? ");
  // Serial.println(WiFi.getMode() != WIFI_MODE_AP); // True if credentials are saved.
  
  // 2. Set up event handler for Wi-Fi and Provisioning events
  WiFi.onEvent(SysProvEvent);

  // 3. Start the Provisioning Process (Advertise via BLE)
  // This will advertise the device as "PROV_ESP32_SETUP" and use the PoP for security.
  WiFiProv.beginProvision(NETWORK_PROV_SCHEME_BLE, NETWORK_PROV_SCHEME_HANDLER_NONE, NETWORK_PROV_SECURITY_1, pop, service_name);
}

void loop() {
  // Your main loop code goes here. 
  // It will run while provisioning is active OR after Wi-Fi is connected.
  
  // --- NEW: Non-Blocking IP Print ---
  if (WiFi.isConnected() && (millis() - last_ip_print > ip_print_interval)) {
    Serial.print("Connected! IP: ");
    Serial.println(WiFi.localIP());
    last_ip_print = millis();
  }

  // --- NEW: Check for Reset Button ---
  // Check if the button is pressed (LOW)
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Button pressed. Hold for 3 seconds to clear Wi-Fi...");
    
    // Wait 3 seconds
    delay(3000); 

    // Check if the button is STILL pressed
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Clearing Wi-Fi credentials and restarting...");

      // Tell Wi-Fi to disconnect and, most importantly,
      // erase the saved credentials from NVS (Non-Volatile Storage)
      WiFi.disconnect(true, true); 
      
      delay(1000);
      ESP.restart(); // Restart the ESP32
    } else {
      Serial.println("Button released. Reset cancelled.");
    }
  }

  // We add a small delay to prevent the loop from running too fast
  // and spamming the button check, but it's short enough to be responsive.
  delay(100);
}

// --- Event Handler Function ---

// This function is called when any Wi-Fi or Provisioning event occurs.
void SysProvEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_PROV_START:
      Serial.println("\nProvisioning Started! Please connect with the app.");
      break;
      
    case ARDUINO_EVENT_PROV_CRED_RECV:
      Serial.println("\nWi-Fi Credentials Received.");
      break;
      
    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      Serial.println("\nSuccessfully Provisioned Wi-Fi Credentials.");
      break;
      
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.println("\nDevice Connected to Wi-Fi!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      // The Provisioning service automatically stops after a successful connection.
      break;
      
    case ARDUINO_EVENT_PROV_END:
      Serial.println("\nProvisioning process finished.");
      // You can add logic here to jump to your main application task.
      break;
      
    default:
      // Handle other Wi-Fi events (e.g., disconnection, failed connection)
      break;
  }
}