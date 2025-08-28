#include <DHT.h>

// Define DHT sensor
#define DHTPIN 7         // DHT11 data pin connected to digital pin 7
#define DHTTYPE DHT11    // DHT 11

DHT dht(DHTPIN, DHTTYPE);

// Motor 1 (Always ON)
const int EN1 = 9;
const int IN1 = 2;
const int IN2 = 3;

// Motor 2 (Temperature controlled)
const int EN2 = 10;
const int IN3 = 4;
const int IN4 = 5;

void setup() {
  Serial.begin(9600);
  dht.begin();

  // Set motor pins
  pinMode(EN1, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(EN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Start Motor 1
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(EN1, 200);  // Adjust speed (0–255)
}

void loop() {
  float temperatureC = dht.readTemperature();  // Read temperature in °C

  if (isnan(temperatureC)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(temperatureC);
  Serial.println(" °C");

  if (temperatureC > 30.0) {
    // Start Motor 2
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(EN2, 180);  // Speed of Motor 2
  } else {
    // Stop Motor 2
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(EN2, 0);
  }

  delay(1000);  // Delay between readings
}
