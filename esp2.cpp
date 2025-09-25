#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

// WiFi credentials
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";

// Motor control pins
#define IN1 D1  // Right motor forward
#define IN2 D2  // Right motor backward
#define IN3 D3  // Left motor forward
#define IN4 D4  // Left motor backward
#define ENA D5  // Right motor speed (PWM)
#define ENB D6  // Left motor speed (PWM)

// Motor speed (0-255)
int motorSpeed = 200;

ESP8266WebServer server(80);

// HTML page for car control
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Web Controlled Car</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            margin: 0;
            padding: 20px;
            background-color: #f0f0f0;
        }
        .container {
            max-width: 400px;
            margin: 0 auto;
            background-color: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
        }
        .control-btn {
            width: 80px;
            height: 80px;
            font-size: 24px;
            margin: 5px;
            border: none;
            border-radius: 50%;
            background-color: #4CAF50;
            color: white;
            cursor: pointer;
        }
        .control-btn:active {
            background-color: #45a049;
        }
        .stop-btn {
            background-color: #f44336;
        }
        .stop-btn:active {
            background-color: #da190b;
        }
        .speed-control {
            margin: 20px 0;
        }
        .speed-slider {
            width: 100%;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Web Controlled Car</h1>
        
        <div class="speed-control">
            <label for="speed">Speed: <span id="speedValue">%SPEED%</span>%</label>
            <input type="range" id="speed" min="0" max="100" value="%SPEED%" class="speed-slider">
        </div>
        
        <div style="margin: 20px 0;">
            <button class="control-btn" onclick="moveCar('forward')">↑</button><br>
            <button class="control-btn" onclick="moveCar('left')">←</button>
            <button class="control-btn stop-btn" onclick="moveCar('stop')">Stop</button>
            <button class="control-btn" onclick="moveCar('right')">→</button><br>
            <button class="control-btn" onclick="moveCar('backward')">↓</button>
        </div>
        
        <div>
            <p>IP Address: %IPADDRESS%</p>
        </div>
    </div>

    <script>
        function moveCar(direction) {
            var xhr = new XMLHttpRequest();
            xhr.open("GET", "/" + direction, true);
            xhr.send();
        }
        
        document.getElementById('speed').addEventListener('input', function() {
            var speed = this.value;
            document.getElementById('speedValue').textContent = speed;
            
            var xhr = new XMLHttpRequest();
            xhr.open("GET", "/speed?value=" + speed, true);
            xhr.send();
        });
    </script>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);
    
    // Initialize motor control pins
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);
    pinMode(ENA, OUTPUT);
    pinMode(ENB, OUTPUT);
    
    // Stop motors initially
    stopMotors();
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Define server routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/forward", HTTP_GET, handleForward);
    server.on("/backward", HTTP_GET, handleBackward);
    server.on("/left", HTTP_GET, handleLeft);
    server.on("/right", HTTP_GET, handleRight);
    server.on("/stop", HTTP_GET, handleStop);
    server.on("/speed", HTTP_GET, handleSpeed);
    
    // Start server
    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    server.handleClient();
}

void handleRoot() {
    String page = htmlPage;
    page.replace("%SPEED%", String(map(motorSpeed, 0, 255, 0, 100)));
    page.replace("%IPADDRESS%", WiFi.localIP().toString());
    server.send(200, "text/html", page);
}

void handleForward() {
    moveForward();
    server.send(200, "text/plain", "Moving Forward");
}

void handleBackward() {
    moveBackward();
    server.send(200, "text/plain", "Moving Backward");
}

void handleLeft() {
    turnLeft();
    server.send(200, "text/plain", "Turning Left");
}

void handleRight() {
    turnRight();
    server.send(200, "text/plain", "Turning Right");
}

void handleStop() {
    stopMotors();
    server.send(200, "text/plain", "Stopped");
}

void handleSpeed() {
    if (server.hasArg("value")) {
        int speedPercent = server.arg("value").toInt();
        motorSpeed = map(speedPercent, 0, 100, 0, 255);
        analogWrite(ENA, motorSpeed);
        analogWrite(ENB, motorSpeed);
        server.send(200, "text/plain", "Speed set to " + String(speedPercent) + "%");
    }
}

// Motor control functions
void moveForward() {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, motorSpeed);
    analogWrite(ENB, motorSpeed);
}

void moveBackward() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    analogWrite(ENA, motorSpeed);
    analogWrite(ENB, motorSpeed);
}

void turnLeft() {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    analogWrite(ENA, motorSpeed);
    analogWrite(ENB, motorSpeed);
}

void turnRight() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, motorSpeed);
    analogWrite(ENB, motorSpeed);
}

void stopMotors() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
}