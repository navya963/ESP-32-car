// Transmitter (Controller) - improved
#include <esp_now.h>
#include <WiFi.h>

// !!! Replace with actual MAC Address of your Receiver (Car) ESP32 !!!
uint8_t receiverAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


// Pin Definitions
const int xAxisPin = 34; // Analog pin for X-axis (Steering)
const int yAxisPin = 35; // Analog pin for Y-axis (Throttle)
const int switchPin = 2; // Digital pin for the joystick switch (Mode Toggle)

// Dead Zone Setting
const int DEAD_ZONE = 20; // Joystick is centered if input is within +/- 20 of 127

typedef struct PacketData {
  byte xAxisValue;
  byte yAxisValue;
  byte switchPressed; // 0 or 1
} PacketData;

PacketData transmitterData;

unsigned long lastTransmissionTime = 0;
const long interval = 50; // Transmit every 50 ms
bool switchState = false; 
bool lastSwitchState = false;

// --- Callback ---
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    //Serial.println("Send Success");
  } else {
    Serial.println("Send Fail");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(switchPin, INPUT_PULLUP); // Use pullup for switch

  WiFi.mode(WIFI_STA);
  Serial.print("Transmitter MAC Address: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_send_cb(OnDataSent);

  // Register peer (receiver)
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0; 
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    // if you get this, check MAC address format and WiFi mode
  } else {
    Serial.println("Peer added");
  }
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastTransmissionTime > interval) {
    int rawX = analogRead(xAxisPin); 
    int rawY = analogRead(yAxisPin); 
    
    int switchStateRaw = digitalRead(switchPin);
    switchState = (switchStateRaw == LOW); // true if pressed

    // map 0-4095 to 0-254
    int mappedX = map(rawX, 0, 4095, 0, 254);
    int mappedY = map(rawY, 0, 4095, 0, 254);

    // apply dead zone
    if (mappedX > (127 - DEAD_ZONE) && mappedX < (127 + DEAD_ZONE)) {
      transmitterData.xAxisValue = 127; 
    } else {
      transmitterData.xAxisValue = (byte)mappedX;
    }

    if (mappedY > (127 - DEAD_ZONE) && mappedY < (127 + DEAD_ZONE)) {
      transmitterData.yAxisValue = 127; 
    } else {
      transmitterData.yAxisValue = (byte)mappedY;
    }
    
    // Edge detection
    if (switchState == true && lastSwitchState == false) {
      transmitterData.switchPressed = 1;
    } else {
      transmitterData.switchPressed = 0;
    }
    lastSwitchState = switchState;
    
    // Send packet
    esp_err_t res = esp_now_send(receiverAddress, (uint8_t *) &transmitterData, sizeof(transmitterData));
    if (res != ESP_OK) {
      Serial.print("esp_now_send failed: ");
      Serial.println(res);
    }

    lastTransmissionTime = currentTime;
  }
}
