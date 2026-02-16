#include "STM32LowPower.h"
#include <Adafruit_NeoPixel.h>

#ifndef USER_BTN
#define USER_BTN pinNametoDigitalPin(PC13)
#endif

#define LED_PIN     PC15
#define LED_COUNT   4
#define BRIGHTNESS  125

const int Button   = PA0;
const int PPR      = PA4;
const int Char_Ind = PA3;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ===== Debounce =====
static const uint32_t DEBOUNCE_DELAY_MS = 500;
volatile uint32_t lastButtonIrqMs = 0;
volatile bool buttonEvent = false;

// ===== Modes =====
volatile int mode_Count = 0;  // 0..3

// ===== Animation timing =====
uint32_t lastFrameMs = 0;
static const uint32_t FRAME_MS = 35;

// ===== Green blink timing (when Char_Ind HIGH in PPR loop) =====
static const uint32_t GREEN_BLINK_MS = 350;
uint32_t lastGreenToggleMs = 0;
bool greenOn = false;

// Forward decl
void Light_Init();
void goToSleep();

// Debounced button ISR (FALLING because INPUT_PULLUP)
void onButtonIRQ() {
  uint32_t now = millis();
  if ((now - lastButtonIrqMs) >= DEBOUNCE_DELAY_MS) {
    lastButtonIrqMs = now;
    buttonEvent = true;
  }
}

// Wake callback (called by STM32LowPower on wake)
void onWake() {
  mode_Count = 0;
  buttonEvent = false;
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(PPR, INPUT);
  pinMode(Char_Ind, INPUT);        // <-- make sure this is set once
  pinMode(Button, INPUT_PULLUP);

  Light_Init();

  LowPower.begin();

  LowPower.attachInterruptWakeup(Button, onWake, FALLING, SLEEP_MODE);
  attachInterrupt(digitalPinToInterrupt(Button), onButtonIRQ, FALLING);

  goToSleep();
}

void loop() {
  // Handle debounced button event
  if (buttonEvent) {
    noInterrupts();
    buttonEvent = false;
    interrupts();

    mode_Count++;
    if (mode_Count > 3) mode_Count = 0;
  }

  // =========================
  // PPR loop: voltage level / converter status
  // =========================
  while (digitalRead(PPR) == LOW) {

    // If Char_Ind LOW -> red flicker
    if (digitalRead(Char_Ind) == LOW) {
      uint32_t now = millis();
      if (now - lastFrameMs >= FRAME_MS) {
        lastFrameMs = now;
        for (int i = 0; i < LED_COUNT; i++) {
          int heat = random(10, 255);
          int r = heat;
          int g = 0;
          int b = 0;
          strip.setPixelColor(i, strip.Color(r, g, b));
        }
        strip.show();
      }

      // keep green blink state reset while not in green mode
      greenOn = false;
      // (optional) don't force LEDs off here; red flicker is already drawing
    }

    // If Char_Ind HIGH -> blink green (non-blocking)
    else if(digitalRead(Char_Ind) == HIGH) { // Char_Ind == HIGH
      uint32_t now = millis();

      if (now - lastGreenToggleMs >= GREEN_BLINK_MS) {
        lastGreenToggleMs = now;
        greenOn = !greenOn;

        uint32_t c = greenOn ? strip.Color(0, 155, 0) : 0;
        for (int i = 0; i < LED_COUNT; i++) {
          strip.setPixelColor(i, c);
        }
        strip.show();
      }
    }
  }

    // Important: do NOT return; we want to stay responsive and keep looping
    // until PPR goes HIGH (meaning exit this "converter status" mode

  // Mode 3: turn off + sleep
  if (mode_Count == 3) {
    for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, 0);
    strip.show();
    delay(20);
    goToSleep();
    return;
  }

  // Non-blocking LED updates for modes 0..2
  uint32_t now = millis();
  if (now - lastFrameMs >= FRAME_MS) {
    lastFrameMs = now;

    if (mode_Count == 0) {
      for (int i = 0; i < LED_COUNT; i++) {
        int heat = random(100, 255);
        int r = 0;
        int g = (heat * 2) / 100;
        int b = 100;
        strip.setPixelColor(i, strip.Color(r, g, b));
      }
      strip.show();
    }
    else if (mode_Count == 1) {
      for (int i = 0; i < LED_COUNT; i++) {
        int heat = random(100, 255);
        int r = 0;
        int g = 10;
        int b = heat;
        strip.setPixelColor(i, strip.Color(r, g, b));
      }
      strip.show();
    }
    else if (mode_Count == 2) {
      for (int i = 0; i < LED_COUNT; i++) {
        int heat = random(100, 255);
        int r = heat / 6;
        int g = 0;
        int b = heat;
        strip.setPixelColor(i, strip.Color(r, g, b));
      }
      strip.show();
    }
  }
}

void goToSleep() {
  detachInterrupt(digitalPinToInterrupt(Button));
  LowPower.attachInterruptWakeup(Button, onWake, FALLING, SLEEP_MODE);

  LowPower.deepSleep();

  attachInterrupt(digitalPinToInterrupt(Button), onButtonIRQ, FALLING);
}

void Light_Init() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
}
