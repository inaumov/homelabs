#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =====================================================
// OLED
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1);

// =====================================================
// PINS
// =====================================================

const int MOTOR_PIN = 18;
const int BUTTON_PIN = 19;
const int POT_PIN = 34;

// =====================================================
// BPM
// =====================================================

const int MIN_BPM = 40;
const int MAX_BPM = 200;

int bpm = 100;

unsigned long beatInterval = 60000UL / 100;

// =====================================================
// STATE MACHINE
// =====================================================

enum State {
  OFF,
  WELCOME,
  READY,
  METRONOME
};

State state = OFF;

unsigned long stateStartedAt = 0;

const unsigned long WELCOME_DURATION = 1000;

// =====================================================
// BUTTON DEBOUNCE
// =====================================================

int lastButtonReading = HIGH;
int buttonState = HIGH;

unsigned long lastDebounceTime = 0;

const unsigned long DEBOUNCE_TIME = 50;

// =====================================================
// POTENTIOMETER FILTER
// =====================================================

unsigned long lastPotRead = 0;

const unsigned long POT_READ_INTERVAL = 20;

float filteredAdc = 0.0f;
bool adcInitialized = false;

// =====================================================
// METRONOME
// =====================================================

unsigned long lastBeatTime = 0;

int currentBeat = 0;

// =====================================================
// VIBRATION
// =====================================================

bool motorOn = false;

unsigned long motorStartedAt = 0;

const unsigned long MOTOR_DURATION = 150;


// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(MOTOR_PIN, OUTPUT);

  digitalWrite(MOTOR_PIN, LOW);

  analogReadResolution(12);

  // I2C: SDA = GPIO 21, SCL = GPIO 22
  Wire.begin(21, 22);

  Serial.println();
  Serial.println("Starting ESP32 Metronome...");

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS)) {
    Serial.println("ERROR: OLED initialization failed!");

    while (true) {
      digitalWrite(MOTOR_PIN, LOW);
    }
  }

  display.ssd1306_command(SSD1306_DISPLAYON);

  display.clearDisplay();
  display.display();

  beatInterval = 60000UL / bpm;

  Serial.println("OLED initialized successfully.");
  Serial.println("State: OFF");
  Serial.println("Ready.");
}


// =====================================================
// MAIN LOOP
// =====================================================

void loop() {
  updateButton();

  updatePotentiometer();

  updateStateMachine();

  if (state == METRONOME) {
    updateMetronome();
  }

  // Always process motor timeout, without blocking.
  updateVibration();
}


// =====================================================
// BUTTON
// =====================================================

void updateButton() {
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if (millis() - lastDebounceTime >= DEBOUNCE_TIME) {
    if (reading != buttonState) {
      buttonState = reading;

      // INPUT_PULLUP: LOW means pressed.
      if (buttonState == LOW) {
        onButtonPressed();
      }
    }
  }

  lastButtonReading = reading;
}


// =====================================================
// BUTTON ACTION / STATE TRANSITIONS
// =====================================================

void onButtonPressed() {
  Serial.println("Button pressed");

  switch (state) {

    case OFF:
      Serial.println("OFF -> WELCOME");

      display.ssd1306_command(SSD1306_DISPLAYON);

      state = WELCOME;
      stateStartedAt = millis();

      drawWelcome();
      break;

    case WELCOME:
      // Ignore presses during the welcome screen.
      break;

    case READY:
      Serial.println("READY -> METRONOME");

      state = METRONOME;

      currentBeat = 0;
      lastBeatTime = millis();

      // Play the first beat immediately.
      startVibration();
      drawMetronome();

      break;

    case METRONOME:
      Serial.println("METRONOME -> OFF");

      state = OFF;

      // Stop vibration immediately.
      digitalWrite(MOTOR_PIN, LOW);
      motorOn = false;

      turnOffDisplay();

      Serial.println("State: OFF");
      break;
  }
}


// =====================================================
// STATE MACHINE
// =====================================================

void updateStateMachine() {
  if (state == WELCOME && millis() - stateStartedAt >= WELCOME_DURATION) {

    state = READY;

    Serial.println("WELCOME -> READY");

    drawReady();
  }
}


// =====================================================
// POTENTIOMETER
// =====================================================

void updatePotentiometer() {
  unsigned long now = millis();

  if (now - lastPotRead < POT_READ_INTERVAL) {
    return;
  }

  lastPotRead = now;

  int rawAdc = analogRead(POT_PIN);

  // Exponential moving average.
  // No delay() and no blocking sample loop.
  if (!adcInitialized) {
    filteredAdc = rawAdc;
    adcInitialized = true;
  } else {
    filteredAdc =
      filteredAdc * 0.8f + rawAdc * 0.2f;
  }

  int adcValue = (int)(filteredAdc + 0.5f);

  int newBpm = map(
    adcValue,
    0,
    4095,
    MIN_BPM,
    MAX_BPM);

  newBpm = constrain(
    newBpm,
    MIN_BPM,
    MAX_BPM);

  if (newBpm != bpm) {
    bpm = newBpm;

    beatInterval = 60000UL / bpm;

    Serial.print("BPM: ");
    Serial.println(bpm);

    if (state == READY) {
      drawReady();
    } else if (state == METRONOME) {
      drawMetronome();
    }
  }
}


// =====================================================
// METRONOME TIMING
// =====================================================

void updateMetronome() {
  unsigned long now = millis();

  if (now - lastBeatTime >= beatInterval) {
    lastBeatTime = now;

    currentBeat++;

    if (currentBeat >= 4) {
      currentBeat = 0;
    }

    startVibration();

    drawMetronome();
  }
}


// =====================================================
// OLED: WELCOME
// =====================================================

void drawWelcome() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  int16_t x1, y1;
  uint16_t w, h;

  // Greeting
  display.setTextSize(2);

  const char* greeting = "Privet:)";

  display.getTextBounds(
    greeting, 0, 0,
    &x1, &y1, &w, &h);

  display.setCursor(
    (SCREEN_WIDTH - w) / 2,
    16);

  display.println(greeting);

  // Title
  display.setTextSize(1);

  const char* title = "ESP32 METRONOME";

  display.getTextBounds(
    title, 0, 0,
    &x1, &y1, &w, &h);

  display.setCursor(
    (SCREEN_WIDTH - w) / 2,
    42);

  display.println(title);

  display.display();
}


// =====================================================
// OLED: READY
// =====================================================

void drawReady() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  int16_t x1, y1;
  uint16_t w, h;

  // Title
  display.setTextSize(1);

  const char* title = "READY";

  display.getTextBounds(
    title, 0, 0,
    &x1, &y1, &w, &h);

  display.setCursor(
    (SCREEN_WIDTH - w) / 2,
    5);

  display.println(title);

  // BPM
  display.setTextSize(2);

  char bpmText[12];

  snprintf(
    bpmText,
    sizeof(bpmText),
    "%d BPM",
    bpm);

  display.getTextBounds(
    bpmText, 0, 0,
    &x1, &y1, &w, &h);

  display.setCursor(
    (SCREEN_WIDTH - w) / 2,
    22);

  display.println(bpmText);

  // Instruction
  display.setTextSize(1);

  const char* instruction = "Press button to start";

  display.getTextBounds(
    instruction, 0, 0,
    &x1, &y1, &w, &h);

  display.setCursor(
    (SCREEN_WIDTH - w) / 2,
    52);

  display.println(instruction);

  display.display();
}


// =====================================================
// OLED: METRONOME
// =====================================================

void drawMetronome() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  int16_t x1, y1;
  uint16_t w, h;

  // Title
  display.setTextSize(1);

  const char* title = "METRONOME";

  display.getTextBounds(
    title, 0, 0,
    &x1, &y1, &w, &h);

  display.setCursor(
    (SCREEN_WIDTH - w) / 2,
    2);

  display.println(title);

  // BPM
  display.setTextSize(2);

  char bpmText[12];

  snprintf(
    bpmText,
    sizeof(bpmText),
    "%d BPM",
    bpm);

  display.getTextBounds(
    bpmText, 0, 0,
    &x1, &y1, &w, &h);

  display.setCursor(
    (SCREEN_WIDTH - w) / 2,
    18);

  display.println(bpmText);

  // Four beats: active beat is filled.
  const int y = 52;
  const int positions[4] = { 32, 53, 74, 95 };

  for (int i = 0; i < 4; i++) {
    if (i == currentBeat) {
      display.fillCircle(
        positions[i], y, 6,
        SSD1306_WHITE);
    } else {
      display.drawCircle(
        positions[i], y, 6,
        SSD1306_WHITE);
    }
  }

  display.display();
}


// =====================================================
// MOTOR
// =====================================================

void startVibration() {
  digitalWrite(MOTOR_PIN, HIGH);

  motorOn = true;
  motorStartedAt = millis();
}

void updateVibration() {
  if (motorOn && millis() - motorStartedAt >= MOTOR_DURATION) {

    digitalWrite(MOTOR_PIN, LOW);

    motorOn = false;
  }
}

// =====================================================
// OLED: OFF
// =====================================================

void turnOffDisplay() {
  display.clearDisplay();
  display.display();
}
