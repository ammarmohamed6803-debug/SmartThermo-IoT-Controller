# SmartThermo: IoT Environmental Controller 🌡️⚙️

An automated, closed-loop climate regulation and telemetry logging system built with an ESP32. This project reverse-engineers stateful AC IR protocols to maintain strict temperature tolerances while logging real-time data to the cloud.

## Features
* **Real-Time Edge Sensing:** Uses a DHT22 sensor for high-fidelity temperature and humidity tracking.
* **Algorithmic IR Control:** Generates mathematical 64-bit Gree/Sharp IR protocols on the fly to eliminate signal jitter and reliably control the AC unit.
* **Closed-Loop Actuation:** Automatically adjusts the AC state to maintain a strict ±0.5°C tolerance of the user's setpoint.
* **Cloud Telemetry Logging:** Connects via Wi-Fi to POST JSON payload data (Temp, Hum, AC State, Setpoint) to a custom Google Apps Script webhook, logging chronologically in Google Sheets.
* **Tactile HMI & Alerts:** Features an I2C LCD for live telemetry, manual override buttons, and a non-blocking audiovisual alarm system for out-of-range states.

## Hardware Components
* ESP32 Development Board
* DHT22 Temperature & Humidity Sensor
* 38kHz IR Transmitter LED (driven by 2N2222 NPN Transistor)
* 16x2 I2C LCD Display
* Active Buzzer & Status LEDs (Red/Green)
* Tactile Push Buttons

## System Architecture 
<img width="1280" height="183" alt="image" src="https://github.com/user-attachments/assets/70467464-8818-4f75-95de-e04f84ee4c1d" />
## Circuit Schematic
<img width="1280" height="685" alt="image" src="https://github.com/user-attachments/assets/d3d1a04c-aa55-4283-8557-428095ba1eaf" />
## 3D Enclosure Design
<img width="989" height="543" alt="image" src="https://github.com/user-attachments/assets/c9c7c01a-3d0a-4b9b-8c2f-283943edc47d" />

## Libraries Used
* `IRremoteESP8266` (For Gree/TECO/Haier protocol generation)
* `DHT sensor library`
* `LiquidCrystal_I2C`
* `WiFi.h` & `HTTPClient.h`
