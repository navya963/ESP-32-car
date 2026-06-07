#include <WiFi.h>
#include "esp_wifi.h"   // provides esp_wifi_get_mac()

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=== ESP32 MAC-Address Finder ===");

  // Put Wi-Fi in Station mode so we get the STA MAC used by ESP-NOW
  WiFi.mode(WIFI_STA);

  // 1) High-level Arduino call
  Serial.print("WiFi.macAddress(): ");
  Serial.println(WiFi.macAddress());

  // 2) Lower-level call (same value, double-check)
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  Serial.print("esp_wifi_get_mac(WIFI_IF_STA): ");
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 0x10) Serial.print('0');
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(':');
  }
  Serial.println();

  Serial.println("=== Copy this MAC for your transmitter ===");
}

void loop() {}
 