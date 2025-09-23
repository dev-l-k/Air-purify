#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "DHT.h"

// ---------------- WiFi ----------------
#define WLAN_SSID   "YOUR_WIFI_SSID"
#define WLAN_PASS   "YOUR_WIFI_PASSWORD"

// ---------------- Sensors ----------------
#define DHTPIN D1
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define LDR_PIN A0
#define LIGHT_THRESHOLD 50    // Light % below this → LED ON
#define TEMP_THRESHOLD 32.0   // Temp < 32°C → Pump ON

// ---------------- L298N Pins ----------------
#define ENA D5
#define IN1 D6
#define IN2 D2
#define ENB D7
#define IN3 D0
#define IN4 D8

// ---------------- Webserver ----------------
ESP8266WebServer server(80);

// ---------------- Variables ----------------
String mode = "automatic";  
bool pumpState = false;
bool lightState = false;
int pumpSpeedPWM = 1000;
int ledBrightness = 1000;

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // OFF initially
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  // WiFi
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());

  // Routes
  server.on("/", handleRoot);
  server.on("/mode", handleMode);
  server.on("/pump", handlePump);
  server.on("/light", handleLight);

  server.begin();
  Serial.println("Webserver started");
}

void loop() {
  server.handleClient();

  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 5000) { // every 5s
    lastUpdate = millis();
    if (mode == "automatic") applyAutomaticMode();
  }
}

// ---------------- Webpage ----------------
void handleRoot() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int ldrRaw = analogRead(LDR_PIN);
  int lightPercent = map(ldrRaw, 0, 1023, 0, 100);

  String page = "<html><head><title>Smart Agriculture</title>";
  page += "<meta http-equiv='refresh' content='5'></head><body>";  // auto-refresh every 5s
  page += "<h2>Smart Agriculture Control</h2>";
  page += "<p><b>Mode:</b> " + mode + "</p>";
  page += "<p>Temperature: " + String(temp) + " °C</p>";
  page += "<p>Humidity: " + String(hum) + " %</p>";
  page += "<p>Light: " + String(lightPercent) + " %</p>";

  // Mode toggle
  page += "<p><a href='/mode?set=automatic'><button>Automatic Mode</button></a> ";
  page += "<a href='/mode?set=manual'><button>Manual Mode</button></a></p>";

  if (mode == "manual") {
    page += "<h3>Manual Controls</h3>";
    page += "<p>Pump (Now " + String(pumpState ? "ON" : "OFF") + "): ";
    page += "<a href='/pump?set=ON'><button>ON</button></a> ";
    page += "<a href='/pump?set=OFF'><button>OFF</button></a></p>";

    page += "<p>Light (Now " + String(lightState ? "ON" : "OFF") + "): ";
    page += "<a href='/light?set=ON'><button>ON</button></a> ";
    page += "<a href='/light?set=OFF'><button>OFF</button></a></p>";
  }

  page += "</body></html>";
  server.send(200, "text/html", page);
}

// ---------------- Handlers ----------------
void handleMode() {
  if (server.hasArg("set")) {
    String newMode = server.arg("set");
    if (newMode == "automatic" || newMode == "manual") {
      mode = newMode;
      if (mode == "automatic") applyAutomaticMode();
    }
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handlePump() {
  if (mode == "manual" && server.hasArg("set")) {
    setPump(server.arg("set") == "ON");
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleLight() {
  if (mode == "manual" && server.hasArg("set")) {
    setLight(server.arg("set") == "ON");
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

// ---------------- Logic ----------------
void applyAutomaticMode() {
  float temp = dht.readTemperature();
  int ldrRaw = analogRead(LDR_PIN);
  int lightPercent = map(ldrRaw, 0, 1023, 0, 100);

  if (!isnan(temp) && temp < TEMP_THRESHOLD) setPump(true);
  else setPump(false);

  if (lightPercent < LIGHT_THRESHOLD) setLight(true);
  else setLight(false);
}

void setPump(bool state) {
  pumpState = state;
  analogWrite(ENA, state ? pumpSpeedPWM : 0);
  Serial.println(state ? "Pump: ON" : "Pump: OFF");
}

void setLight(bool state) {
  lightState = state;
  analogWrite(ENB, state ? ledBrightness : 0);
  Serial.println(state ? "LED: ON" : "LED: OFF");
}