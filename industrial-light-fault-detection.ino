/*****************************************
* Project: Industrial Light Monitoring & Fault Detection System
* Author: Ali Sbeity
* Version: 1.0
* Copyright (c) 2026 Ali Sbeity
* License: MIT License
********************************/


#include <EEPROM.h>

/* --- Pin Definitions --- */
#define LDR_PIN A0

#define BTN_MODE 2
#define BTN_SET  3

#define LED1 8
#define LED2 9
#define LED3 10
#define LED4 11

#define BUZZER 6

/* --- EEPROM Addresses --- */
#define EE_REF        0
#define EE_SENS       4
#define EE_ALARM_CNT  8
#define EE_NOISE_CNT  12

/* --- System States --- */
enum State {
  MONITORING,
  CALIBRATION,
  ALARM,
  NOISE
};

State currentState = MONITORING;

/* --- Parameters --- */
int referenceLight = 500;
int sensitivity = 80;

int alarmCounter = 0;
int noiseCounter = 0;

/* --- Hysteresis --- */
int alarmOnThreshold;
int alarmOffThreshold;
bool alarmLatched = false;

/* --- Day/Night --- */
bool isNight = false;

/* Filtering */
float filteredValue = 0;
const float alpha = 0.1;

/* --- Sudden change detection --- */
float previousFiltered = 0;
int suddenThreshold = 60;

/* --- Alarm duration --- */
unsigned long alarmStartTime = 0;
const unsigned long alarmDuration = 2000;

/* --- Noise detection --- */
bool noiseDetected = false;
int noiseMax = 0;
int noiseMin = 1023;
unsigned long noiseWindowStart = 0;
const unsigned long noiseWindow = 1000;
int noiseLimit = 120;

/* --- Timing --- */
unsigned long previousMillis = 0;
const unsigned long sampleInterval = 50;

/* --- Serial Monitoring Timing --- */
unsigned long serialTimer = 0;
const unsigned long serialInterval = 500;

/* --- Button debounce --- */
unsigned long lastDebounceMode = 0;
unsigned long lastDebounceSet = 0;
const unsigned long debounceDelay = 50;

bool lastModeState = LOW;
bool lastSetState = LOW;

/* --- Alarm pattern --- */
unsigned long alarmTimer = 0;
bool alarmToggle = false;

/* --- Noise pattern --- */
unsigned long noiseTimer = 0;
bool noiseToggle = false;


/* --- Setup --- */
void setup() {

  Serial.begin(9600);

  pinMode(BTN_MODE, INPUT);
  pinMode(BTN_SET, INPUT);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  EEPROM.get(EE_REF, referenceLight);
  EEPROM.get(EE_SENS, sensitivity);
  EEPROM.get(EE_ALARM_CNT, alarmCounter);
  EEPROM.get(EE_NOISE_CNT, noiseCounter);

  if (referenceLight < 0 || referenceLight > 1023) referenceLight = 500;
  if (sensitivity < 20 || sensitivity > 400) sensitivity = 100;

  filteredValue = analogRead(LDR_PIN);
  previousFiltered = filteredValue;
}


/* --- Main Loop --- */
void loop() {

  readButtons();

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= sampleInterval) {
    previousMillis = currentMillis;

    int raw = analogRead(LDR_PIN);

    /* Digital Low-pass Filter (EMA) */
    filteredValue = filteredValue * (1 - alpha) + raw * alpha;

    updateDayNight();
    updateThresholds();
    updateNoise(raw);
    updateFSM();
    updateOutputs();

    if (millis() - serialTimer > serialInterval) {
      serialTimer = millis();
      printSystemStatus(raw);
    }

    previousFiltered = filteredValue;
  }
}

/* --- Day/Night Detection --- */
void updateDayNight() {
  isNight = (filteredValue < 300);
}

/* --- Threshold Calculation --- */
void updateThresholds() {

  int baseSens = sensitivity;

  if (isNight)
    baseSens *= 0.7;

  alarmOnThreshold  = baseSens;
  alarmOffThreshold = baseSens * 0.6;
}

/* --- Noise Detection --- */
void updateNoise(int value) {

  if (value > noiseMax) noiseMax = value;
  if (value < noiseMin) noiseMin = value;

  if (millis() - noiseWindowStart > noiseWindow) {

    int noiseRange = noiseMax - noiseMin;

    noiseDetected = (noiseRange > noiseLimit);

    noiseMax = 0;
    noiseMin = 1023;
    noiseWindowStart = millis();
  }
}

/* --- FSM Logic --- */
void updateFSM() {

  if (currentState == CALIBRATION) return;

  float errorRef  = abs(filteredValue - referenceLight);
  float errorRate = abs(filteredValue - previousFiltered);

  switch (currentState) {

    case MONITORING:

      if (noiseDetected) {
        currentState = NOISE;
        noiseCounter++;
        EEPROM.put(EE_NOISE_CNT, noiseCounter);
        break;
      }

      /* --- Hysteresis Latch --- */
      if (!alarmLatched && errorRef > alarmOnThreshold) {
        alarmLatched = true;
      }

      if (alarmLatched && errorRef < alarmOffThreshold) {
        alarmLatched = false;
      }

      if (alarmLatched && errorRate > suddenThreshold) {
        currentState = ALARM;
        alarmStartTime = millis();
        alarmCounter++;
        EEPROM.put(EE_ALARM_CNT, alarmCounter);
      }

      break;

    case ALARM:

      if (millis() - alarmStartTime > alarmDuration) {
        currentState = MONITORING;
      }

      break;

    case NOISE:

      if (!noiseDetected && errorRef < alarmOffThreshold) {
        currentState = MONITORING;
      }

      break;
  }
}

/* --- Output Control --- */
void updateOutputs() {

  if (currentState == MONITORING) {
    displayBar(filteredValue);
    noTone(BUZZER);
  }
  else if (currentState == CALIBRATION) {
    blinkAllLEDs(200);
  }
  else if (currentState == ALARM) {
    alarmPattern();
  }
  else if (currentState == NOISE) {
    noisePattern();
  }
}

/* --- LED Bar Graph --- */
void displayBar(int value) {

  digitalWrite(LED1, value > 250);
  digitalWrite(LED2, value > 500);
  digitalWrite(LED3, value > 750);
  digitalWrite(LED4, value > 950);
}

/* --- Calibration --- */
void startCalibration() {

  currentState = CALIBRATION;

  long sum = 0;
  int samples = 100;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(LDR_PIN);
    delay(10);
  }

  referenceLight = sum / samples;
  EEPROM.put(EE_REF, referenceLight);

  alarmLatched = false;
  currentState = MONITORING;
}

/* --- Alarm Pattern --- */
void alarmPattern() {

  if (millis() - alarmTimer > 200) {
    alarmTimer = millis();
    alarmToggle = !alarmToggle;

    digitalWrite(LED1, alarmToggle);
    digitalWrite(LED2, alarmToggle);
    digitalWrite(LED3, alarmToggle);
    digitalWrite(LED4, alarmToggle);

    if (alarmToggle)
      tone(BUZZER, 2000);
    else
      noTone(BUZZER);
  }
}

/* --- Noise Pattern --- */
void noisePattern() {

  if (millis() - noiseTimer > 150) {
    noiseTimer = millis();
    noiseToggle = !noiseToggle;

    digitalWrite(LED4, noiseToggle);

    if (noiseToggle)
      tone(BUZZER, 4000);
    else
      noTone(BUZZER);
  }
}

/* --- LED Blink Helper --- */
void blinkAllLEDs(int interval) {

  static unsigned long timer = 0;
  static bool state = false;

  if (millis() - timer > interval) {
    timer = millis();
    state = !state;

    digitalWrite(LED1, state);
    digitalWrite(LED2, state);
    digitalWrite(LED3, state);
    digitalWrite(LED4, state);
  }
}

/* --- Button Handling --- */
void readButtons() {

  bool modeState = digitalRead(BTN_MODE);
  bool setState  = digitalRead(BTN_SET);

  if (modeState != lastModeState) {
    lastDebounceMode = millis();
  }

  if ((millis() - lastDebounceMode) > debounceDelay) {
    if (modeState == HIGH && lastModeState == LOW) {
      sensitivity += 20;
      if (sensitivity > 300) sensitivity = 60;
      EEPROM.put(EE_SENS, sensitivity);
    }
  }

  lastModeState = modeState;

  if (setState != lastSetState) {
    lastDebounceSet = millis();
  }

  if ((millis() - lastDebounceSet) > debounceDelay) {
    if (setState == HIGH && lastSetState == LOW) {
      startCalibration();
    }
  }

  lastSetState = setState;
}

/* --- Serial Monitor --- */
void printSystemStatus(int rawValue) {

  Serial.println("----- SYSTEM STATUS -----");

  Serial.print("Raw: ");
  Serial.println(rawValue);

  Serial.print("Filtered: ");
  Serial.println(filteredValue);

  Serial.print("Reference: ");
  Serial.println(referenceLight);

  Serial.print("Change Rate: ");
  Serial.println(abs(filteredValue - previousFiltered));

  Serial.print("Sensitivity: ");
  Serial.println(sensitivity);

  Serial.print("Sudden Threshold: ");
  Serial.println(suddenThreshold);

  Serial.print("Alarm ON Threshold: ");
  Serial.println(alarmOnThreshold);

  Serial.print("Alarm OFF Threshold: ");
  Serial.println(alarmOffThreshold);

  Serial.print("Day/Night: ");
  Serial.println(isNight ? "Night" : "Day");

  Serial.print("State: ");
  if (currentState == MONITORING) Serial.println("MONITORING");
  else if (currentState == CALIBRATION) Serial.println("CALIBRATION");
  else if (currentState == ALARM) Serial.println("ALARM");
  else if (currentState == NOISE) Serial.println("NOISE");

  Serial.print("Alarm Count: ");
  Serial.println(alarmCounter);

  Serial.print("Noise Count: ");
  Serial.println(noiseCounter);

  Serial.println("--------------------------");
  Serial.println();
}


// Copyright (c) 2026 Ali Sbeity  
// License: MIT License
