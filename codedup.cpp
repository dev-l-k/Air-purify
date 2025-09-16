#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "DHT.h"

// ---------------- WiFi ----------------
#define WLAN_SSID       "YOUR_WIFI_SSID"
#define WLAN_PASS       "YOUR_WIFI_PASSWORD"

// ---------------- Adafruit IO ----------------
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "YOUR_ADAFRUIT_USERNAME"
#define AIO_KEY         "YOUR_ADAFRUIT_KEY"

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

// ---------------- MQTT ----------------
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Subscriptions (App → NodeMCU)
Adafruit_MQTT_Subscribe modeFeed        = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/mode");
Adafruit_MQTT_Subscribe pumpControlFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/pumpControl");
Adafruit_MQTT_Subscribe lightControlFeed= Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/lightControl");

// Publishes (NodeMCU → App) - ONLY SENSOR DATA
Adafruit_MQTT_Publish tempPub      = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");
Adafruit_MQTT_Publish humPub       = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");
Adafruit_MQTT_Publish lightPub     = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/lightPercent");

// ---------------- Variables ----------------
String mode = "automatic";  // START IN AUTOMATIC MODE
bool pumpState = false;
bool lightState = false;
int pumpSpeedPWM = 1000;    // Pump speed at maximum (1000/1023)
int ledBrightness = 1000;   // LED brightness at maximum when ON (1000/1023)

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
  Serial.println("Initial Mode: AUTOMATIC");
  Serial.println("OUT1/2: Water Pump Motor");
  Serial.println("OUT3/4: 12V LED Bulb");
  Serial.println("Publishing: Temperature, Humidity, Light% only");

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

  // Subscribe to MQTT feeds
  mqtt.subscribe(&modeFeed);
  mqtt.subscribe(&pumpControlFeed);
  mqtt.subscribe(&lightControlFeed);
}

void loop() {
  MQTT_connect();
  yield();

  // Handle incoming MQTT messages
  Adafruit_MQTT_Subscribe *sub;
  while ((sub = mqtt.readSubscription(100))) {
    if(sub == &modeFeed) {
      String newMode = String((char *)modeFeed.lastread);
      if (newMode == "automatic" || newMode == "manual") {
        mode = newMode;
        Serial.print("Mode changed to: ");
        Serial.println(mode);
        
        // Apply automatic logic immediately when switching to auto mode
        if (mode == "automatic") {
          applyAutomaticMode();
        }
      }
    }
    
    // Only process manual controls if in manual mode
    if(mode == "manual") {
      if(sub == &pumpControlFeed) {
        String val = String((char *)pumpControlFeed.lastread);
        setPump(val == "ON");
      }
      if(sub == &lightControlFeed) {
        String val = String((char *)lightControlFeed.lastread);
        setLight(val == "ON");
      }
    }
  }

  // Read sensors and apply automatic logic every 5 seconds
  static unsigned long lastSensorTime = 0;
  if(millis() - lastSensorTime > 5000){
    lastSensorTime = millis();

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    int ldrRaw = analogRead(LDR_PIN);
    int lightPercent = map(ldrRaw, 0, 1023, 0, 100);

    // ALWAYS PUBLISH SENSOR DATA regardless of mode
    if(!isnan(temp)) tempPub.publish(temp);
    if(!isnan(hum)) humPub.publish(hum);
    lightPub.publish(lightPercent);

    Serial.printf("Temp: %.1f°C, Hum: %.1f%%, Light: %d%%", temp, hum, lightPercent);
    Serial.printf(", Mode: %s\n", mode.c_str());

    // Apply automatic mode logic
    if(mode == "automatic"){
      applyAutomaticMode();
    }
  }
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
      analogWrite(ENA, pumpSpeedPWM); // Pump at full speed
      Serial.println("Pump: ON (Full speed)");
    } else {
      analogWrite(ENA, 0);
      Serial.println("Pump: OFF");
    }
    // NO STATE PUBLISHING
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
    // NO STATE PUBLISHING
  }
}

void MQTT_connect() {
  int8_t ret;
  if(mqtt.connected()) return;
  
  Serial.print("Connecting to MQTT... ");
  while((ret = mqtt.connect()) != 0){
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying in 5 seconds...");
    mqtt.disconnect();
    delay(5000);
  }
  Serial.println("MQTT Connected!");
}