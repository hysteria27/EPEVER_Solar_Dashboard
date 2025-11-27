/*
  EPEVER MPPT Solar Charge Controller Web Dashboard V9.7 (MODULAR CONFIG)
  
  FEATURES:
  - Added "Rated Voltage" setting field back to UI.
  - Separated Configuration: WiFi and PIN are now in config.h
  - Hybrid Write Protocol (0x10 -> 0x06 Fallback) for maximum compatibility.
  - Smart Direction: Logic to handle raising/lowering voltage limits safely.
  - Check-Before-Write: Skips writing if the parameter is already set correctly.
  - Diagnostic Mode: Prints all voltage settings to Serial Monitor on load.
  - Connectivity: WiFi Station Mode + AP Fallback (EPEVER_Direct).
  - Logging: Minute-by-minute CSV logging to LittleFS.
*/

#include <WiFi.h>
#include <WiFiProv.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <ArduinoJson.h>
#include <ModbusMaster.h>
#include <LittleFS.h>
#include <time.h>

// --- INCLUDE UI & CONFIG ---
#include "web_page.h" 
#include "config.h" // <--- Credentials are now here

// --- AP FALLBACK ---
const char* ap_ssid = "EPEVER_Direct";
const char* ap_pass = "12345678";
bool isAPMode = false;

// --- TIME CONFIG ---
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 25200; // UTC+7 (WIB)
const int   daylightOffset_sec = 0;

// --- MODBUS CONFIG ---
#define MODBUS_SLAVE_ID 0x01
#define MODBUS_SERIAL Serial2
#define RX_PIN 16
#define TX_PIN 17
#define RS485_DE_RE_PIN 2

// --- TIMING ---
const long POLLING_INTERVAL = 2000; 
const long MINUTE_LOG_INTERVAL = 60000;
const long HISTORY_SAVE_INTERVAL = 600000;

unsigned long lastModbusUpdate = 0;
unsigned long lastHistorySave = 0;
unsigned long lastMinuteLog = 0;
unsigned long lastWiFiCheck = 0;

// --- UPDATE FLAGS ---
bool shouldUpdateSettings = false;
StaticJsonDocument<2048> pendingSettings; 

// --- OBJECTS ---
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
ModbusMaster node;

// --- REGISTERS ---
// Live Data (Input Registers - 0x04)
const uint16_t REG_PV_VOLTAGE      = 0x3100; 
const uint16_t REG_PV_CURRENT      = 0x3101; 
const uint16_t REG_PV_POWER_L      = 0x3102; 
const uint16_t REG_BAT_VOLTAGE     = 0x3104; 
const uint16_t REG_CHARGE_CURRENT  = 0x3105; 
const uint16_t REG_LOAD_VOLTAGE    = 0x310C; 
const uint16_t REG_LOAD_CURRENT    = 0x310D; 
const uint16_t REG_LOAD_POWER_L    = 0x310E; 
const uint16_t REG_TEMP_CTRL       = 0x3111; 
const uint16_t REG_BAT_SOC         = 0x311A; 
const uint16_t REG_STATUS          = 0x3201; 
const uint16_t REG_DAILY_ENERGY_L  = 0x330C;

// Settings (Holding Registers - 0x03/0x06/0x10)
const uint16_t REG_SET_BAT_TYPE    = 0x9000; 
const uint16_t REG_SET_BAT_CAP     = 0x9001; 
const uint16_t REG_SET_TEMP_COMP   = 0x9002; 
const uint16_t REG_SET_OVD         = 0x9003; 
const uint16_t REG_SET_CLV         = 0x9004; 
const uint16_t REG_SET_OVR         = 0x9005; 
const uint16_t REG_SET_EQV         = 0x9006; 
const uint16_t REG_SET_BST         = 0x9007; 
const uint16_t REG_SET_FLT         = 0x9008; 
const uint16_t REG_SET_BSR         = 0x9009; 
const uint16_t REG_SET_LVR         = 0x900A; 
const uint16_t REG_SET_UVR         = 0x900B; 
const uint16_t REG_SET_UVW         = 0x900C; 
const uint16_t REG_SET_LVD         = 0x900D; 
const uint16_t REG_SET_DLV         = 0x900E; 
const uint16_t REG_SET_RATED_VOLT  = 0x9067; 
const uint16_t REG_SET_EQ_TIME     = 0x906B;
const uint16_t REG_SET_BST_TIME    = 0x906C;

struct EpeverData {
  float batVoltage; float pvVoltage; float pvCurrent; float pvPower;
  float chargeCurrent; float batSOC; float loadVoltage; float loadCurrent;
  float loadPower; float temp; float dailyEnergy;
  uint16_t rawStatus; String statusStr; bool valid;
};

struct EpeverSettings {
  uint16_t batType; uint16_t batCap; uint16_t tempComp;
  uint16_t ratedVolt; // ADDED
  float ovd; float clv; float ovr; float eqv; float bst; float flt;
  float bsr; float lvr; float uvr; float uvw; float lvd; float dlv;
  uint16_t eqTime; uint16_t bstTime;
  bool valid;
};

EpeverData liveData = {0,0,0,0,0,0,0,0,0,0,0,0,"BOOTING", false};
EpeverSettings currentSettings;

const char* HISTORY_FILE = "/history.json";

void preTransmission() { 
  delay(2); 
  digitalWrite(RS485_DE_RE_PIN, HIGH); 
  delay(2);
}
void postTransmission() { 
  delay(2);
  digitalWrite(RS485_DE_RE_PIN, LOW); 
  delay(2);
}

String getCurrentDate() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return "N/A";
  char buff[20]; strftime(buff, sizeof(buff), "%Y-%m-%d", &timeinfo);
  return String(buff);
}
String getCurrentTimeShort() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return "00:00";
  char buff[10]; strftime(buff, sizeof(buff), "%H:%M", &timeinfo); 
  return String(buff);
}

// Diagnostic: Verify Modbus connection to controller
void verifyModbusConnection() {
  Serial.println("\n=== MODBUS DIAGNOSTICS ===");
  Serial.printf("Slave ID: 0x%02X | Baud: 115200 | Pins: RX=%d TX=%d DE_RE=%d\n", 
    MODBUS_SLAVE_ID, RX_PIN, TX_PIN, RS485_DE_RE_PIN);
  
  // Try to read a simple register
  Serial.print("Testing read (Status register)... ");
  uint8_t result = node.readInputRegisters(REG_STATUS, 1);
  
  if (result == node.ku8MBSuccess) {
    Serial.printf("✓ SUCCESS - Got value: %d\n", node.getResponseBuffer(0));
    Serial.println("→ Modbus connection OK, writes should work!");
  } else {
    Serial.printf("✗ FAIL - Error: 0x%02X\n", result);
    Serial.println("→ Check: Slave ID, baud rate, RS485 wiring, terminator resistor");
  }
  Serial.println("==========================\n");
}

// --- MODBUS FUNCTIONS ---
void readLiveSensors() {
  uint8_t result;
  liveData.valid = false;

  node.readInputRegisters(REG_PV_VOLTAGE, 2);
  liveData.pvVoltage = node.getResponseBuffer(0) / 100.0f;
  liveData.pvCurrent = node.getResponseBuffer(1) / 100.0f;
  delay(5);

  node.readInputRegisters(REG_PV_POWER_L, 2);
  liveData.pvPower = (node.getResponseBuffer(0) | (node.getResponseBuffer(1) << 16)) / 100.0f;
  delay(5);

  node.readInputRegisters(REG_BAT_VOLTAGE, 2);
  liveData.batVoltage = node.getResponseBuffer(0) / 100.0f;
  liveData.chargeCurrent = node.getResponseBuffer(1) / 100.0f;
  delay(5);
  
  node.readInputRegisters(REG_LOAD_VOLTAGE, 4);
  liveData.loadVoltage = node.getResponseBuffer(0) / 100.0f;
  liveData.loadCurrent = node.getResponseBuffer(1) / 100.0f;
  liveData.loadPower = (node.getResponseBuffer(2) | (node.getResponseBuffer(3) << 16)) / 100.0f;
  delay(5);

  node.readInputRegisters(REG_TEMP_CTRL, 1);
  liveData.temp = node.getResponseBuffer(0) / 100.0f;
  delay(5);
  
  node.readInputRegisters(REG_BAT_SOC, 1);
  liveData.batSOC = node.getResponseBuffer(0);
  delay(5);
  
  node.readInputRegisters(REG_DAILY_ENERGY_L, 2);
  liveData.dailyEnergy = (node.getResponseBuffer(0) | (node.getResponseBuffer(1) << 16)) / 100.0f;
  delay(5);

  result = node.readInputRegisters(REG_STATUS, 1);
  if (result == node.ku8MBSuccess) {
    liveData.rawStatus = node.getResponseBuffer(0);
    liveData.statusStr = (liveData.pvPower > 1.0) ? "CHARGING" : (liveData.pvVoltage < 2.0 ? "NIGHT" : "STANDBY");
    liveData.valid = true;
  } else {
    liveData.valid = false; 
  }
}

void readEpeverSettings() {
  uint8_t result;
  
  // Read Type, Cap, Temp Comp
  result = node.readHoldingRegisters(REG_SET_BAT_TYPE, 3);
  if (result == node.ku8MBSuccess) {
    currentSettings.batType = node.getResponseBuffer(0);
    currentSettings.batCap  = node.getResponseBuffer(1);
    currentSettings.tempComp = node.getResponseBuffer(2);
  } else return; 
  delay(10);

  result = node.readHoldingRegisters(REG_SET_OVD, 12);
  if (result == node.ku8MBSuccess) {
    currentSettings.ovd = node.getResponseBuffer(0) / 100.0f;
    currentSettings.clv = node.getResponseBuffer(1) / 100.0f;
    currentSettings.ovr = node.getResponseBuffer(2) / 100.0f;
    currentSettings.eqv = node.getResponseBuffer(3) / 100.0f;
    currentSettings.bst = node.getResponseBuffer(4) / 100.0f;
    currentSettings.flt = node.getResponseBuffer(5) / 100.0f;
    currentSettings.bsr = node.getResponseBuffer(6) / 100.0f;
    currentSettings.lvr = node.getResponseBuffer(7) / 100.0f;
    currentSettings.uvr = node.getResponseBuffer(8) / 100.0f;
    currentSettings.uvw = node.getResponseBuffer(9) / 100.0f;
    currentSettings.lvd = node.getResponseBuffer(10) / 100.0f;
    currentSettings.dlv = node.getResponseBuffer(11) / 100.0f;
  } else return;
  delay(10);

  // ADDED: Read Rated Voltage
  result = node.readHoldingRegisters(REG_SET_RATED_VOLT, 1);
  if (result == node.ku8MBSuccess) {
    currentSettings.ratedVolt = node.getResponseBuffer(0);
  }
  delay(10);

  result = node.readHoldingRegisters(REG_SET_EQ_TIME, 2);
  if (result == node.ku8MBSuccess) {
    currentSettings.eqTime = node.getResponseBuffer(0);
    currentSettings.bstTime = node.getResponseBuffer(1);
    currentSettings.valid = true;
  }
}

// === HYBRID WRITE: CHECK -> TRY 0x10 -> TRY 0x06 ===
const char* getModbusErrorMsg(uint8_t code) {
  switch(code) {
    case 0x01: return "Illegal Function";
    case 0x02: return "Illegal Data Address";
    case 0x03: return "Illegal Data Value";
    case 0x04: return "Device Failure";
    case 0x05: return "Acknowledge";
    case 0x06: return "Device Busy";
    case 0x08: return "CRC Error";
    default: return "Unknown Error";
  }
}

bool writeHybrid(uint16_t reg, uint16_t val, const char* name) {
  // 1. Check if already set
  Serial.printf("[%s] Reading current value at 0x%04X... ", name, reg);
  uint8_t result = node.readHoldingRegisters(reg, 1);
  
  if (result == node.ku8MBSuccess) {
    uint16_t current = node.getResponseBuffer(0);
    Serial.printf("Current=%d\n", current);
    
    if (current == val) {
      Serial.printf("[%s] ✓ SKIP - Already correct value\n\n", name);
      return true;
    }
  } else {
    Serial.printf("FAIL - %s (0x%02X)\n", getModbusErrorMsg(result), result);
  }
  delay(50); 

  // 2. Try FC16 (Write Multiple Registers)
  Serial.printf("[%s] Writing 0x%04X via FC16... ", name, val);
  node.setTransmitBuffer(0, val);
  result = node.writeMultipleRegisters(reg, 1);
  
  if (result == node.ku8MBSuccess) {
    Serial.println("✓ OK (FC16)");
    delay(200); 
    return true;
  }
  
  Serial.printf("FAIL - %s (0x%02X)\n", getModbusErrorMsg(result), result);
  
  // 3. Fallback FC06 (Write Single Register)
  Serial.printf("[%s] Retrying via FC06... ", name);
  delay(50);
  result = node.writeSingleRegister(reg, val);
  
  if (result == node.ku8MBSuccess) {
    Serial.println("✓ OK (FC06)");
    delay(200);
    return true;
  }

  Serial.printf("✗ FAIL - %s (0x%02X)\n\n", getModbusErrorMsg(result), result);
  return false;
}

// --- BACKGROUND UPDATE TASK ---
void processSettingsUpdate() {
  Serial.println("\n--- UPDATE V9.7 (MODULAR CONFIG) ---");
  
  // 1. Battery Type, Rated Volt, Temp Comp (Write these first!)
  if(pendingSettings.containsKey("type")) writeHybrid(REG_SET_BAT_TYPE, pendingSettings["type"], "Bat Type");
  if(pendingSettings.containsKey("ratedVolt")) writeHybrid(REG_SET_RATED_VOLT, pendingSettings["ratedVolt"], "Rated Volt");
  if(pendingSettings.containsKey("cap")) writeHybrid(REG_SET_BAT_CAP, pendingSettings["cap"], "Cap");
  if(pendingSettings.containsKey("tcomp")) writeHybrid(REG_SET_TEMP_COMP, pendingSettings["tcomp"], "Temp Comp");
  
  // 2. Determine Direction: Are we raising or lowering OVD?
  bool raisingVoltages = true; // Default assumption
  uint16_t targetOVD = (uint16_t)(pendingSettings["ovd"].as<float>()*100);
  
  if (node.readHoldingRegisters(REG_SET_OVD, 1) == node.ku8MBSuccess) {
      uint16_t currentOVD = node.getResponseBuffer(0);
      if (targetOVD < currentOVD) {
          raisingVoltages = false; // Must lower floor before lowering ceiling
          Serial.println(">> DIRECTION: LOWERING (Bottom-Up)");
      } else {
          Serial.println(">> DIRECTION: RAISING (Top-Down)");
      }
  }

  // 3. EXECUTE SMART SEQUENCE
  if (raisingVoltages) {
      // TOP-DOWN: Raise ceiling first (OVD), then fill down to DLV
      if(pendingSettings.containsKey("ovd")) writeHybrid(REG_SET_OVD, targetOVD, "OVD");
      if(pendingSettings.containsKey("clv")) writeHybrid(REG_SET_CLV, (uint16_t)(pendingSettings["clv"].as<float>()*100), "CLV");
      if(pendingSettings.containsKey("ovr")) writeHybrid(REG_SET_OVR, (uint16_t)(pendingSettings["ovr"].as<float>()*100), "OVR");
      if(pendingSettings.containsKey("eqv")) writeHybrid(REG_SET_EQV, (uint16_t)(pendingSettings["eqv"].as<float>()*100), "EQV");
      if(pendingSettings.containsKey("bst")) writeHybrid(REG_SET_BST, (uint16_t)(pendingSettings["bst"].as<float>()*100), "BST");
      if(pendingSettings.containsKey("flt")) writeHybrid(REG_SET_FLT, (uint16_t)(pendingSettings["flt"].as<float>()*100), "FLT");
      if(pendingSettings.containsKey("bsr")) writeHybrid(REG_SET_BSR, (uint16_t)(pendingSettings["bsr"].as<float>()*100), "BSR");
      if(pendingSettings.containsKey("lvr")) writeHybrid(REG_SET_LVR, (uint16_t)(pendingSettings["lvr"].as<float>()*100), "LVR");
      if(pendingSettings.containsKey("uvr")) writeHybrid(REG_SET_UVR, (uint16_t)(pendingSettings["uvr"].as<float>()*100), "UVR");
      if(pendingSettings.containsKey("uvw")) writeHybrid(REG_SET_UVW, (uint16_t)(pendingSettings["uvw"].as<float>()*100), "UVW");
      if(pendingSettings.containsKey("lvd")) writeHybrid(REG_SET_LVD, (uint16_t)(pendingSettings["lvd"].as<float>()*100), "LVD");
      if(pendingSettings.containsKey("dlv")) writeHybrid(REG_SET_DLV, (uint16_t)(pendingSettings["dlv"].as<float>()*100), "DLV");
  } else {
      // BOTTOM-UP: Lower floor first (DLV), then allow ceiling to drop
      if(pendingSettings.containsKey("dlv")) writeHybrid(REG_SET_DLV, (uint16_t)(pendingSettings["dlv"].as<float>()*100), "DLV");
      if(pendingSettings.containsKey("lvd")) writeHybrid(REG_SET_LVD, (uint16_t)(pendingSettings["lvd"].as<float>()*100), "LVD");
      if(pendingSettings.containsKey("uvw")) writeHybrid(REG_SET_UVW, (uint16_t)(pendingSettings["uvw"].as<float>()*100), "UVW");
      if(pendingSettings.containsKey("uvr")) writeHybrid(REG_SET_UVR, (uint16_t)(pendingSettings["uvr"].as<float>()*100), "UVR");
      if(pendingSettings.containsKey("lvr")) writeHybrid(REG_SET_LVR, (uint16_t)(pendingSettings["lvr"].as<float>()*100), "LVR");
      if(pendingSettings.containsKey("bsr")) writeHybrid(REG_SET_BSR, (uint16_t)(pendingSettings["bsr"].as<float>()*100), "BSR");
      if(pendingSettings.containsKey("flt")) writeHybrid(REG_SET_FLT, (uint16_t)(pendingSettings["flt"].as<float>()*100), "FLT");
      if(pendingSettings.containsKey("bst")) writeHybrid(REG_SET_BST, (uint16_t)(pendingSettings["bst"].as<float>()*100), "BST");
      if(pendingSettings.containsKey("eqv")) writeHybrid(REG_SET_EQV, (uint16_t)(pendingSettings["eqv"].as<float>()*100), "EQV");
      if(pendingSettings.containsKey("ovr")) writeHybrid(REG_SET_OVR, (uint16_t)(pendingSettings["ovr"].as<float>()*100), "OVR");
      if(pendingSettings.containsKey("clv")) writeHybrid(REG_SET_CLV, (uint16_t)(pendingSettings["clv"].as<float>()*100), "CLV");
      if(pendingSettings.containsKey("ovd")) writeHybrid(REG_SET_OVD, targetOVD, "OVD");
  }

  // 4. Durations
  if(pendingSettings.containsKey("eqt")) writeHybrid(REG_SET_EQ_TIME, pendingSettings["eqt"], "Eq Time");
  if(pendingSettings.containsKey("bstt")) writeHybrid(REG_SET_BST_TIME, pendingSettings["bstt"], "Boost Time");

  Serial.println("--- DONE ---");
  shouldUpdateSettings = false;
}

// --- LOGGING ---
void updateDailySummary() {
  if (!liveData.valid || isAPMode) return; 
  String today = getCurrentDate(); if (today == "N/A") return;
  File file = LittleFS.open(HISTORY_FILE, "r");
  DynamicJsonDocument doc(8192); 
  if (file) { deserializeJson(doc, file); file.close(); }
  JsonObject dayData = doc[today].to<JsonObject>();
  dayData["y"] = liveData.dailyEnergy;
  file = LittleFS.open(HISTORY_FILE, "w");
  if (file) { serializeJson(doc, file); file.close(); }
}

void logMinuteData() {
  if (!liveData.valid || isAPMode) return;
  String today = getCurrentDate(); String timeNow = getCurrentTimeShort(); if (today == "N/A") return;
  String filename = "/" + today + ".csv";
  File file = LittleFS.open(filename, "a");
  if(file) {
    file.printf("%s,%.1f,%.1f,%.0f,%d\n", timeNow.c_str(), liveData.pvPower, liveData.batVoltage, liveData.batSOC, liveData.rawStatus);
    file.close();
  }
}

void notifyClients() {
  StaticJsonDocument<1024> doc; 
  doc["type"] = "live";
  doc["valid"] = liveData.valid;
  doc["pvVolt"] = liveData.pvVoltage;
  doc["pvCurrent"] = liveData.pvCurrent;
  doc["pvPower"] = liveData.pvPower;
  doc["battVolt"] = liveData.batVoltage;
  doc["chgAmp"] = liveData.chargeCurrent;
  doc["soc"] = liveData.batSOC;
  doc["loadVolt"] = liveData.loadVoltage;
  doc["loadAmp"] = liveData.loadCurrent;
  doc["loadPower"] = liveData.loadPower;
  doc["temp"] = liveData.temp;
  doc["dailyKwh"] = liveData.dailyEnergy;
  doc["chgStatus"] = liveData.rawStatus; 
  doc["statusStr"] = liveData.statusStr;
  char jsonBuffer[1024];
  serializeJson(doc, jsonBuffer);
  ws.textAll(jsonBuffer);
}

// --- BLE PROVISIONING ---
void startBLEProvisioning() {
  Serial.println("Starting BLE WiFi provisioning...");
  WiFi.mode(WIFI_MODE_STA);
  WiFi.onEvent(SysProvEvent);
  
  const char* prov_pop = "epeverpop";
  const char* prov_service = "PROV_EPEVER_SETUP";
  
  WiFiProv.beginProvision(NETWORK_PROV_SCHEME_BLE, NETWORK_PROV_SCHEME_HANDLER_NONE, NETWORK_PROV_SECURITY_1, prov_pop, prov_service);
}

void SysProvEvent(arduino_event_id_t event) {
  switch(event) {
    case ARDUINO_EVENT_PROV_START:
      Serial.println("Provisioning Started! Please connect with the provisioning app (BLE).");
      break;
    case ARDUINO_EVENT_PROV_CRED_RECV:
      Serial.println("Wi-Fi Credentials Received via BLE.");
      break;
    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      Serial.println("Successfully Provisioned Wi-Fi Credentials.");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("Device Connected to Wi-Fi! IP: ");
      Serial.println(WiFi.localIP());
      break;
    case ARDUINO_EVENT_PROV_END:
      Serial.println("Provisioning process finished.");
      break;
    default:
      break;
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n--- SOLAR MONITOR V9.7 BOOT ---");

  if(!LittleFS.begin(true)) { Serial.println("Mount Failed"); return; }

  pinMode(RS485_DE_RE_PIN, OUTPUT); digitalWrite(RS485_DE_RE_PIN, LOW); 
  MODBUS_SERIAL.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  node.begin(MODBUS_SLAVE_ID, MODBUS_SERIAL);
  node.preTransmission(preTransmission); 
  node.postTransmission(postTransmission);
  
  delay(500); // Let controller stabilize
  verifyModbusConnection();

  // Start BLE provisioning to accept WiFi credentials
  Serial.println("Initializing BLE WiFi provisioning...");
  startBLEProvisioning();

  ws.onEvent(onWebSocketEvent); server.addHandler(&ws);
  // Serve HTML directly from PROGMEM to avoid RAM copying and blank responses
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  
  server.on("/api/day-log", HTTP_GET, [](AsyncWebServerRequest *request){
    if(request->hasParam("date")) {
      String date = request->getParam("date")->value();
      String filename = "/" + date + ".csv";
      if(LittleFS.exists(filename)) request->send(LittleFS, filename, "text/csv");
      else request->send(404, "text/plain", "Log not found");
    } else request->send(400, "text/plain", "Missing date");
  });

  server.on("/api/summary", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(LittleFS, HISTORY_FILE, "application/json"); });

  server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request){
      readEpeverSettings();
      
      // DIAGNOSTIC DUMP TO SERIAL
      Serial.println("\n--- CURRENT CONTROLLER SETTINGS ---");
      Serial.printf("Bat Type: %d | Capacity: %d | Temp Comp: %d | Rated Volt: %d\n", currentSettings.batType, currentSettings.batCap, currentSettings.tempComp, currentSettings.ratedVolt);
      Serial.printf("OVD: %.2f | CLV: %.2f | OVR: %.2f\n", currentSettings.ovd, currentSettings.clv, currentSettings.ovr);
      Serial.printf("EQV: %.2f | BST: %.2f | FLT: %.2f | BSR: %.2f\n", currentSettings.eqv, currentSettings.bst, currentSettings.flt, currentSettings.bsr);
      Serial.printf("LVR: %.2f | UVR: %.2f | UVW: %.2f | LVD: %.2f | DLV: %.2f\n", currentSettings.lvr, currentSettings.uvr, currentSettings.uvw, currentSettings.lvd, currentSettings.dlv);
      Serial.println("-----------------------------------\n");

      StaticJsonDocument<1280> doc; 
      doc["type"] = currentSettings.batType;
      doc["cap"] = currentSettings.batCap;
      doc["tcomp"] = currentSettings.tempComp;
      doc["ratedVolt"] = currentSettings.ratedVolt; // ADDED
      doc["ovd"] = currentSettings.ovd; doc["clv"] = currentSettings.clv;
      doc["ovr"] = currentSettings.ovr; doc["eqv"] = currentSettings.eqv;
      doc["bst"] = currentSettings.bst; doc["flt"] = currentSettings.flt;
      doc["bsr"] = currentSettings.bsr; doc["lvr"] = currentSettings.lvr;
      doc["uvr"] = currentSettings.uvr; doc["uvw"] = currentSettings.uvw;
      doc["lvd"] = currentSettings.lvd; doc["dlv"] = currentSettings.dlv;
      doc["eqt"] = currentSettings.eqTime; doc["bstt"] = currentSettings.bstTime;
      String json; serializeJson(doc, json); request->send(200, "application/json", json);
  });

  // API: POST SETTINGS
  server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest *request){ }, NULL, 
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      if (shouldUpdateSettings) { request->send(429, "text/plain", "Busy"); return; }
      pendingSettings.clear();
      DeserializationError err = deserializeJson(pendingSettings, data);
      if (err) { request->send(400, "text/plain", "Bad JSON"); return; }
      if (!pendingSettings.containsKey("pin") || String(pendingSettings["pin"]) != ADMIN_PIN) {
          request->send(401, "text/plain", "Incorrect PIN"); return;
      }
      shouldUpdateSettings = true;
      request->send(200, "text/plain", "Update Queued");
  });

  server.begin();
}

void onWebSocketEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {}

void loop() {
  ws.cleanupClients();
  unsigned long currentMillis = millis();
  
  // BACKGROUND TASK (Non-blocking)
  if (shouldUpdateSettings) {
      processSettingsUpdate();
      lastModbusUpdate = millis(); 
  }

  if (!isAPMode && (WiFi.status() != WL_CONNECTED) && (currentMillis - lastWiFiCheck >= 30000)) { WiFi.disconnect(); WiFi.reconnect(); lastWiFiCheck = currentMillis; }
  if (currentMillis - lastModbusUpdate > POLLING_INTERVAL) { readLiveSensors(); notifyClients(); lastModbusUpdate = currentMillis; }
  if (currentMillis - lastMinuteLog > MINUTE_LOG_INTERVAL) { if(liveData.valid) logMinuteData(); lastMinuteLog = currentMillis; }
  if (currentMillis - lastHistorySave > HISTORY_SAVE_INTERVAL) { if(liveData.valid) updateDailySummary(); lastHistorySave = currentMillis; }
}