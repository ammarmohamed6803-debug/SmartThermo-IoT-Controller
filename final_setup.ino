/*
 * ============================================================
 * SmartThermo Phase 4 — 3-button final
 * ESP32 + DHT22 + Gree AC (IR) + I2C LCD + Wi-Fi Logging
 * ============================================================
 * v4.2 changes:
 *   - FIXED: IR transmission now suspends log task during send
 *           (prevents Wi-Fi/FreeRTOS timing disruption)
 *   - Reverted to single IR send (double-send was unreliable)
 *   - 3 buttons: UP, DOWN, POWER
 *   - UP/DOWN auto-sends new setpoint after 2s wait
 *   - UP long-press (2s) toggles MANUAL / AUTO mode
 *   - POWER toggles AC ON/OFF
 *   - Alarm fires when OUT of range
 *   - HTTP logging on Core 0 (non-blocking)
 * ============================================================
 * MODE 1: Manual — UP/DOWN sets AC temp directly
 * MODE 2: Auto  — UP/DOWN sets target ROOM temp
 * ──────────────────────────────────────────────────────────────
 * UP short      : setpoint +1  (auto-sends after 2s if AC on)
 * UP long (2s)  : toggle MANUAL / AUTO mode
 * DOWN short    : setpoint -1  (auto-sends after 2s if AC on)
 * POWER short   : toggle AC ON / OFF
 * ============================================================
 * >>>  CHANGE roomName PER UNIT BEFORE UPLOADING  
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
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ══════════════════════════════════════════════════════════════
//  CHANGE THIS PER ROOM — must match the Google Sheet tab name
// ══════════════════════════════════════════════════════════════
const char* roomName = "Filling Room";

// ── Wi-Fi & Google Sheets config ─────────────────────────────
const char* ssid      = "SparkeLabs_2.4GHz";
const char* password  = "12345678";
const char* scriptUrl = "https://script.google.com/macros/s/AKfycby3Sph_hCZV9bX5cS6v5fkehoYulXwKRZOLm6wEMfRHKZ4dpHyjm0AnLq1JRopFkwZ4/exec";

// ── Pin config ───────────────────────────────────────────────
#define IR_LED_PIN      4
#define DHT_PIN         14
#define BTN_UP_PIN      12
#define BTN_DOWN_PIN    32
#define BTN_POWER_PIN   33
#define RED_LED_PIN     25
#define GREEN_LED_PIN   26
#define BUZZER_PIN      27

// ── Settings ─────────────────────────────────────────────────
#define TEMP_MIN        16
#define TEMP_MAX        30
#define TEMP_DEFAULT    21
#define AUTO_OFFSET     2

// ── Alarm range (OUT of these = alarm) ───────────────────────
#define ALARM_TEMP_LOW   18.0
#define ALARM_TEMP_HIGH  30.0
#define ALARM_HUM_LOW    30.0
#define ALARM_HUM_HIGH   90.0

// ── Timing ───────────────────────────────────────────────────
#define DHT_INTERVAL_MS       2000
#define LONG_PRESS_MS         2000
#define BUZZER_ON_MS          150
#define BUZZER_OFF_MS         150
#define LOG_INTERVAL_MS       3600000UL
#define AUTO_RESEND_MS        900000UL
#define LOG_DEFER_MS          5000
#define DEBOUNCE_MS           50
#define SETPOINT_SEND_DELAY   2000

// ── Modes ────────────────────────────────────────────────────
#define MODE_MANUAL  0
#define MODE_AUTO    1

// ── Objects ──────────────────────────────────────────────────
DHT dht(DHT_PIN, DHT22);
LiquidCrystal_I2C lcd(0x27, 16, 2);
IRGreeAC ac(IR_LED_PIN);

// ── State ────────────────────────────────────────────────────
float currentTemp     = 0.0;
float currentHumidity = 0.0;
bool  sensorReady     = false;
bool  acOn            = false;
bool  alertActive     = false;
bool  buzzerState     = false;
int   setpoint        = TEMP_DEFAULT;
int   currentMode     = MODE_MANUAL;

unsigned long lastAutoSend = 0;
bool autoHasSentOnce = false;

TaskHandle_t   logTaskHandle = NULL;
volatile bool  logRequested  = false;
bool pendingLog = false;

bool pendingSetpointSend = false;
unsigned long lastSetpointChange = 0;

// ── Timing Variables ─────────────────────────────────────────
unsigned long lastDHTRead         = 0;
unsigned long lastBuzzerTime      = 0;
unsigned long lastLogTime         = 0;
unsigned long lastInteractionTime = 0;

// ── Button state ─────────────────────────────────────────────
bool lastUpState    = HIGH;
bool lastDownState  = HIGH;
bool lastPowerState = HIGH;
unsigned long upPressTime = 0;
bool upHeld = false;

unsigned long lastUpDebounce    = 0;
unsigned long lastDownDebounce  = 0;
unsigned long lastPowerDebounce = 0;

// ── HTTP runs on Core 0 (non-blocking) ───────────────────────
void logTask(void *parameter) {
  for (;;) {
    if (logRequested) {
      logRequested = false;

      if (WiFi.status() == WL_CONNECTED && sensorReady) {
        Serial.println("[WIFI] Connecting to Google...");
        HTTPClient http;
        http.begin(scriptUrl);
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(5000);

        String modeStr = (currentMode == MODE_AUTO) ? "AUTO" : "MANUAL";
        String payload = "{\"room\":\"" + String(roomName) + "\"" +
                         ",\"temp\":" + String(currentTemp, 1) +
                         ",\"hum\":" + String(currentHumidity, 1) +
                         ",\"acState\":\"" + (acOn ? "ON" : "OFF") + "\"" +
                         ",\"setpoint\":" + String(setpoint) +
                         ",\"mode\":\"" + modeStr + "\"}";

        Serial.println("[WIFI] Sending: " + payload);
        int httpResponseCode = http.POST(payload);

        if (httpResponseCode > 0) {
          Serial.print("[WIFI] Success! HTTP Code: ");
          Serial.println(httpResponseCode);
        } else {
          Serial.print("[WIFI] Error sending POST: ");
          Serial.println(httpResponseCode);
        }
        http.end();
      } else if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WIFI] Not connected. Skipping log.");
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// ── IR Helper Functions ──────────────────────────────────────
// FIX: suspend log task during IR send to prevent timing disruption.
// FreeRTOS + Wi-Fi can interfere with the 38kHz IR carrier.
void sendACCommand(int acTemp) {
  if (acTemp < TEMP_MIN) acTemp = TEMP_MIN;
  if (acTemp > TEMP_MAX) acTemp = TEMP_MAX;

  // Pause the log task so HTTP/Wi-Fi doesn't disrupt IR timing
  if (logTaskHandle != NULL) vTaskSuspend(logTaskHandle);
  noInterrupts();   // protect IR carrier timing

  ac.setPower(true);
  ac.setMode(kGreeCool);
  ac.setFan(kGreeFanAuto);
  ac.setTemp(acTemp);
  ac.send();

  interrupts();
  if (logTaskHandle != NULL) vTaskResume(logTaskHandle);

  Serial.print("[IR] AC ON -> ");
  Serial.print(acTemp);
  Serial.println("C");
}

void sendACOff() {
  if (logTaskHandle != NULL) vTaskSuspend(logTaskHandle);
  noInterrupts();

  ac.setPower(false);
  ac.send();

  interrupts();
  if (logTaskHandle != NULL) vTaskResume(logTaskHandle);

  Serial.println("[IR] AC OFF");
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
  if (currentMode == MODE_MANUAL) {
    if (acOn) {
      lcd.print("AC:ON S:");
      lcd.print(setpoint);
      lcd.print("C");
    } else {
      lcd.print("AC:OFF S:");
      lcd.print(setpoint);
      lcd.print("C");
    }
  } else {
    if (acOn) {
      lcd.print("AUTO T:");
      lcd.print(setpoint);
      lcd.print("C [ON]");
    } else {
      lcd.print("AUTO T:");
      lcd.print(setpoint);
      lcd.print("C[OFF]");
    }
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

// ── Beep helpers ─────────────────────────────────────────────
void buttonBeep() {
  if (alertActive) return;
  digitalWrite(BUZZER_PIN, HIGH);
  delay(40);
  digitalWrite(BUZZER_PIN, LOW);
}

void powerOnBeep() {
  if (alertActive) return;
  digitalWrite(BUZZER_PIN, HIGH); delay(60); digitalWrite(BUZZER_PIN, LOW);
  delay(60);
  digitalWrite(BUZZER_PIN, HIGH); delay(60); digitalWrite(BUZZER_PIN, LOW);
}

void powerOffBeep() {
  if (alertActive) return;
  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
}

// ── Flag a log event securely ────────────────────────────────
void triggerLog() {
  pendingLog = true;
  lastInteractionTime = millis();
}

// ── Mode toggle ──────────────────────────────────────────────
void toggleMode() {
  if (currentMode == MODE_MANUAL) {
    currentMode = MODE_AUTO;
    autoHasSentOnce = false;
    lastAutoSend = 0;
    Serial.println("[MODE] -> AUTO");
  } else {
    currentMode = MODE_MANUAL;
    Serial.println("[MODE] -> MANUAL");
  }

  buttonBeep();
  delay(60);
  buttonBeep();

  updateLCD();
  triggerLog();
}

// ── Auto switch to manual when target reached ────────────────
void autoSwitchToManual() {
  currentMode = MODE_MANUAL;
  Serial.print("[MODE] Target reached -> MANUAL at ");
  Serial.print(setpoint);
  Serial.println("C");

  buttonBeep();
  delay(50);
  buttonBeep();
  delay(50);
  buttonBeep();

  updateLCD();
  triggerLog();
}

// ── Button handlers ──────────────────────────────────────────
void handleButtons() {
  bool upState    = digitalRead(BTN_UP_PIN);
  bool downState  = digitalRead(BTN_DOWN_PIN);
  bool powerState = digitalRead(BTN_POWER_PIN);
  unsigned long now = millis();

  // ════════════════════════════════════════════════════════════
  // BTN UP — short: setpoint+1 / long (2s): toggle Manual/Auto
  // ════════════════════════════════════════════════════════════
  if (lastUpState == HIGH && upState == LOW && (now - lastUpDebounce > DEBOUNCE_MS)) {
    lastUpDebounce = now;
    upPressTime = now;
    upHeld = false;
    lastInteractionTime = now;
    buttonBeep();
  }

  if (upState == LOW && !upHeld) {
    if (now - upPressTime >= LONG_PRESS_MS) {
      upHeld = true;
      toggleMode();
    }
  }

  if (lastUpState == LOW && upState == HIGH) {
    if (!upHeld) {
      if (setpoint < TEMP_MAX) {
        setpoint++;
        Serial.print("[BTN] Setpoint: ");
        Serial.println(setpoint);

        if (acOn) {
          pendingSetpointSend = true;
          lastSetpointChange = now;
        }
        if (currentMode == MODE_AUTO) {
          autoHasSentOnce = false;
          lastAutoSend = 0;
        }
        updateLCD();
      }
    }
    upHeld = false;
  }
  lastUpState = upState;

  // ════════════════════════════════════════════════════════════
  // BTN DOWN — setpoint-1
  // ════════════════════════════════════════════════════════════
  if (lastDownState == HIGH && downState == LOW && (now - lastDownDebounce > DEBOUNCE_MS)) {
    lastDownDebounce = now;
    lastInteractionTime = now;
    buttonBeep();
    if (setpoint > TEMP_MIN) {
      setpoint--;
      Serial.print("[BTN] Setpoint: ");
      Serial.println(setpoint);

      if (acOn) {
        pendingSetpointSend = true;
        lastSetpointChange = now;
      }
      if (currentMode == MODE_AUTO) {
        autoHasSentOnce = false;
        lastAutoSend = 0;
      }
      updateLCD();
    }
  }
  lastDownState = downState;

  // ════════════════════════════════════════════════════════════
  // BTN POWER — toggle AC ON/OFF
  // ════════════════════════════════════════════════════════════
  if (lastPowerState == HIGH && powerState == LOW && (now - lastPowerDebounce > DEBOUNCE_MS)) {
    lastPowerDebounce = now;
    lastInteractionTime = now;
  }

  if (lastPowerState == LOW && powerState == HIGH) {
    if (!acOn) {
      acOn = true;
      if (currentMode == MODE_MANUAL) {
        Serial.print("[PWR] AC ON manual -> ");
        Serial.println(setpoint);
        sendACCommand(setpoint);
      } else {
        int acTemp = setpoint - AUTO_OFFSET;
        Serial.print("[PWR] AC ON auto -> ");
        Serial.println(acTemp);
        sendACCommand(acTemp);
        lastAutoSend = now;
        autoHasSentOnce = true;
      }
      powerOnBeep();
    } else {
      acOn = false;
      autoHasSentOnce = false;
      pendingSetpointSend = false;
      Serial.println("[PWR] AC OFF");
      sendACOff();
      powerOffBeep();
    }
    updateLCD();
    triggerLog();
  }
  lastPowerState = powerState;
}

// ── DHT read + auto-control ──────────────────────────────────
void readDHT() {
  if (millis() - lastDHTRead < DHT_INTERVAL_MS) return;
  lastDHTRead = millis();

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println("[DHT] Read failed, retrying...");
    return;
  }

  currentTemp     = t;
  currentHumidity = h;
  sensorReady     = true;

  bool outOfRange = (t <= ALARM_TEMP_LOW) || (t >= ALARM_TEMP_HIGH) ||
                    (h <= ALARM_HUM_LOW)  || (h >= ALARM_HUM_HIGH);
  updateAlert(outOfRange);

  if (currentMode == MODE_AUTO && acOn) {
    unsigned long now = millis();

    if (t <= setpoint + 0.5f && autoHasSentOnce) {
      autoSwitchToManual();
      return;
    }

    bool canResend = !autoHasSentOnce || (now - lastAutoSend >= AUTO_RESEND_MS);

    if (t > setpoint + 0.5f && canResend) {
      int acTemp = setpoint - AUTO_OFFSET;
      Serial.print("[AUTO] Room ");
      Serial.print(t, 1);
      Serial.print("C > target ");
      Serial.print(setpoint);
      Serial.print("C, sending AC=");
      Serial.print(acTemp);
      Serial.println("C");
      sendACCommand(acTemp);
      lastAutoSend = now;
      autoHasSentOnce = true;
      triggerLog();
    }
  }

  updateLCD();
}

// ── Setup ────────────────────────────────────────────────────
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);

  pinMode(BTN_UP_PIN,    INPUT_PULLUP);
  pinMode(BTN_DOWN_PIN,  INPUT_PULLUP);
  pinMode(BTN_POWER_PIN, INPUT_PULLUP);
  pinMode(RED_LED_PIN,   OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN,    OUTPUT);

  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN,   LOW);
  digitalWrite(BUZZER_PIN,    LOW);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("  SmartThermo   ");
  lcd.setCursor(0, 1);
  lcd.print("Connecting Wi-Fi");

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

  xTaskCreatePinnedToCore(
    logTask, "logTask", 8192, NULL, 1, &logTaskHandle, 0
  );

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  SmartThermo   ");
  lcd.setCursor(0, 1);
  lcd.print(roomName);
  delay(1500);

  updateLCD();

  Serial.println("=========================================");
  Serial.print("  SmartThermo v4.2 Ready — ");
  Serial.println(roomName);
  Serial.println("  UP/DOWN    : adjust setpoint (auto-send 2s)");
  Serial.println("  UP long    : toggle MANUAL / AUTO mode");
  Serial.println("  POWER      : toggle AC ON / OFF");
  Serial.println("=========================================");
}

// ── Loop ─────────────────────────────────────────────────────
void loop() {
  handleButtons();
  handleBuzzer();
  readDHT();

  if (pendingSetpointSend && (millis() - lastSetpointChange >= SETPOINT_SEND_DELAY)) {
    pendingSetpointSend = false;
    if (acOn) {
      if (currentMode == MODE_MANUAL) {
        Serial.print("[DEFER] Sending setpoint: ");
        Serial.println(setpoint);
        sendACCommand(setpoint);
      } else {
        int acTemp = setpoint - AUTO_OFFSET;
        Serial.print("[DEFER] Sending auto AC: ");
        Serial.println(acTemp);
        sendACCommand(acTemp);
        lastAutoSend = millis();
        autoHasSentOnce = true;
      }
      triggerLog();
    }
  }

  if (pendingLog && (millis() - lastInteractionTime >= LOG_DEFER_MS)) {
    pendingLog = false;
    logRequested = true;
  }

  if (millis() - lastLogTime >= LOG_INTERVAL_MS) {
    lastLogTime = millis();
    logRequested = true;
  }
}
