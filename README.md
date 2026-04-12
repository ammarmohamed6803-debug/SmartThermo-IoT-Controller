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
*(Add your flowchart image here!)*

## Circuit Schematic
*(Add your Proteus schematic image here!)*

## 3D Enclosure Design
*(Add your CAD model image here!)*

## Libraries Used
* `IRremoteESP8266` (For Gree/TECO/Haier protocol generation)
* `DHT sensor library`
* `LiquidCrystal_I2C`
* `WiFi.h` & `HTTPClient.h`
