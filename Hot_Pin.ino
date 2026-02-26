#include "STM32LowPower.h"
#include <Adafruit_NeoPixel.h>

#ifndef USER_BTN
#define USER_BTN pinNametoDigitalPin(PC13)
#endif

#define LED_PIN     PC15
#define LED_COUNT   4
#define BRIGHTNESS  255

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

// ===== Flame state =====
uint8_t heat[LED_COUNT];          // per-pixel heat
uint32_t lastEmberMs = 0;         // ember timing
uint8_t emberPos = 0;             // drifting ember index
uint8_t emberHeat = 0;            // ember intensity

// Forward decl
void Light_Init();
void goToSleep();

// ------------------------
// Debounced button ISR
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
// Flame palette mapping
// temperature 0..255 -> color (black->red->orange->yellow->white)
// Tuned for "HOT rave pin" look.
// ------------------------
static inline uint32_t flameColor(uint8_t t) {
  // Boost mid/highs so it reads "hot" even at BRIGHTNESS 60
  uint8_t x = t;

  // 0..85: red up
  // 86..170: red max, green up (orange->yellow)
  // 171..255: red+green max, blue up (yellow->white)
  if (x <= 85) {
    uint8_t r = x * 3;          // 0..255
    return strip.Color(r, 0, 0);
  } else if (x <= 170) {
    x -= 85;
    uint8_t g = x * 3;          // 0..255
    return strip.Color(255, g, 0);
  } else {
    x -= 170;
    uint8_t b = (x * 3);        // 0..255
    return strip.Color(255, 255, b);
  }
}

// ------------------------
// Core 1D flame step (non-blocking)
// Cooling/Sparking control the "style".
// With only 4 pixels, we bias effects to be readable.
// ------------------------
void flameStep(uint8_t Cooling, uint8_t Sparking) {
  // 1) cool down every cell a little
  for (int i = 0; i < LED_COUNT; i++) {
    uint8_t cooldown = random(0, ((Cooling * 10) / LED_COUNT) + 2);
    heat[i] = (heat[i] > cooldown) ? (heat[i] - cooldown) : 0;
  }

  // 2) heat drifts up + diffuses
  for (int k = LED_COUNT - 1; k >= 1; k--) {
    // weighted average of lower cells (keeps flame bottom-hot)
    heat[k] = (uint8_t)((heat[k - 1] * 2 + heat[k]) / 3);
  }

  // 3) random sparks near the bottom
  if (random(0, 255) < Sparking) {
    int y = random(0, min(2, LED_COUNT)); // spark in bottom 1-2 pixels
    uint8_t add = random(160, 255);
    uint16_t boosted = heat[y] + add;
    heat[y] = (boosted > 255) ? 255 : (uint8_t)boosted;
  }

  // 4) map heat to pixels
  for (int j = 0; j < LED_COUNT; j++) {
    strip.setPixelColor(j, flameColor(heat[j]));
  }
  strip.show();
}

// ------------------------
// Ember overlay (Mode 2)
// A single "ember" that drifts upward and fades.
// ------------------------
void emberStep(uint32_t now) {
  // spawn / move ember every so often
  if (now - lastEmberMs > 110) {
    lastEmberMs = now;

    // fade ember
    if (emberHeat > 10) emberHeat -= 10;
    else emberHeat = 0;

    // drift upward
    if (emberPos < (LED_COUNT - 1)) {
      emberPos++;
    } else {
      // respawn near bottom occasionally
      if (random(0, 100) < 35) {
        emberPos = 0;
        emberHeat = random(120, 220);
      } else {
        emberHeat = 0;
      }
    }

    // sometimes re-ignite ember mid-way for extra pop
    if (random(0, 100) < 12) {
      emberHeat = max<uint8_t>(emberHeat, random(140, 240));
    }
  }

  // overlay ember as a warmer orange sparkle (additive-ish)
  if (emberHeat > 0) {
    // read current pixel and add some orange/white
    // (NeoPixel lib doesn't expose readback reliably on all targets,
    //  so just "punch in" a bright ember color on top)
    uint8_t r = min<int>(255, 200 + emberHeat / 2);
    uint8_t g = min<int>(255, 60 + emberHeat / 2);
    uint8_t b = min<int>(255, emberHeat / 6);
    strip.setPixelColor(emberPos, strip.Color(r, g, b));
    strip.show();
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(PPR, INPUT);
  pinMode(Char_Ind, INPUT);
  pinMode(Button, INPUT_PULLUP);

  Light_Init();

  // init flame state
  for (int i = 0; i < LED_COUNT; i++) heat[i] = 0;
  emberPos = 0;
  emberHeat = 0;
  lastEmberMs = millis();

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

  // PPR behavior (keep your original logic)
  while (digitalRead(PPR) == LOW) {

    // If Char_Ind LOW -> red flicker
    if (digitalRead(Char_Ind) == LOW) {
      uint32_t now = millis();
      if (now - lastFrameMs >= FRAME_MS) {
        lastFrameMs = now;
        for (int i = 0; i < LED_COUNT; i++) {
          int heatRand = random(10, 255);
          strip.setPixelColor(i, strip.Color(heatRand, 0, 0));
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

  // Non-blocking flame updates for modes 0..2
  uint32_t now = millis();
  if (now - lastFrameMs >= FRAME_MS) {
    lastFrameMs = now;

    // MODE 0: Campfire simmer (smooth, slower)
    if (mode_Count == 0) {
      // more cooling, fewer sparks
      flameStep(/*Cooling=*/85, /*Sparking=*/55);
    }

    // MODE 1: Torch / aggressive flame (fast, tall, chaotic)
    else if (mode_Count == 1) {
      // less cooling, more sparks
      flameStep(/*Cooling=*/55, /*Sparking=*/120);

      // occasional "flare" (a quick hot burst at the bottom)
      if (random(0, 100) < 10) {
        heat[0] = min<int>(255, heat[0] + random(80, 150));
      }
    }

    // MODE 2: Inferno + embers (hot, punchy, with drifting ember)
    else if (mode_Count == 2) {
      flameStep(/*Cooling=*/45, /*Sparking=*/150);

      // extra top heat sometimes to look like licking flame
      if (random(0, 100) < 18) {
        heat[LED_COUNT - 1] = min<int>(255, heat[LED_COUNT - 1] + random(30, 90));
      }

      emberStep(now);
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