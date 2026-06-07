# ESP-32-car using ESP-NOW Protocol

Overwiew
This project is a wireless ar built using two ESP32 microcontrollers and the ESP-NOW communication protocol.The transmitter sends the control commands wirelessly to the receiver ESP32 mounted on the car enabling real-time movement with low latency.

Components required
1. ESP 32 Dev Board - 2
2. Motor driver (L298N) - 1
3. DC motors - 2/4 (based on chassis)
4. Chassis - 1
5. Wheels - 2/4 (based on chassis)
6. Castor wheel - 1 (optional)
7. Battery - 1
8. Joystic module - 1

System Architecture
Transmitter ESP32
       |
       |
ESP-NOW Communication
       |
       |
Receiver ESP32
       |
       |
  Motor Driver
       |
       |
    Motors

IMPORTANT
Make sure to get the receiver MAX of your receiver ESP32 first.
