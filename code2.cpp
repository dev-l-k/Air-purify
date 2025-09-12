// Line Follower Robot with 2 IR Sensors and L298N Motor Driver
// Board: Arduino UNO / Nano

// ====== CONFIGURATION CONSTANTS ======
#define BASE_SPEED 150    // Base speed for straight movement (0-255)
#define TURN_SPEED 120    // Speed for turning motor (0-255)
#define SMOOTH_TURN 180   // Speed for outer wheel during smooth turns (0-255)
#define LOOP_DELAY 10     // Delay between loop iterations (ms)

// ====== PIN DEFINITIONS ======
// Motor driver pins (L298N)
int ENA = 9;   // Enable pin for left motor (PWM)
int IN1 = 8;   // Left motor forward
int IN2 = 7;   // Left motor backward
int ENB = 10;  // Enable pin for right motor (PWM)
int IN3 = 6;   // Right motor forward
int IN4 = 5;   // Right motor backward

// IR sensors
int IR_left = 2;   // Left sensor
int IR_right = 3;  // Right sensor

// ====== VARIABLES ======
bool useSmoothTurns = true;  // Set to false for pivot turns, true for smooth turns

void setup() {
  // Motor pins
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // IR sensor pins
  pinMode(IR_left, INPUT);
  pinMode(IR_right, INPUT);

  // Serial communication for debugging
  Serial.begin(9600);
  Serial.println("Line Follower Robot Started");
  Serial.println("Sensor Values: 0=Black/Line, 1=White/Off-line");
  
  stopMotors(); // Start with motors stopped
  
  // Brief startup sequence
  startupBeep();
}

void loop() {
  // Read sensor values
  int left = digitalRead(IR_left);
  int right = digitalRead(IR_right);
  
  // Debug output
  Serial.print("Left: ");
  Serial.print(left);
  Serial.print(" Right: ");
  Serial.println(right);
  
  // Decision logic
  if (left == 0 && right == 0) {
    // Both sensors on black -> Stop or special handling
    Serial.println("Both on BLACK - Stopping");
    stopMotors();
  }
  else if (left == 0 && right == 1) {
    // Left on black, right on white -> Turn left
    Serial.println("Turning LEFT");
    if (useSmoothTurns) {
      turnLeftSmooth();
    } else {
      turnLeft();
    }
  }
  else if (left == 1 && right == 0) {
    // Right on black, left on white -> Turn right
    Serial.println("Turning RIGHT");
    if (useSmoothTurns) {
      turnRightSmooth();
    } else {
      turnRight();
    }
  }
  else if (left == 1 && right == 1) {
    // Both on white -> Move forward
    Serial.println("Moving FORWARD");
    forward();
  }
  
  delay(LOOP_DELAY);
}

// ====== MOTOR CONTROL FUNCTIONS ======
void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, BASE_SPEED);
  analogWrite(ENB, BASE_SPEED);
}

// Original pivot turn (sharp turn)
void turnLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);  // Left motor backward
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);   // Right motor forward
  analogWrite(ENA, TURN_SPEED);
  analogWrite(ENB, TURN_SPEED);
}

// Original pivot turn (sharp turn)
void turnRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);   // Left motor forward
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);  // Right motor backward
  analogWrite(ENA, TURN_SPEED);
  analogWrite(ENB, TURN_SPEED);
}

// Smooth turn (differential speed)
void turnLeftSmooth() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, TURN_SPEED);     // Left motor slower
  analogWrite(ENB, SMOOTH_TURN);    // Right motor faster
}

// Smooth turn (differential speed)
void turnRightSmooth() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, SMOOTH_TURN);    // Left motor faster
  analogWrite(ENB, TURN_SPEED);     // Right motor slower
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// ====== UTILITY FUNCTIONS ======
void startupBeep() {
  // Simple beep using motors
  analogWrite(ENA, 100);
  analogWrite(ENB, 100);
  delay(100);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  delay(100);
  analogWrite(ENA, 100);
  analogWrite(ENB, 100);
  delay(100);
  stopMotors();
}

// ====== SENSOR CALIBRATION HELPER ======
void sensorTest() {
  // Run this function to test your sensor values
  Serial.println("=== SENSOR TEST ===");
  Serial.println("Place sensors on BLACK line and press any key...");
  while(!Serial.available()); // Wait for input
  Serial.read(); // Clear buffer
  
  int blackLeft = digitalRead(IR_left);
  int blackRight = digitalRead(IR_right);
  
  Serial.println("Place sensors on WHITE surface and press any key...");
  while(!Serial.available()); // Wait for input
  Serial.read(); // Clear buffer
  
  int whiteLeft = digitalRead(IR_left);
  int whiteRight = digitalRead(IR_right);
  
  Serial.println("=== TEST RESULTS ===");
  Serial.print("BLACK - Left: "); Serial.print(blackLeft);
  Serial.print(" Right: "); Serial.println(blackRight);
  Serial.print("WHITE - Left: "); Serial.print(whiteLeft);
  Serial.print(" Right: "); Serial.println(whiteRight);
  
  if (blackLeft == 0 && blackRight == 0 && whiteLeft == 1 && whiteRight == 1) {
    Serial.println("✓ Sensor polarity is CORRECT");
  } else {
    Serial.println("✗ Sensor polarity may be REVERSED");
    Serial.println("Swap the sensor values in your code if needed");
  }
}