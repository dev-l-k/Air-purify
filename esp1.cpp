#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "DHT.h"

// ---------------- WiFi ----------------
#define WLAN_SSID       "YOUR_WIFI_SSID"
#define WLAN_PASS       "YOUR_WIFI_PASSWORD"

// ---------------- Web Server ----------------
ESP8266WebServer server(80);

// ---------------- Sensors ----------------
#define DHTPIN D1        // GPIO5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define LDR_PIN A0
#define LIGHT_THRESHOLD 50    // light below 50% → turn ON LED
#define TEMP_THRESHOLD 32.0   // temperature above 32°C → turn ON pump

// ---------------- L298N Pins ----------------
// Pump Motor (Motor A) - Uses PWM for speed control
#define ENA D5           // GPIO14 (PWM for pump speed)
#define IN1 D6           // GPIO12 
#define IN2 D2           // GPIO4

// 12V LED Bulb (Motor B) - Fixed PWM brightness when ON
#define ENB D7           // GPIO13 (PWM for LED - fixed at 1000 when ON)
#define IN3 D0           // GPIO16 (LED direction)
#define IN4 D8           // GPIO15 (LED direction)

// ---------------- Variables ----------------
String mode = "none";    // START WITH NO MODE SELECTED
bool pumpState = false;
bool lightState = false;
int pumpSpeedPWM = 1000;    // Pump speed at maximum (1000/1023)
int ledBrightness = 1000;   // LED brightness at maximum when ON (1000/1023)

// Sensor data
float currentTemp = 0;
float currentHum = 0;
int currentLight = 0;

// HTML Page
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Smart Agriculture System</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 20px; background: #f0f0f0; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }
        .card { background: #fff; padding: 15px; margin: 10px 0; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
        .sensor-data { background: #e3f2fd; }
        .controls { background: #f3e5f5; }
        button { padding: 10px 15px; margin: 5px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }
        .auto-btn { background: #4caf50; color: white; }
        .manual-btn { background: #ff9800; color: white; }
        .on-btn { background: #2196f3; color: white; }
        .off-btn { background: #f44336; color: white; }
        .status { padding: 8px; border-radius: 4px; margin: 5px 0; display: inline-block; }
        .on-status { background: #c8e6c9; color: #2e7d32; }
        .off-status { background: #ffcdd2; color: #c62828; }
        .mode-status { background: #9e9e9e; color: white; padding: 10px; border-radius: 5px; }
        .auto-mode { background: #4caf50; }
        .manual-mode { background: #ff9800; }
        .sensor-value { font-size: 18px; font-weight: bold; color: #1976d2; }
        .warning { background: #fff3e0; padding: 10px; border-radius: 5px; margin: 10px 0; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌱 Smart Agriculture System</h1>
        
        <div class="card sensor-data">
            <h2>📊 Live Sensor Data</h2>
            <div id="sensorData">
                <div>Temperature: <span class="sensor-value" id="tempValue">0</span>°C</div>
                <div>Humidity: <span class="sensor-value" id="humValue">0</span>%</div>
                <div>Light: <span class="sensor-value" id="lightValue">0</span>%</div>
            </div>
        </div>
        
        <div class="card controls">
            <h2>⚙️ Control Panel</h2>
            
            <div class="warning" id="modeWarning">
                ⚠️ Please select a mode to start controlling the system
            </div>
            
            <div>
                <strong>Mode Control:</strong><br>
                <button class="auto-btn" onclick="setMode('automatic')">AUTOMATIC MODE</button>
                <button class="manual-btn" onclick="setMode('manual')">MANUAL MODE</button>
                <br>
                <div class="status mode-status" id="modeStatus">Current Mode: NOT SELECTED</div>
            </div>
            
            <div id="manualControls" style="display:none; margin-top: 15px;">
                <strong>Manual Controls:</strong><br>
                <div style="margin: 10px 0;">
                    Water Pump: 
                    <button class="on-btn" onclick="controlDevice('pump', 'ON')">PUMP ON</button>
                    <button class="off-btn" onclick="controlDevice('pump', 'OFF')">PUMP OFF</button>
                </div>
                <div style="margin: 10px 0;">
                    LED Light: 
                    <button class="on-btn" onclick="controlDevice('light', 'ON')">LIGHT ON</button>
                    <button class="off-btn" onclick="controlDevice('light', 'OFF')">LIGHT OFF</button>
                </div>
            </div>
            
            <div id="autoInfo" style="display:none; background: #e8f5e8; padding: 10px; border-radius: 5px; margin-top: 15px;">
                <strong>Automatic Mode Active:</strong><br>
                • Pump will turn ON when Temperature ≥ 32°C<br>
                • Light will turn ON when Light ≤ 50%
            </div>
            
            <div style="margin-top: 15px;">
                <strong>Device Status:</strong><br>
                <div style="margin: 5px 0;">
                    Pump: <span id="pumpStatus" class="status off-status">OFF</span>
                </div>
                <div style="margin: 5px 0;">
                    Light: <span id="lightStatus" class="status off-status">OFF</span>
                </div>
            </div>
        </div>
    </div>

    <script>
        function setMode(newMode) {
            fetch('/control?mode=' + newMode)
                .then(response => response.text())
                .then(data => {
                    updateUI();
                    showMessage("Mode changed to " + newMode.toUpperCase());
                });
        }

        function controlDevice(device, action) {
            fetch('/control?' + device + '=' + action)
                .then(response => response.text())
                .then(data => {
                    updateUI();
                    showMessage(device.toUpperCase() + " turned " + action);
                });
        }

        function updateUI() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    // Update sensor data
                    document.getElementById('tempValue').textContent = data.temperature;
                    document.getElementById('humValue').textContent = data.humidity;
                    document.getElementById('lightValue').textContent = data.light;
                    
                    // Update mode display
                    const modeStatus = document.getElementById('modeStatus');
                    const modeWarning = document.getElementById('modeWarning');
                    const autoInfo = document.getElementById('autoInfo');
                    
                    if(data.mode === 'none') {
                        modeStatus.textContent = 'Current Mode: NOT SELECTED';
                        modeStatus.className = 'status mode-status';
                        modeWarning.style.display = 'block';
                        autoInfo.style.display = 'none';
                    } else {
                        modeStatus.textContent = 'Current Mode: ' + data.mode.toUpperCase();
                        modeStatus.className = data.mode === 'automatic' ? 
                            'status mode-status auto-mode' : 'status mode-status manual-mode';
                        modeWarning.style.display = 'none';
                        autoInfo.style.display = data.mode === 'automatic' ? 'block' : 'none';
                    }
                    
                    // Show/hide manual controls
                    document.getElementById('manualControls').style.display = 
                        data.mode === 'manual' ? 'block' : 'none';
                    
                    // Update device status
                    document.getElementById('pumpStatus').textContent = data.pumpState ? 'ON' : 'OFF';
                    document.getElementById('pumpStatus').className = data.pumpState ? 
                        'status on-status' : 'status off-status';
                    
                    document.getElementById('lightStatus').textContent = data.lightState ? 'ON' : 'OFF';
                    document.getElementById('lightStatus').className = data.lightState ? 
                        'status on-status' : 'status off-status';
                });
        }

        function showMessage(message) {
            // Simple message display
            console.log(message);
        }

        // Update data every 3 seconds
        setInterval(updateUI, 3000);
        // Initial update
        updateUI();
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Initialize all motor control pins
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Start with everything OFF
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  
  // Set motor directions
  digitalWrite(IN1, HIGH); // Pump forward
  digitalWrite(IN2, LOW);

  Serial.println("Smart Agriculture System Starting...");
  Serial.println("Initial State: NO MODE SELECTED - ALL DEVICES OFF");
  Serial.println("OUT1/2: Water Pump Motor");
  Serial.println("OUT3/4: 12V LED Bulb");
  Serial.println("Web Server Control Interface");
  Serial.println("Please select a mode from the web interface to begin");

  // Connect to WiFi
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  Serial.print("Connecting to WiFi");
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to WiFi!");
  } else {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }

  // Setup web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/control", HTTP_GET, handleControl);
  server.on("/data", HTTP_GET, handleData);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  yield();

  // Read sensors every 3 seconds
  static unsigned long lastSensorTime = 0;
  if(millis() - lastSensorTime > 3000){
    lastSensorTime = millis();

    // Read sensor data
    currentTemp = dht.readTemperature();
    currentHum = dht.readHumidity();
    int ldrRaw = analogRead(LDR_PIN);
    currentLight = map(ldrRaw, 0, 1023, 0, 100);

    // Handle NaN values
    if(isnan(currentTemp)) currentTemp = 0;
    if(isnan(currentHum)) currentHum = 0;

    Serial.printf("Temp: %.1f°C, Hum: %.1f%%, Light: %d%%, Mode: %s", 
                  currentTemp, currentHum, currentLight, mode.c_str());
    Serial.printf(", Pump: %s, Light: %s\n", 
                  pumpState ? "ON" : "OFF", lightState ? "ON" : "OFF");

    // Apply automatic mode logic only if in automatic mode
    if(mode == "automatic"){
      applyAutomaticMode();
    }
  }
}

// ---------------- Web Server Handlers ----------------
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleControl() {
  String response = "OK";
  
  // Handle mode change
  if(server.hasArg("mode")) {
    String newMode = server.arg("mode");
    if(newMode == "automatic" || newMode == "manual") {
      mode = newMode;
      Serial.print("Mode changed to: ");
      Serial.println(mode);
      
      // If switching to automatic mode, apply the logic immediately
      if(mode == "automatic") {
        applyAutomaticMode();
      }
      // If switching to manual mode, ensure devices are off initially
      else if(mode == "manual") {
        setPump(false);
        setLight(false);
      }
    }
  }
  
  // Handle manual controls (only in manual mode)
  if(mode == "manual") {
    if(server.hasArg("pump")) {
      String pumpCmd = server.arg("pump");
      setPump(pumpCmd == "ON");
      Serial.println("Manual Pump Control: " + pumpCmd);
    }
    if(server.hasArg("light")) {
      String lightCmd = server.arg("light");
      setLight(lightCmd == "ON");
      Serial.println("Manual Light Control: " + lightCmd);
    }
  }
  
  server.send(200, "text/plain", response);
}

void handleData() {
  // Create JSON response with current data
  String json = "{";
  json += "\"temperature\":" + String(currentTemp, 1) + ",";
  json += "\"humidity\":" + String(currentHum, 1) + ",";
  json += "\"light\":" + String(currentLight) + ",";
  json += "\"mode\":\"" + mode + "\",";
  json += "\"pumpState\":" + String(pumpState ? "true" : "false") + ",";
  json += "\"lightState\":" + String(lightState ? "true" : "false");
  json += "}";
  
  server.send(200, "application/json", json);
}

// ---------------- Automatic Mode Logic ----------------
void applyAutomaticMode() {
  bool newPumpState = false;
  bool newLightState = false;

  // Automatic Logic: Pump ON when temperature >= 32°C
  if(currentTemp >= TEMP_THRESHOLD) {
    newPumpState = true;
  } else {
    newPumpState = false;
  }

  // Automatic Logic: Light ON when light <= 50%
  if(currentLight <= LIGHT_THRESHOLD) {
    newLightState = true;
  } else {
    newLightState = false;
  }

  // Apply the new states
  setPump(newPumpState);
  setLight(newLightState);
}

void setPump(bool state){
  if (pumpState != state) {
    pumpState = state;
    if(state){
      analogWrite(ENA, pumpSpeedPWM);
      Serial.println("Pump: ON (Full speed)");
    } else {
      analogWrite(ENA, 0);
      Serial.println("Pump: OFF");
    }
  }
}

void setLight(bool state){
  if (lightState != state) {
    lightState = state;
    if(state){
      analogWrite(ENB, ledBrightness);
      Serial.println("LED: ON (Full brightness)");
    } else {
      analogWrite(ENB, 0);
      Serial.println("LED: OFF");
    }
  }
}