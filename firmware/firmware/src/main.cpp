#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ArduinoOTA.h>
#include "config.h"

static const uint8_t PIN_FLOAT_LOW   = 32;
static const uint8_t PIN_FLOAT_MID   = 33;
static const uint8_t PIN_FLOAT_HIGH  = 25;
static const uint8_t PIN_RELAY       = 26;
static const uint8_t PIN_LED_RED     = 14;
static const uint8_t PIN_LED_BLUE    = 27;
static const uint8_t PIN_LED_GREEN   = 12;

static const bool RELAY_ACTIVE_LOW = true;
static const bool FLOAT_ACTIVE_LOW = true;

static const uint8_t VPIN_LEVEL_PERCENT = V1;
static const uint8_t VPIN_MOTOR_STATUS  = V2;

static const unsigned long SAMPLE_INTERVAL_MS = 30000UL;
static const unsigned long NOTIFY_INTERVAL_MS = 3600000UL;
static const unsigned long CRITICAL_DELAY_MS  = 10800000UL;
static const unsigned long OTA_WINDOW_MS      = 300000UL;
static const unsigned long BLINK_PERIOD_MS    = 500UL;

volatile int waterPercent = 0;
int lastWaterPercent = -1;
bool motorOn = false;

unsigned long lastLow25Notify = 0;
bool fifteenNotified = false;
unsigned long lowStateStartMs = 0;
bool lowTimerRunning = false;
unsigned long lastBlink = 0;
bool blinkState = false;
unsigned long otaStartTime = 0;
bool otaOpen = true;

BlynkTimer timer;

void setRelay(bool on) {
  if (RELAY_ACTIVE_LOW) digitalWrite(PIN_RELAY, on ? LOW : HIGH);
  else digitalWrite(PIN_RELAY, on ? HIGH : LOW);
  motorOn = on;
}

void clearLEDs() {
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_BLUE, LOW);
  digitalWrite(PIN_LED_RED, LOW);
}

void showLEDs(int percent) {
  clearLEDs();
  switch (percent) {
    case 100: digitalWrite(PIN_LED_GREEN, HIGH); break;
    case 75:  break;
    case 50:  digitalWrite(PIN_LED_BLUE, HIGH); break;
    case 25:  digitalWrite(PIN_LED_RED, HIGH); break;
    case 15:  break;
  }
}

void handleBlink(int percent) {
  if (percent != 75 && percent != 15) return;
  unsigned long now = millis();
  if (now - lastBlink >= BLINK_PERIOD_MS) {
    lastBlink = now;
    blinkState = !blinkState;
    if (percent == 75) digitalWrite(PIN_LED_GREEN, blinkState ? HIGH : LOW);
    else digitalWrite(PIN_LED_RED, blinkState ? HIGH : LOW);
  }
}

bool levelPresent(bool raw) { return FLOAT_ACTIVE_LOW ? (raw == LOW) : (raw == HIGH); }

int computeLevelPercent(bool atLow, bool atMid, bool atHigh) {
  if (atHigh && atMid && atLow) return 100;
  if (!atHigh && atMid && atLow) return 75;
  if (!atHigh && !atMid && atLow) return 50;
  return 25;
}

int readWaterLevel() {
  bool atHigh = levelPresent(digitalRead(PIN_FLOAT_HIGH));
  bool atMid  = levelPresent(digitalRead(PIN_FLOAT_MID));
  bool atLow  = levelPresent(digitalRead(PIN_FLOAT_LOW));

  int percent = computeLevelPercent(atLow, atMid, atHigh);
  unsigned long now = millis();

  if (percent == 25) {
    if (!lowTimerRunning) {
      lowTimerRunning = true;
      lowStateStartMs = now;
    } else if (now - lowStateStartMs >= CRITICAL_DELAY_MS) percent = 15;
  } else {
    lowTimerRunning = false;
  }

  return percent;
}

void pushToBlynk(int percent) {
  if (percent != lastWaterPercent) {
    Blynk.virtualWrite(VPIN_LEVEL_PERCENT, percent);
    Blynk.virtualWrite(VPIN_MOTOR_STATUS, motorOn ? "ON" : "OFF");
    lastWaterPercent = percent;
  }
}

void checkNotifications(int percent) {
  unsigned long now = millis();
  if (percent == 25 && (now - lastLow25Notify >= NOTIFY_INTERVAL_MS)) {
    Blynk.logEvent("low_water", "Warning: Tank water at 25%");
    lastLow25Notify = now;
  }
  if (percent == 15 && !fifteenNotified) {
    Blynk.logEvent("critical_water", "Critical: Tank below 15%! Motor OFF");
    fifteenNotified = true;
  }
  if (percent > 25) fifteenNotified = false;
}

void controlMotor(int percent) {
  if (percent == 15) setRelay(false);
  else setRelay(true);
}

void sampleTask() {
  int percent = readWaterLevel();
  controlMotor(percent);
  checkNotifications(percent);
  pushToBlynk(percent);
  waterPercent = percent;
  showLEDs(percent);
}

void setupGPIO() {
  pinMode(PIN_FLOAT_LOW, INPUT_PULLUP);
  pinMode(PIN_FLOAT_MID, INPUT_PULLUP);
  pinMode(PIN_FLOAT_HIGH, INPUT_PULLUP);
  pinMode(PIN_RELAY, OUTPUT);
  setRelay(false);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  clearLEDs();
}

void setupWiFiAndBlynk() { Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS); }

void setupOTA() {
  ArduinoOTA.setHostname("ESP32-WaterMonitor");
  ArduinoOTA.begin();
  otaStartTime = millis();
}

void setup() {
  Serial.begin(115200);
  setupGPIO();
  setupWiFiAndBlynk();
  setupOTA();
  timer.setInterval(SAMPLE_INTERVAL_MS, sampleTask);
}

void loop() {
  Blynk.run();
  timer.run();
  if (otaOpen) {
    unsigned long now = millis();
    if (now - otaStartTime <= OTA_WINDOW_MS) ArduinoOTA.handle();
    else otaOpen = false;
  }
  handleBlink(waterPercent);
}
