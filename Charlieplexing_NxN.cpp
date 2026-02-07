#include <Arduino.h>


const int L1 = PA5;
const int L2 = PA6;
const int L3 = PA7;
const int L4 = PA8;
const int L5 = PA11;

// ================= Charlieplex (generic Arduino) =================

#ifndef CHARLIE_MAX_PINS
#define CHARLIE_MAX_PINS 8                 // raise if you want (watch RAM)
#endif

#ifndef CHARLIE_MAX_LEDS
#define CHARLIE_MAX_LEDS (CHARLIE_MAX_PINS * (CHARLIE_MAX_PINS - 1))
#endif

struct Charlieplex {
  uint8_t pins[CHARLIE_MAX_PINS];
  uint8_t nPins = 0;

  // For each LED index: which pin is VCC and which is GND (indices into pins[])
  uint8_t map[CHARLIE_MAX_LEDS][2];
  uint8_t nLeds = 0;

  // Optional frame buffer: 1 = LED should be on
  bool frame[CHARLIE_MAX_LEDS];

  void begin(const uint8_t *arduinoPins, uint8_t n) {
    if (n < 2) return;
    if (n > CHARLIE_MAX_PINS) n = CHARLIE_MAX_PINS;

    nPins = n;
    for (uint8_t i = 0; i < n; i++) pins[i] = arduinoPins[i];

    // Build LED map: ordered pairs (i, j) with i != j
    uint8_t k = 0;
    for (uint8_t i = 0; i < nPins; i++) {
      for (uint8_t j = 0; j < nPins; j++) {
        if (i == j) continue;
        map[k][0] = i; // VCC pin index
        map[k][1] = j; // GND pin index
        k++;
      }
    }
    nLeds = k;

    clear();
    allOff();
  }

  uint8_t ledCount() const { return nLeds; }

  void clear() {
    for (uint8_t i = 0; i < nLeds; i++) frame[i] = false;
  }

  void set(uint8_t led, bool on) {
    if (led >= nLeds) return;
    frame[led] = on;
  }

  // Tri-state all pins (INPUT) so nothing conducts.
  void allOff() {
    for (uint8_t i = 0; i < nPins; i++) {
      pinMode(pins[i], INPUT);          // hi-Z
      // Avoid INPUT_PULLUP here; we want truly off.
      // digitalWrite(pins[i], LOW);    // not needed, but harmless on most cores
    }
  }

  // Turn on a single LED by index (one step of the multiplex scan)
  void ledOn(uint8_t led) {
    if (led >= nLeds) return;

    uint8_t vccIdx = map[led][0];
    uint8_t gndIdx = map[led][1];

    // First tri-state everything
    allOff();

    // Configure the two pins:
    // VCC: OUTPUT HIGH
    // GND: OUTPUT LOW
    pinMode(pins[vccIdx], OUTPUT);
    digitalWrite(pins[vccIdx], HIGH);

    pinMode(pins[gndIdx], OUTPUT);
    digitalWrite(pins[gndIdx], LOW);
  }

  // Scan the frame buffer. Call this *often* (e.g., every loop()).
  // onTimeUs controls brightness (higher = brighter but more flicker risk)
  void show(uint16_t onTimeUs = 500) {
    // For persistence of vision, we light LEDs one at a time briefly.
    for (uint8_t i = 0; i < nLeds; i++) {
      if (!frame[i]) continue;
      ledOn(i);
      delayMicroseconds(onTimeUs);
    }
    allOff();
  }

  // Convenience: pulse one LED for ms (blocking). Good for testing only.
  void pulse(uint8_t led, uint16_t ms) {
    const uint32_t endAt = millis() + ms;
    while (millis() < endAt) {
      ledOn(led);
      delayMicroseconds(800);
      allOff();
      delayMicroseconds(200);
    }
  }
};

// ================= Example usage =================

Charlieplex cp;

// Put your Arduino pin numbers here (works on any board)
const uint8_t PINS[] = { L1,L2,L3,L4,L5 }; // 3 pins -> 3*(3-1)=6 LEDs

void setup() {
  cp.begin(PINS, sizeof(PINS));

  // Example: turn all LEDs on in the frame buffer
  for (uint8_t i = 0; i < cp.ledCount(); i++) {
    cp.set(i, true);
  }
}

void loop() {
  // Keep scanning to display the current frame buffer
  cp.show(1000);  

  // Example animation: chase (non-blocking-ish)
  //static uint32_t last = 0;
  //static uint8_t idx = 0;

  //if (millis() - last > 120) {
    //last = millis();
    //cp.clear();
   // cp.set(idx, true);
   //idx = (idx + 1) % cp.ledCount();
  //}
}
