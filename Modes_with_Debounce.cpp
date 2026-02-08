#include "STM32LowPower.h"
#include <Adafruit_NeoPixel.h>

#ifndef USER_BTN
#define USER_BTN pinNametoDigitalPin(PC13)
#endif

#define LED_PIN     PC15
#define LED_COUNT   4
#define BRIGHTNESS  50

const int Button = PA0;
const int PPR    = PA4;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

// ===== Debounce =====
static const uint32_t DEBOUNCE_DELAY_MS = 1000;
volatile uint32_t lastButtonIrqMs = 0;
volatile bool buttonEvent = false;

// ===== Modes =====
volatile int mode_Count = 0;  // 0..3

// ===== Animation timing =====
uint32_t lastFrameMs = 0;
static const uint32_t FRAME_MS = 35;

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
  if (digitalRead(PPR) == LOW) {
    // simple debounce / confirm for PPR
    delay(500);
    if (digitalRead(PPR) == LOW) {
      // show PPR effect while held low
      uint32_t now = millis();
      if (now - lastFrameMs >= FRAME_MS) {
        lastFrameMs = now;
        for (int i = 0; i < LED_COUNT; i++) {
          int heat = random(100, 255);
          int r = (heat * 4) / 150;
          int g = (heat * 2) / 25;
          int b = 10;
          strip.setPixelColor(i, strip.Color(r, g, b));
        }
        strip.show();
      }
      return; // keep loop responsive
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
      // Warm flame
      for (int i = 0; i < LED_COUNT; i++) {
        int heat = random(100, 255);
        int r = 50;
        int g = (heat * 2) / 100;
        int b = 0;
        strip.setPixelColor(i, strip.Color(r, g, b));
      }
      strip.show();
    }
    else if (mode_Count == 1) {
      // Blue-ish
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
      // Another variant (tweak as you like)
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
