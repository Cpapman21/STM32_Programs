#include "STM32LowPower.h"
#include <Adafruit_NeoPixel.h>

#ifndef USER_BTN
#define USER_BTN pinNametoDigitalPin(PC13)
#endif

#define LED_PIN     PC15
#define LED_COUNT   4
#define BRIGHTNESS  100

const int Button   = PA0;
const int PPR      = PA4;
const int Char_Ind = PA3;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

// ===== Debounce =====
static const uint32_t DEBOUNCE_DELAY_MS = 650;
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

// ===== Rainbow state =====
uint16_t hueBase = 0;

// Forward decl
void Light_Init();
void goToSleep();

// Debounced button ISR (FALLING because INPUT_PULLUP)
void onButtonIRQ() {
  uint32_t now = millis();
  if ((now - lastButtonIrqMs) >= DEBOUNCE_DELAY_MS) {
    lastButtonIrqMs = now;
    buttonEvent = true;  // tell loop() a debounced press happened
  }
} 

// Wake callback (called by STM32LowPower on wake)
void onWake() {
  // Keep this short: just prep state for loop
  mode_Count = 0;
  buttonEvent = false;
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(PPR, INPUT);
  pinMode(Char_Ind, INPUT);
  pinMode(Button, INPUT_PULLUP);

  Light_Init();

  LowPower.begin();

  // Wake from deep sleep on button
  LowPower.attachInterruptWakeup(Button, onWake, FALLING, SLEEP_MODE);

  // While awake, use our debounced IRQ to change modes
  attachInterrupt(digitalPinToInterrupt(Button), onButtonIRQ, FALLING);

  // Start asleep like your original
  goToSleep();
}

// Helpers for rainbow effects
static inline uint32_t HSV(uint16_t h, uint8_t s, uint8_t v) {
  return strip.ColorHSV(h, s, v);
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

  // PPR behavior (your “special mode then off/sleep” logic)
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
    }

    // If Char_Ind HIGH -> blink green (non-blocking)
    else if (digitalRead(Char_Ind) == HIGH) {
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

  // Mode 3: turn off + sleep
  if (mode_Count == 3) {
    for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, 0);
    strip.show();
    delay(500);
    goToSleep();
    return;
  }

  // Non-blocking LED updates for modes 0..2
  uint32_t now = millis();
  if (now - lastFrameMs >= FRAME_MS) {
    lastFrameMs = now;

    if (mode_Count == 0) {
      hueBase += 220; // speed
      for (int i = 0; i < LED_COUNT; i++) {
        uint16_t h = hueBase + (uint16_t)(i * (65535UL / LED_COUNT));
        strip.setPixelColor(i, HSV(h, 255, 255));
      }
      strip.show();
    }
    else if (mode_Count == 1) {
      // faint background
      hueBase += 140;
      for (int i = 0; i < LED_COUNT; i++) {
        uint16_t h = hueBase + (uint16_t)(i * 9000);
        strip.setPixelColor(i, HSV(h, 255, 40)); // dim base
      }
      // sparkles
      int sparkCount = 1 + random(1, 3); // 2-3 sparkles total (including the +1)
      for (int k = 0; k < sparkCount; k++) {
        int p = random(0, LED_COUNT);
        uint16_t h = (uint16_t)random(0, 65535);
        uint8_t v  = (uint8_t)random(150, 255);
        strip.setPixelColor(p, HSV(h, 255, v));
      }
      strip.show();
    }
    else if (mode_Count == 2) {
      for (int i = 0; i < LED_COUNT; i++) {
        uint16_t h = (uint16_t)random(0, 65535);
        uint8_t  s = (uint8_t)random(200, 255);
        uint8_t  v = (uint8_t)random(120, 255);
        strip.setPixelColor(i, HSV(h, s, v));
      }
      strip.show();
    }
  }
}
void goToSleep() {
  // Ensure wake source is set
  detachInterrupt(digitalPinToInterrupt(Button));
  LowPower.attachInterruptWakeup(Button, onWake, FALLING, SLEEP_MODE);

  // Go sleep
  LowPower.deepSleep();

  // After waking up, reattach debounced interrupt for mode changes
  attachInterrupt(digitalPinToInterrupt(Button), onButtonIRQ, FALLING);
}

void Light_Init() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
}
