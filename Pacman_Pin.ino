 #include <Adafruit_NeoPixel.h>

#ifndef USER_BTN
#define USER_BTN pinNametoDigitalPin(PC13)
#endif

// ================= CONFIG =================
#define LED_PIN     PC15
#define LED_COUNT   4
#define BRIGHTNESS  30

const int Button   = PA0;
const int PPR      = PA4;
const int Char_Ind = PA3;

// Discrete RGB LED pins
const int LED_R = PA5;
const int LED_G = PA7;
const int LED_B = PA6;

// Set true if your LEDs are active LOW
static const bool DISCRETE_ACTIVE_LOW = false;

// ==========================================

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ===== Debounce =====
static const uint32_t DEBOUNCE_DELAY_MS = 650;
volatile uint32_t lastButtonIrqMs = 0;
volatile bool buttonEvent = false;

// ===== Modes =====
volatile int mode_Count = 0;

// ===== Timing =====
uint32_t lastFrameMs = 0;
static const uint32_t FRAME_MS = 35;

// ===== Discrete LED Scroll =====
uint32_t lastRgbStepMs = 0;
uint8_t  rgbStep = 0;

// ===== Green blink (PPR) =====
static const uint32_t GREEN_BLINK_MS = 350;
uint32_t lastGreenToggleMs = 0;
bool greenOn = false;

// ================= COLORS =================

uint32_t pacYellow() { return strip.Color(255, 210, 20); }
uint32_t mazeBlue()  { return strip.Color(0, 18, 70); }

uint32_t blinkyRed() { return strip.Color(255, 0, 0); }
uint32_t pinkyPink() { return strip.Color(255, 90, 180); }
uint32_t inkyCyan()  { return strip.Color(0, 220, 255); }
uint32_t clydeOrng() { return strip.Color(255, 140, 0); }

uint32_t pelletWhite(){ return strip.Color(255,255,255); }

// ==========================================

void Light_Init();
void goToSleep();

// ================= BUTTON ISR =================

void onButtonIRQ() {
  uint32_t now = millis();

  if ((now - lastButtonIrqMs) >= DEBOUNCE_DELAY_MS) {
    lastButtonIrqMs = now;
    buttonEvent = true;
  }
}

void onWake() {
  mode_Count = 0;
  buttonEvent = false;
}

// ================= DISCRETE LED CONTROL =================

void setDiscreteRGB(bool r, bool g, bool b) {

  if (!DISCRETE_ACTIVE_LOW) {
    digitalWrite(LED_R, r ? HIGH : LOW);
    digitalWrite(LED_G, g ? HIGH : LOW);
    digitalWrite(LED_B, b ? HIGH : LOW);
  } 
  else {
    digitalWrite(LED_R, r ? LOW : HIGH);
    digitalWrite(LED_G, g ? LOW : HIGH);
    digitalWrite(LED_B, b ? LOW : HIGH);
  }
}

void allOffDiscrete() {
  setDiscreteRGB(false,false,false);
}

// Continuous scrolling RGB pattern
void updateModeRgbIndicator(uint32_t now) {

  if (mode_Count == 3) {
    allOffDiscrete();
    return;
  }

  uint32_t interval;

  if (mode_Count == 0) interval = 120;
  else if (mode_Count == 1) interval = 170;
  else interval = 90;

  if (now - lastRgbStepMs < interval) return;

  lastRgbStepMs = now;

  rgbStep = (rgbStep + 1) % 7;

  switch (rgbStep) {

    case 0: setDiscreteRGB(true,  false, false); break; // R
    case 1: setDiscreteRGB(false, true,  false); break; // G
    case 2: setDiscreteRGB(false, false, true ); break; // B

    case 3: setDiscreteRGB(true,  true,  false); break; // RG
    case 4: setDiscreteRGB(false, true,  true ); break; // GB
    case 5: setDiscreteRGB(true,  false, true ); break; // RB

    case 6: setDiscreteRGB(true,  true,  true ); break; // RGB
  }
}

// ================= SETUP =================

void setup() {

  pinMode(LED_PIN, OUTPUT);
  pinMode(PPR, INPUT);
  pinMode(Char_Ind, INPUT);
  pinMode(Button, INPUT_PULLUP);

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  allOffDiscrete();

  Light_Init();

  LowPower.begin();

  LowPower.attachInterruptWakeup(Button, onWake, FALLING, SLEEP_MODE);

  attachInterrupt(digitalPinToInterrupt(Button),
                  onButtonIRQ,
                  FALLING);

  goToSleep();
}

// ================= LOOP =================

void loop() {

  // Handle button press
  if (buttonEvent) {

    noInterrupts();
    buttonEvent = false;
    interrupts();

    mode_Count++;

    if (mode_Count > 3) mode_Count = 0;
  }

  uint32_t now = millis();

  // Update RGB scroll
  updateModeRgbIndicator(now);

  // -------- PPR MODE --------
  while (digitalRead(PPR) == LOW) {

    now = millis();
    updateModeRgbIndicator(now);

    if (digitalRead(Char_Ind) == LOW) {

      if (now - lastFrameMs >= FRAME_MS) {

        lastFrameMs = now;

        for (int i=0;i<LED_COUNT;i++) {

          int heat = random(50,255);
          strip.setPixelColor(i, strip.Color(heat,0,0));
        }

        strip.show();
      }

      greenOn = false;
    }

    else {

      if (now - lastGreenToggleMs >= GREEN_BLINK_MS) {

        lastGreenToggleMs = now;
        greenOn = !greenOn;

        uint32_t c = greenOn ?
          strip.Color(0,155,0) :
          strip.Color(0,0,0);

        for (int i=0;i<LED_COUNT;i++)
          strip.setPixelColor(i,c);

        strip.show();
      }
    }
  }

  // -------- MODE 3: SLEEP --------
  if (mode_Count == 3) {

    for (int i=0;i<LED_COUNT;i++)
      strip.setPixelColor(i,0);

    strip.show();

    allOffDiscrete();

    delay(250);

    goToSleep();

    return;
  }

  // -------- ANIMATIONS --------
  if (now - lastFrameMs >= FRAME_MS) {

    lastFrameMs = now;

    // MODE 0: PACMAN CHASE
    if (mode_Count == 0) {

      static int pacPos = 0;
      static uint32_t lastStepMs = 0;

      for (int i=0;i<LED_COUNT;i++)
        strip.setPixelColor(i, mazeBlue());

      if (now - lastStepMs >= 120) {

        lastStepMs = now;
        pacPos = (pacPos+1)%LED_COUNT;
      }

      strip.setPixelColor(pacPos, pacYellow());

      strip.show();
    }

    // MODE 1: GHOST PARADE
    else if (mode_Count == 1) {

      static uint8_t phase = 0;
      static uint32_t lastPhaseMs = 0;

      if (now - lastPhaseMs >= 180) {

        lastPhaseMs = now;
        phase++;
      }

      uint32_t ghosts[4] =
      {
        blinkyRed(),
        pinkyPink(),
        inkyCyan(),
        clydeOrng()
      };

      for (int i=0;i<LED_COUNT;i++) {

        strip.setPixelColor(i,
          ghosts[(i+phase)&3]);
      }

      strip.show();
    }

    // MODE 2: POWER PELLET
    else if (mode_Count == 2) {

      static bool pelletOn=false;
      static uint32_t lastPelletMs=0;

      if (now - lastPelletMs >= 140) {

        lastPelletMs = now;
        pelletOn = !pelletOn;
      }

      uint32_t base = pelletOn ?
        pelletWhite() :
        strip.Color(0,0,200);

      for (int i=0;i<LED_COUNT;i++)
        strip.setPixelColor(i,base);

      if (random(0,100)<35) {

        uint32_t ghosts[4]={
          blinkyRed(),
          pinkyPink(),
          inkyCyan(),
          clydeOrng()
        };

        int p=random(0,LED_COUNT);

        strip.setPixelColor(p,
          ghosts[random(0,4)]);
      }

      strip.show();
    }
  }
}

// ================= SLEEP =================

void goToSleep() {

  detachInterrupt(digitalPinToInterrupt(Button));

  LowPower.attachInterruptWakeup(Button,
                                 onWake,
                                 FALLING,
                                 SLEEP_MODE);

  LowPower.deepSleep();

  attachInterrupt(digitalPinToInterrupt(Button),
                  onButtonIRQ,
                  FALLING);
}

// ================= INIT =================

void Light_Init() {

  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
}