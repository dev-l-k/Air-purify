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
#define TEMP_THRESHOLD 30.0   // temperature above 30°C → turn ON pump

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
String mode = "automatic";  // START IN AUTOMATIC MODE
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
        button { padding: 10px 15px; margin: 5px; border: none; border-radius: 5px; cursor: pointer; }
        .auto-btn { background: #4caf50; color: white; }
        .manual-btn { background: #ff9800; color: white; }
        .on-btn { background: #2196f3; color: white; }
        .off-btn { background: #f44336; color: white; }
        .status { padding: 8px; border-radius: 4px; margin: 5px 0; }
        .on-status { background: #c8e6c9; color: #2e7d32; }
        .off-status { background: #ffcdd2; color: #c62828; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌱 Smart Agriculture System</h1>
        
        <div class="card sensor-data">
            <h2>📊 Sensor Data</h2>
            <div id="sensorData">Loading...</div>
        </div>
        
        <div class="card controls">
            <h2>⚙️ Control Panel</h2>
            
            <div>
                <strong>Mode Control:</strong><br>
                <button class="auto-btn" onclick="setMode('automatic')">AUTOMATIC</button>
                <button class="manual-btn" onclick="setMode('manual')">MANUAL</button>
                <div class="status" id="modeStatus">Current: AUTOMATIC</div>
            </div>
            
            <div id="manualControls" style="display:none;">
                <strong>Manual Controls:</strong><br>
                <button class="on-btn" onclick="controlDevice('pump', 'ON')">PUMP ON</button>
                <button class="off-btn" onclick="controlDevice('pump', 'OFF')">PUMP OFF</button>
                <br>
                <button class="on-btn" onclick="controlDevice('light', 'ON')">LIGHT ON</button>
                <button class="off-btn" onclick="controlDevice('light', 'OFF')">LIGHT OFF</button>
            </div>
            
            <div>
                <strong>Device Status:</strong><br>
                Pump: <span id="pumpStatus" class="status off-status">OFF</span><br>
                Light: <span id="lightStatus" class="status off-status">OFF</span>
            </div>
        </div>
    </div>

    <script>
        function setMode(mode) {
            fetch('/control?mode=' + mode)
                .then(response => response.text())
                .then(data => {
                    updateUI();
                });
        }

        function controlDevice(device, action) {
            fetch('/control?' + device + '=' + action)
                .then(response => response.text())
                .then(data => {
                    updateUI();
                });
        }

        function updateUI() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    // Update sensor data
                    document.getElementById('sensorData').innerHTML = 
                        'Temperature: ' + data.temperature + '°C<br>' +
                        'Humidity: ' + data.humidity + '%<br>' +
                        'Light: ' + data.light + '%';
                    
                    // Update mode
                    document.getElementById('modeStatus').textContent = 'Current: ' + data.mode.toUpperCase();
                    document.getElementById('modeStatus').className = data.mode === 'automatic' ? 
                        'status on-status' : 'status off-status';
                    
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
  Serial.println("Initial Mode: AUTOMATIC");
  Serial.println("OUT1/2: Water Pump Motor");
  Serial.println("OUT3/4: 12V LED Bulb");
  Serial.println("Web Server Control Interface");

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

  // Read sensors and apply automatic logic every 5 seconds
  static unsigned long lastSensorTime = 0;
  if(millis() - lastSensorTime > 5000){
    lastSensorTime = millis();

    currentTemp = dht.readTemperature();
    currentHum = dht.readHumidity();
    int ldrRaw = analogRead(LDR_PIN);
    currentLight = map(ldrRaw, 0, 1023, 0, 100);

    Serial.printf("Temp: %.1f°C, Hum: %.1f%%, Light: %d%%", currentTemp, currentHum, currentLight);
    Serial.printf(", Mode: %s\n", mode.c_str());

    // Apply automatic mode logic
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
      
      // Apply automatic logic immediately when switching to auto mode
      if(mode == "automatic") {
        applyAutomaticMode();
      }
    }
  }
  
  // Handle manual controls (only in manual mode)
  if(mode == "manual") {
    if(server.hasArg("pump")) {
      String pumpCmd = server.arg("pump");
      setPump(pumpCmd == "ON");
    }
    if(server.hasArg("light")) {
      String lightCmd = server.arg("light");
      setLight(lightCmd == "ON");
    }
  }
  
  server.send(200, "text/plain", response);
}

void handleData() {
  // Create JSON response with current data
  String json = "{";
  json += "\"temperature\":" + String(isnan(currentTemp) ? "0" : String(currentTemp)) + ",";
  json += "\"humidity\":" + String(isnan(currentHum) ? "0" : String(currentHum)) + ",";
  json += "\"light\":" + String(currentLight) + ",";
  json += "\"mode\":\"" + mode + "\",";
  json += "\"pumpState\":" + String(pumpState ? "true" : "false") + ",";
  json += "\"lightState\":" + String(lightState ? "true" : "false");
  json += "}";
  
  server.send(200, "application/json", json);
}

// ---------------- Functions ----------------
void applyAutomaticMode() {
  float temp = dht.readTemperature();
  int ldrRaw = analogRead(LDR_PIN);
  int lightPercent = map(ldrRaw, 0, 1023, 0, 100);

  // Light control - turn ON LED if dark
  if(lightPercent < LIGHT_THRESHOLD){ 
    setLight(true); 
  } else { 
    setLight(false); 
  }

  // Pump control - turn ON pump if hot
  if(!isnan(temp) && temp > TEMP_THRESHOLD){ 
    setPump(true); 
  } else { 
    setPump(false); 
  }
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