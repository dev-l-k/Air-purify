#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "DHT.h"

// ---------------- WiFi ----------------
#define WLAN_SSID       "YOUR_WIFI_SSID"
#define WLAN_PASS       "YOUR_WIFI_PASSWORD"

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

// ---------------- Web Server ----------------
ESP8266WebServer server(80);

// ---------------- Variables ----------------
String mode = "off";  // START WITH EVERYTHING OFF
bool pumpState = false;
bool lightState = false;
int pumpSpeedPWM = 1000;    // Pump speed at maximum (1000/1023)
int ledBrightness = 1000;   // LED brightness at maximum when ON (1000/1023)

// Sensor values
float temperature = 0.0;
float humidity = 0.0;
int lightPercent = 0;

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Initialize all motor control pins
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);    // ENB for LED brightness control
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Start with everything OFF
  analogWrite(ENA, 0);     // Pump OFF
  analogWrite(ENB, 0);     // LED OFF
  digitalWrite(IN3, LOW);  // LED direction
  digitalWrite(IN4, LOW);  // LED direction
  
  // Set motor directions
  digitalWrite(IN1, HIGH); // Pump forward
  digitalWrite(IN2, LOW);

  Serial.println("Smart Agriculture System Starting...");
  Serial.println("Initial Mode: OFF");
  Serial.println("OUT1/2: Water Pump Motor");
  Serial.println("OUT3/4: 12V LED Bulb");

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
  server.on("/", handleRoot);
  server.on("/setMode", handleSetMode);
  server.on("/setPump", handleSetPump);
  server.on("/setLight", handleSetLight);
  server.on("/getSensorData", handleGetSensorData);
  
  server.begin();
  Serial.println("Web server started!");
  Serial.print("Access the control panel at: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();
  yield();

  // Read sensors every 2 seconds
  static unsigned long lastSensorTime = 0;
  if(millis() - lastSensorTime > 2000){
    lastSensorTime = millis();

    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    int ldrRaw = analogRead(LDR_PIN);
    lightPercent = map(ldrRaw, 0, 1023, 0, 100);

    Serial.printf("Temp: %.1f°C, Hum: %.1f%%, Light: %d%%", temperature, humidity, lightPercent);
    Serial.printf(", Mode: %s, Pump: %s, LED: %s\n", mode.c_str(), 
                  pumpState ? "ON" : "OFF", lightState ? "ON" : "OFF");

    // Apply automatic mode logic
    if(mode == "automatic"){
      applyAutomaticMode();
    }
  }
}

// ---------------- Web Server Handlers ----------------
void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Smart Agriculture System</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 20px; 
            background-color: #f0f8ff;
        }
        .container { 
            max-width: 600px; 
            margin: 0 auto; 
            background: white; 
            padding: 20px; 
            border-radius: 10px; 
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        h1 { 
            text-align: center; 
            color: #2e7d32; 
        }
        .sensor-panel {
            background: #e8f5e8;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
        }
        .sensor-value {
            font-size: 24px;
            font-weight: bold;
            color: #1976d2;
            display: inline-block;
            margin: 10px 20px;
        }
        .control-panel {
            background: #fff3e0;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
        }
        .mode-buttons {
            margin-bottom: 20px;
        }
        button {
            padding: 12px 24px;
            margin: 5px;
            border: none;
            border-radius: 6px;
            cursor: pointer;
            font-size: 16px;
            font-weight: bold;
        }
        .mode-btn {
            background-color: #2196f3;
            color: white;
        }
        .mode-btn.active {
            background-color: #4caf50;
        }
        .control-btn {
            background-color: #ff9800;
            color: white;
        }
        .control-btn.on {
            background-color: #4caf50;
        }
        .control-btn.off {
            background-color: #f44336;
        }
        .status {
            text-align: center;
            margin: 10px 0;
            font-weight: bold;
        }
        .manual-controls {
            display: none;
        }
        .manual-controls.show {
            display: block;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌱 Smart Agriculture System</h1>
        
        <div class="sensor-panel">
            <h3>📊 Sensor Readings</h3>
            <div class="sensor-value">🌡️ <span id="temperature">--</span>°C</div>
            <div class="sensor-value">💧 <span id="humidity">--</span>%</div>
            <div class="sensor-value">☀️ <span id="light">--</span>%</div>
        </div>

        <div class="control-panel">
            <h3>⚙️ Control Panel</h3>
            
            <div class="mode-buttons">
                <h4>Mode Selection:</h4>
                <button class="mode-btn" id="offBtn" onclick="setMode('off')">⏹️ Off</button>
                <button class="mode-btn" id="autoBtn" onclick="setMode('automatic')">🤖 Automatic</button>
                <button class="mode-btn active" id="manualBtn" onclick="setMode('manual')">👤 Manual</button>
            </div>
            
            <div class="status">
                Current Mode: <span id="currentMode">Off</span>
            </div>
            
            <div class="manual-controls" id="manualControls">
                <h4>Manual Controls:</h4>
                <button class="control-btn off" id="pumpBtn" onclick="togglePump()">
                    💧 Pump: <span id="pumpStatus">OFF</span>
                </button>
                <button class="control-btn off" id="lightBtn" onclick="toggleLight()">
                    💡 Light: <span id="lightStatus">OFF</span>
                </button>
            </div>
            
            <div class="status">
                <div>Pump: <span id="pumpState">OFF</span></div>
                <div>Light: <span id="lightState">OFF</span></div>
            </div>
        </div>
    </div>

    <script>
        let currentMode = 'off';
        let pumpState = false;
        let lightState = false;

        function updateSensorData() {
            fetch('/getSensorData')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('temperature').textContent = data.temperature;
                    document.getElementById('humidity').textContent = data.humidity;
                    document.getElementById('light').textContent = data.lightPercent;
                    
                    // Update device states
                    pumpState = data.pumpState;
                    lightState = data.lightState;
                    updateDeviceStatus();
                })
                .catch(error => console.error('Error:', error));
        }

        function setMode(mode) {
            currentMode = mode;
            fetch('/setMode?mode=' + mode)
                .then(response => response.text())
                .then(data => {
                    document.getElementById('currentMode').textContent = 
                        mode.charAt(0).toUpperCase() + mode.slice(1);
                    
                    // Update button states
                    document.getElementById('offBtn').classList.toggle('active', mode === 'off');
                    document.getElementById('autoBtn').classList.toggle('active', mode === 'automatic');
                    document.getElementById('manualBtn').classList.toggle('active', mode === 'manual');
                    
                    // Show/hide manual controls
                    document.getElementById('manualControls').classList.toggle('show', mode === 'manual');
                });
        }

        function togglePump() {
            if (currentMode === 'manual') {
                const newState = !pumpState;
                fetch('/setPump?state=' + (newState ? 'ON' : 'OFF'))
                    .then(response => response.text())
                    .then(data => {
                        pumpState = newState;
                        updateDeviceStatus();
                    });
            }
        }

        function toggleLight() {
            if (currentMode === 'manual') {
                const newState = !lightState;
                fetch('/setLight?state=' + (newState ? 'ON' : 'OFF'))
                    .then(response => response.text())
                    .then(data => {
                        lightState = newState;
                        updateDeviceStatus();
                    });
            }
        }

        function updateDeviceStatus() {
            // Update main status display
            document.getElementById('pumpState').textContent = pumpState ? 'ON' : 'OFF';
            document.getElementById('lightState').textContent = lightState ? 'ON' : 'OFF';
            
            // Update manual control buttons
            const pumpBtn = document.getElementById('pumpBtn');
            const lightBtn = document.getElementById('lightBtn');
            
            pumpBtn.className = 'control-btn ' + (pumpState ? 'on' : 'off');
            pumpBtn.innerHTML = '💧 Pump: <span id="pumpStatus">' + (pumpState ? 'ON' : 'OFF') + '</span>';
            
            lightBtn.className = 'control-btn ' + (lightState ? 'on' : 'off');
            lightBtn.innerHTML = '💡 Light: <span id="lightStatus">' + (lightState ? 'ON' : 'OFF') + '</span>';
        }

        // Update sensor data every 2 seconds
        setInterval(updateSensorData, 2000);
        updateSensorData(); // Initial call
    </script>
</body>
</html>
)";
  
  server.send(200, "text/html", html);
}

void handleSetMode() {
  if (server.hasArg("mode")) {
    String newMode = server.arg("mode");
    if (newMode == "automatic" || newMode == "manual" || newMode == "off") {
      mode = newMode;
      Serial.print("Mode changed to: ");
      Serial.println(mode);
      
      // Turn everything OFF when switching to off mode
      if (mode == "off") {
        setPump(false);
        setLight(false);
      }
      // Apply automatic logic immediately when switching to auto mode
      else if (mode == "automatic") {
        applyAutomaticMode();
      }
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleSetPump() {
  if (mode == "manual" && server.hasArg("state")) {
    String state = server.arg("state");
    setPump(state == "ON");
  }
  server.send(200, "text/plain", "OK");
}

void handleSetLight() {
  if (mode == "manual" && server.hasArg("state")) {
    String state = server.arg("state");
    setLight(state == "ON");
  }
  server.send(200, "text/plain", "OK");
}

void handleGetSensorData() {
  String json = "{";
  json += "\"temperature\":" + String(isnan(temperature) ? 0 : temperature, 1) + ",";
  json += "\"humidity\":" + String(isnan(humidity) ? 0 : humidity, 1) + ",";
  json += "\"lightPercent\":" + String(lightPercent) + ",";
  json += "\"pumpState\":" + String(pumpState ? "true" : "false") + ",";
  json += "\"lightState\":" + String(lightState ? "true" : "false") + ",";
  json += "\"mode\":\"" + mode + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}

// ---------------- Functions ----------------
void applyAutomaticMode() {
  // Pump control - turn ON pump if temperature >= 32°C
  if(!isnan(temperature) && temperature >= TEMP_THRESHOLD){ 
    setPump(true); 
  } else { 
    setPump(false); 
  }
  
  // Light control - turn ON LED if light is low
  if(lightPercent < LIGHT_THRESHOLD){ 
    setLight(true); 
  } else { 
    setLight(false); 
  }
}

void setPump(bool state){
  if (pumpState != state) {
    pumpState = state;
    if(state){
      analogWrite(ENA, pumpSpeedPWM); // Pump at full speed
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
      analogWrite(ENB, ledBrightness); // LED at full brightness
      Serial.println("LED: ON (Full brightness)");
    } else {
      analogWrite(ENB, 0);
      Serial.println("LED: OFF");
    }
  }
}