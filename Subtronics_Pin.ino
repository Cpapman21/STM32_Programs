#include "STM32LowPower.h"
#include <Adafruit_NeoPixel.h>

#ifndef USER_BTN
#define USER_BTN pinNametoDigitalPin(PC13)
#endif

#define LED_PIN     PC15
#define LED_COUNT   4
#define BRIGHTNESS  60

const int Button   = PA0;
const int PPR      = PA4;
const int Char_Ind = PA3;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

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

// ===== Theme state =====
uint16_t hueBase = 0;          // for prismatic mode
uint8_t  eyePos  = 0;          // for cyclops pulse position
int8_t   eyeDir  = 1;          // bounce direction
uint32_t lastStepMs = 0;       // step timing for slower motion

// Forward decl
void Light_Init();
void goToSleep();

// ------------------------
// Debounced button ISR (FALLING because INPUT_PULLUP)
// ------------------------
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

// ------------------------
// Color helpers
// ------------------------
static inline uint32_t RGB(uint8_t r, uint8_t g, uint8_t b) {
  return strip.Color(r, g, b);
}
static inline uint32_t HSV(uint16_t h, uint8_t s, uint8_t v) {
  return strip.ColorHSV(h, s, v);
}

// Theme palette (tuned to look good at BRIGHTNESS 60)
static inline uint32_t SUB_YELLOW() { return RGB(255, 210, 30); } // logo-y yellow
static inline uint32_t SUB_GREEN()  { return RGB(  0, 255,  80); } // neon eye green
static inline uint32_t SUB_PURPLE() { return RGB(150,  20, 255); } // neon purple
static inline uint32_t DEEP_PURPLE(){ return RGB( 35,   0,  70); } // darker wash
static inline uint32_t CYAN_ICE()   { return RGB(  0, 220, 255); } // tesseract-ish
static inline uint32_t MAGENTA()    { return RGB(255,  60, 200); } // prismatic pop
static inline uint32_t WHITE_HOT()  { return RGB(255, 255, 255); }
static inline uint32_t OFF()        { return RGB(0,0,0); }

// Simple “additive-ish” brighten: returns brighter of two colors (cheap & safe)
static inline uint32_t overlay(uint32_t base, uint32_t top) {
  // We can't read current pixel reliably everywhere, so we do logic externally.
  // This is left as a placeholder if you want more advanced blending later.
  (void)base; (void)top;
  return top;
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

void loop() {
  // Handle debounced button event
  if (buttonEvent) {
    noInterrupts();
    buttonEvent = false;
    interrupts();

    mode_Count++;
    if (mode_Count > 3) mode_Count = 0;
  }

  // PPR behavior (your existing “special mode then off/sleep” logic)
  while (digitalRead(PPR) == LOW) {

    // If Char_Ind LOW -> red flicker
    if (digitalRead(Char_Ind) == LOW) {
      uint32_t now = millis();
      if (now - lastFrameMs >= FRAME_MS) {
        lastFrameMs = now;
        for (int i = 0; i < LED_COUNT; i++) {
          int heat = random(10, 255);
          strip.setPixelColor(i, strip.Color(heat, 0, 0));
        }
        strip.show();
      }
      greenOn = false;
    }

    // If Char_Ind HIGH -> blink green (non-blocking)
    else {
      uint32_t now = millis();
      if (now - lastGreenToggleMs >= GREEN_BLINK_MS) {
        lastGreenToggleMs = now;
        greenOn = !greenOn;

        uint32_t c = greenOn ? strip.Color(0, 155, 0) : strip.Color(0, 0, 0);
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, c);
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

  // ------------------------
  // Non-blocking updates for modes 0..2
  // ------------------------
  uint32_t now = millis();
  if (now - lastFrameMs >= FRAME_MS) {
    lastFrameMs = now;

    // =========================
    // MODE 0: "Cyclops Eye Pulse"
    // Yellow body glow with green "iris" pulse moving across pixels.
    // =========================
    if (mode_Count == 0) {
      // soft yellow base with slight breathing
      static uint8_t breathe = 0;
      static int8_t bdir = 1;

      breathe = (uint8_t)(breathe + bdir * 3);
      if (breathe > 220) { breathe = 220; bdir = -1; }
      if (breathe < 120) { breathe = 120; bdir =  1; }

      // move the green iris slower than frame
      if (now - lastStepMs >= 120) {
        lastStepMs = now;
        eyePos = (uint8_t)(eyePos + eyeDir);
        if (eyePos == (LED_COUNT - 1) || eyePos == 0) eyeDir = -eyeDir;
      }

      for (int i = 0; i < LED_COUNT; i++) {
        // base yellow (breathed)
        uint8_t r = (uint8_t)min<int>(255, 200 + (breathe / 6));
        uint8_t g = (uint8_t)min<int>(255, 140 + (breathe / 5));
        uint8_t b = (uint8_t)min<int>(255,  10 + (breathe / 30));
        strip.setPixelColor(i, RGB(r, g, b));
      }

      // iris pulse (bright green) at eyePos + faint halo around it
      strip.setPixelColor(eyePos, SUB_GREEN());
      if (LED_COUNT > 1) {
        int left  = (int)eyePos - 1;
        int right = (int)eyePos + 1;
        if (left >= 0)  strip.setPixelColor(left,  RGB(0, 140, 40));
        if (right < LED_COUNT) strip.setPixelColor(right, RGB(0, 140, 40));
      }

      strip.show();
    }

    // =========================
    // MODE 1: "Purple Invasion Laser"
    // Deep purple wash + neon green sweeps (like lasers/eye beams).
    // =========================
    else if (mode_Count == 1) {
      static uint8_t sweepPos = 0;
      static uint32_t lastSweepMs = 0;

      // purple base
      for (int i = 0; i < LED_COUNT; i++) {
        strip.setPixelColor(i, DEEP_PURPLE());
      }

      // step sweep position
      if (now - lastSweepMs >= 90) {
        lastSweepMs = now;
        sweepPos = (uint8_t)((sweepPos + 1) % LED_COUNT);
      }

      // main laser pixel
      strip.setPixelColor(sweepPos, RGB(0, 255, 90));

      // secondary faint laser tail
      int tail = (int)sweepPos - 1;
      if (tail < 0) tail += LED_COUNT;
      strip.setPixelColor(tail, RGB(0, 120, 35));

      // occasional purple flare
      if (random(0, 100) < 10) {
        int p = random(0, LED_COUNT);
        strip.setPixelColor(p, SUB_PURPLE());
      }

      strip.show();
    }

    // =========================
    // MODE 2: "Fractals Prism Shards"
    // Iridescent HSV shards + white/magenta spark hits.
    // =========================
    else if (mode_Count == 2) {
      // drifting prismatic hues
      hueBase += 260;

      for (int i = 0; i < LED_COUNT; i++) {
        // Make shards: each pixel has a different hue offset
        uint16_t h = hueBase + (uint16_t)(i * 11000);
        // higher saturation, medium-high value
        uint32_t c = HSV(h, 255, 160);
        strip.setPixelColor(i, c);
      }

      // “fracture flashes” — quick bright hits
      if (random(0, 100) < 25) {
        int p = random(0, LED_COUNT);
        strip.setPixelColor(p, WHITE_HOT());
      }
      if (random(0, 100) < 18) {
        int p = random(0, LED_COUNT);
        strip.setPixelColor(p, MAGENTA());
      }
      if (random(0, 100) < 12) {
        int p = random(0, LED_COUNT);
        strip.setPixelColor(p, CYAN_ICE());
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