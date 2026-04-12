/*
 * ============================================================
 * SmartThermo Phase 3 — Wi-Fi & Google Sheets Integration
 * ESP32 + DHT22 + Gree AC (IR) + I2C LCD + Wi-Fi Logging
 * ============================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <IRremoteESP8266.h>
#include <ir_Gree.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ── Wi-Fi & Google Sheets config ─────────────────────────────
const char* ssid      = "Handmade Hereos";
const char* password  = "admin123";
const char* scriptUrl = "https://script.google.com/macros/s/AKfycby3Sph_hCZV9bX5cS6v5fkehoYulXwKRZOLm6wEMfRHKZ4dpHyjm0AnLq1JRopFkwZ4/exec";

// ── Pin config ───────────────────────────────────────────────
#define IR_LED_PIN      4
#define DHT_PIN         14
#define BTN_UP_PIN      12
#define BTN_DOWN_PIN    32
#define BTN_CONFIRM_PIN 33
#define RED_LED_PIN     25
#define GREEN_LED_PIN   26
#define BUZZER_PIN      27

// ── Settings ─────────────────────────────────────────────────
#define TEMP_MIN        16
#define TEMP_MAX        30
#define TEMP_DEFAULT    21
#define TEMP_TOLERANCE  0.5f

// ── Timing ───────────────────────────────────────────────────
#define DHT_INTERVAL_MS  2000
#define LONG_PRESS_MS    2000
#define BUZZER_ON_MS     200
#define BUZZER_OFF_MS    800
#define LOG_INTERVAL_MS  3600000UL // Logs to Google every 1 hour

// ── Objects ──────────────────────────────────────────────────
DHT dht(DHT_PIN, DHT22);
LiquidCrystal_I2C lcd(0x27, 16, 2);
IRGreeAC ac(IR_LED_PIN);

// ── State ────────────────────────────────────────────────────
float currentTemp     = 0.0;
float currentHumidity = 0.0;
bool  sensorReady     = false;
bool  acOn            = false;
bool  autoTriggered   = false;
bool  alertActive     = false;
bool  buzzerState     = false;
int   setpoint        = TEMP_DEFAULT;

// ── Timing Variables ─────────────────────────────────────────
unsigned long lastDHTRead    = 0;
unsigned long lastBuzzerTime = 0;
unsigned long lastLogTime    = 0;

// ── Button state ─────────────────────────────────────────────
bool lastUpState      = HIGH;
bool lastDownState    = HIGH;
bool lastConfirmState = HIGH;
unsigned long confirmPressTime = 0;
bool confirmHeld      = false;

// ── Google Sheets Data Logger ────────────────────────────────
void logDataToGoogle() {
  // Only attempt to log if connected to Wi-Fi and sensor has valid data
  if (WiFi.status() == WL_CONNECTED && sensorReady) {
    Serial.println("[WIFI] Connecting to Google...");
    HTTPClient http;
    
    // Configure the connection
    http.begin(scriptUrl);
    http.addHeader("Content-Type", "application/json");
    
    // Build the JSON payload string exactly how the Apps Script expects it
    String payload = "{\"temp\":" + String(currentTemp, 1) + 
                     ",\"hum\":" + String(currentHumidity, 1) + 
                     ",\"acState\":\"" + (acOn ? "ON" : "OFF") + "\"" +
                     ",\"setpoint\":" + String(setpoint) + "}";
                     
    Serial.println("[WIFI] Sending: " + payload);
    
    // Send the POST request
    int httpResponseCode = http.POST(payload);
    
    // Check if it was successful (HTTP 200 or 302 redirects)
    if (httpResponseCode > 0) {
      Serial.print("[WIFI] Success! HTTP Code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("[WIFI] Error sending POST: ");
      Serial.println(httpResponseCode);
    }
    
    http.end(); // Free the resources
  } else if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Error: Not connected to Wi-Fi. Cannot log.");
  }
}

// ── IR Helper Function ───────────────────────────────────────
void sendACState() {
  if (acOn) {
    ac.setPower(true);
    ac.setMode(kGreeCool);
    ac.setFan(kGreeFanAuto);
    ac.setTemp(setpoint);
    Serial.print("[IR] Sending GREE AC ON, Temp: ");
    Serial.println(setpoint);
  } else {
    ac.setPower(false);
    Serial.println("[IR] Sending GREE AC OFF");
  }
  ac.send();
}

// ── LCD update ───────────────────────────────────────────────
void updateLCD() {
  lcd.clear();

  lcd.setCursor(0, 0);
  if (sensorReady) {
    lcd.print("T:");
    lcd.print(currentTemp, 1);
    lcd.print("C H:");
    lcd.print((int)currentHumidity);
    lcd.print("%");
  } else {
    lcd.print("T:---  H:---");
  }

  lcd.setCursor(0, 1);
  if (acOn) {
    lcd.print("AC:ON S:");
    lcd.print(setpoint);
    lcd.print("C");
  } else {
    lcd.print("AC:OFF S:");
    lcd.print(setpoint);
    lcd.print("C");
  }
}

// ── Alert indicators ─────────────────────────────────────────
void updateAlert(bool active) {
  alertActive = active;
  if (!active) {
    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN,   LOW);
    digitalWrite(BUZZER_PIN,    LOW);
    buzzerState = false;
  } else {
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN,   HIGH);
  }
}

// ── Buzzer non-blocking blink ────────────────────────────────
void handleBuzzer() {
  if (!alertActive) return;
  unsigned long now = millis();
  if (buzzerState && now - lastBuzzerTime >= BUZZER_ON_MS) {
    buzzerState = false;
    digitalWrite(BUZZER_PIN, LOW);
    lastBuzzerTime = now;
  } else if (!buzzerState && now - lastBuzzerTime >= BUZZER_OFF_MS) {
    buzzerState = true;
    digitalWrite(BUZZER_PIN, HIGH);
    lastBuzzerTime = now;
  }
}

// ── Button handlers ──────────────────────────────────────────
void handleButtons() {
  bool upState      = digitalRead(BTN_UP_PIN);
  bool downState    = digitalRead(BTN_DOWN_PIN);
  bool confirmState = digitalRead(BTN_CONFIRM_PIN);
  unsigned long now = millis();

  if (lastUpState == HIGH && upState == LOW) {
    if (setpoint < TEMP_MAX) {
      setpoint++;
      updateLCD();
    }
    delay(50);
  }
  lastUpState = upState;

  if (lastDownState == HIGH && downState == LOW) {
    if (setpoint > TEMP_MIN) {
      setpoint--;
      updateLCD();
    }
    delay(50);
  }
  lastDownState = downState;

  if (lastConfirmState == HIGH && confirmState == LOW) {
    confirmPressTime = now;
    confirmHeld      = false;
  }

  if (confirmState == LOW && !confirmHeld) {
    if (now - confirmPressTime >= LONG_PRESS_MS) {
      confirmHeld = true;
      if (acOn) {
        acOn          = false;
        autoTriggered = false;
        sendACState();
        updateLCD();
        logDataToGoogle(); // Log the event immediately when turned off
      }
    }
  }

  if (lastConfirmState == LOW && confirmState == HIGH) {
    if (!confirmHeld) {
      if (!acOn) {
        acOn = true;
      }
      sendACState();
      autoTriggered = false;
      updateLCD();
      logDataToGoogle(); // Log the event immediately when turned on or adjusted
    }
    confirmHeld = false;
    delay(50);
  }

  lastConfirmState = confirmState;
}

// ── DHT read + auto-control ──────────────────────────────────
void readDHT() {
  if (millis() - lastDHTRead < DHT_INTERVAL_MS) return;
  lastDHTRead = millis();

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) return;

  currentTemp     = t;
  currentHumidity = h;
  sensorReady     = true;

  bool outOfRange = (t < setpoint - TEMP_TOLERANCE || t > setpoint + TEMP_TOLERANCE);
  updateAlert(outOfRange);

  if (acOn) {
    if (t > setpoint && !autoTriggered) {
      sendACState();
      autoTriggered = true;
      logDataToGoogle(); // Log the auto-trigger event
    } else if (t <= setpoint && autoTriggered) {
      autoTriggered = false;
    }
  }
  updateLCD();
}

// ── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(BTN_UP_PIN,      INPUT_PULLUP);
  pinMode(BTN_DOWN_PIN,    INPUT_PULLUP);
  pinMode(BTN_CONFIRM_PIN, INPUT_PULLUP);
  pinMode(RED_LED_PIN,     OUTPUT);
  pinMode(GREEN_LED_PIN,   OUTPUT);
  pinMode(BUZZER_PIN,      OUTPUT);

  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN,   LOW);
  digitalWrite(BUZZER_PIN,    LOW);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("  SmartThermo   ");
  lcd.setCursor(0, 1);
  lcd.print("Connecting Wi-Fi");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.print(ssid);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WIFI] Connected!");
    lcd.setCursor(0, 1);
    lcd.print(" Wi-Fi Connected");
  } else {
    Serial.println("\n[WIFI] Failed to connect.");
    lcd.setCursor(0, 1);
    lcd.print(" Wi-Fi Failed   ");
  }
  
  delay(2000);
  
  dht.begin();
  ac.begin();
  updateLCD();
}

// ── Loop ─────────────────────────────────────────────────────
void loop() {
  handleButtons();
  handleBuzzer();
  readDHT();

  // Log data to Google Sheets every X milliseconds
  if (millis() - lastLogTime >= LOG_INTERVAL_MS) {
    lastLogTime = millis();
    logDataToGoogle();
  }
}