#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// --- L298N PIN DEFINITIONS (Must match wiring) ---
// Right motor
int enableRightMotor = 22; // ENA (Speed/Enable)
int rightMotorPin1 = 16;   // IN3 (Labeled RX2)
int rightMotorPin2 = 17;   // IN4 (Labeled TX2)

// Left motor
int enableLeftMotor = 23;  // ENB (Speed/Enable)
int leftMotorPin1 = 18;    // IN1 
int leftMotorPin2 = 19;    // IN2

#define MAX_MOTOR_SPEED 240 // Max speed for PWM (0-255)

const int PWMFreq = 1000;
const int PWMResolution = 8;
const int rightMotorPWMSpeedChannel = 4;
const int leftMotorPWMSpeedChannel = 5;

#define SIGNAL_TIMEOUT 1000 // Stops motors if no signal after 1 second
unsigned long lastRecvTime = 0;

// --- Data structure to receive (Must match the transmitter) ---
typedef struct PacketData {
    byte xAxisValue;
    byte yAxisValue;
    byte switchPressed;
} PacketData;

PacketData receiverData;

bool throttleAndSteeringMode = false;

// --- Function Prototypes ---
void rotateMotor(int rightMotorSpeed, int leftMotorSpeed);
void simpleMovements();
void throttleAndSteeringMovements();

// --- ESP-NOW CALLBACK ---
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
    if (len == 0) return;
    
    memcpy(&receiverData, incomingData, sizeof(receiverData));

    // Toggle mode only if the switch was pressed (1)
    if (receiverData.switchPressed == 1) {
        throttleAndSteeringMode = !throttleAndSteeringMode;
        // Serial prints are optional but useful for debugging
        Serial.print("Mode Toggled! Current Mode: ");
        Serial.println(throttleAndSteeringMode ? "Throttle & Steering" : "Simple 4-Way");
    }

    // Choose control mode
    if (throttleAndSteeringMode) {
        throttleAndSteeringMovements();
    } else {
        simpleMovements();
    }

    lastRecvTime = millis();  
}

// --- Movement Functions ---

void simpleMovements() {
    if (receiverData.yAxisValue <= 75) {      // Forward (Joystick pushed up)
        rotateMotor(MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
    } else if (receiverData.yAxisValue >= 178) { // Backward (Joystick pulled down)
        rotateMotor(-MAX_MOTOR_SPEED, -MAX_MOTOR_SPEED);
    } else if (receiverData.xAxisValue >= 172) { // Right
        rotateMotor(MAX_MOTOR_SPEED, -MAX_MOTOR_SPEED);
    } else if (receiverData.xAxisValue <= 75) {  // Left
        rotateMotor(-MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
    } else {                                // Stop (Joystick centered)
        rotateMotor(0, 0);
    }  
}

void throttleAndSteeringMovements() {
    // Map Y-axis (throttle) from 254 (down) -> 0 (up) to -255 (rev) -> 255 (fwd)
    int throttle = map(receiverData.yAxisValue, 254, 0, -255, 255);
    // Map X-axis (steering) from 0 (left) -> 254 (right) to -255 (left) -> 255 (right)
    int steering = map(receiverData.xAxisValue, 0, 254, -255, 255);  
    int motorDirection = 1; // 1 for forward, -1 for backward
    
    if (throttle < 0) {
        motorDirection = -1;  // backward
    }

    // Calculate motor speeds based on throttle and steering mix
    int rightMotorSpeed = abs(throttle) - steering;
    int leftMotorSpeed  = abs(throttle) + steering;

    // Ensure speeds are within the 0-255 range
    rightMotorSpeed = constrain(rightMotorSpeed, 0, 255);
    leftMotorSpeed  = constrain(leftMotorSpeed, 0, 255);

    // Apply speed and direction
    rotateMotor(rightMotorSpeed * motorDirection, leftMotorSpeed * motorDirection);
}


void rotateMotor(int rightMotorSpeed, int leftMotorSpeed) {
    // --- Right motor direction ---
    if (rightMotorSpeed < 0) { // BACKWARD
        digitalWrite(rightMotorPin1, LOW);
        digitalWrite(rightMotorPin2, HIGH);    
    } else if (rightMotorSpeed > 0) { // FORWARD
        digitalWrite(rightMotorPin1, HIGH);
        digitalWrite(rightMotorPin2, LOW);      
    } else {
        digitalWrite(rightMotorPin1, LOW);
        digitalWrite(rightMotorPin2, LOW);      
    }
    
    // --- Left motor direction ---
    if (leftMotorSpeed < 0) { // BACKWARD
        digitalWrite(leftMotorPin1, LOW);
        digitalWrite(leftMotorPin2, HIGH);    
    } else if (leftMotorSpeed > 0) { // FORWARD
        digitalWrite(leftMotorPin1, HIGH);
        digitalWrite(leftMotorPin2, LOW);      
    } else {
        digitalWrite(leftMotorPin1, LOW);
        digitalWrite(leftMotorPin2, LOW);      
    } 

    // Apply speed via PWM
    ledcWrite(rightMotorPWMSpeedChannel, abs(rightMotorSpeed));
    ledcWrite(leftMotorPWMSpeedChannel, abs(leftMotorSpeed));    
}


void setUpPinModes() {
    pinMode(enableRightMotor, OUTPUT);
    pinMode(rightMotorPin1, OUTPUT);
    pinMode(rightMotorPin2, OUTPUT);
    
    pinMode(enableLeftMotor, OUTPUT);
    pinMode(leftMotorPin1, OUTPUT);
    pinMode(leftMotorPin2, OUTPUT);

    // PWM setup
    ledcSetup(rightMotorPWMSpeedChannel, PWMFreq, PWMResolution);
    ledcSetup(leftMotorPWMSpeedChannel, PWMFreq, PWMResolution);  
    ledcAttachPin(enableRightMotor, rightMotorPWMSpeedChannel);
    ledcAttachPin(enableLeftMotor, leftMotorPWMSpeedChannel); 
    
    rotateMotor(0, 0);
}

// --- Setup ---
void setup() {
    setUpPinModes();
    
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    Serial.print("Receiver MAC: ");
    Serial.println(WiFi.macAddress());

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Register the callback function to receive data
    esp_now_register_recv_cb(OnDataRecv);
}
  
// --- Main Loop ---
void loop() {
    // Check for signal timeout and stop motors if signal is lost
    unsigned long now = millis();
    if (now - lastRecvTime > SIGNAL_TIMEOUT) {
        rotateMotor(0, 0);
    }
}

