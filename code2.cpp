// Line Follower Robot with 2 IR Sensors and L298N Motor Driver
// Board: Arduino UNO / Nano

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

  stopMotors(); // Start with motors stopped
}

void loop() {
  int left = digitalRead(IR_left);
  int right = digitalRead(IR_right);

  if (left == 0 && right == 0) {
    // Both sensors on black -> Stop
    stopMotors();
  }
  else if (left == 0 && right == 1) {
    // Left on black, right on white -> Turn left
    turnLeft();
  }
  else if (left == 1 && right == 0) {
    // Right on black, left on white -> Turn right
    turnRight();
  }
  else if (left == 1 && right == 1) {
    // Both on white -> Move forward
    forward();
  }
}

// ====== Motor control functions ======
void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 150); // Speed 0-255
  analogWrite(ENB, 150);
}

void turnLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);  // Left motor backward
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);   // Right motor forward
  analogWrite(ENA, 120);
  analogWrite(ENB, 150);
}

void turnRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);   // Left motor forward
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);  // Right motor backward
  analogWrite(ENA, 150);
  analogWrite(ENB, 120);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}