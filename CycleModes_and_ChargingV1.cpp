#include "STM32LowPower.h"
#include <Adafruit_NeoPixel.h>

// Blink sequence number
// Declare it volatile since it's incremented inside an interrupt
volatile int repetitions = 1;

// Pin used to trigger a wakeup
#ifndef USER_BTN
#define USER_BTN pinNametoDigitalPin(PC13)
#endif

#define LED_PIN PC15

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT  4

// NeoPixel brightness, 0 (min) to 255 (max)
#define BRIGHTNESS 50 // Set BRIGHTNESS to about 1/5 (max = 255)

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

const int Button = PA0;
int Charging_Ind = PA3;
int PPR = PA4;
volatile int mode_Count = 0; 

void setup() {

  pinMode(LED_PIN, OUTPUT);
  pinMode(PPR,INPUT_PULLUP);
  pinMode(Button, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PPR),Charge_Detect,FALLING);
  Light_Init();
  LowPower.begin();
  // Attach a wakeup interrupt on pin, calling repetitionsIncrease when the device is woken up
  // Last parameter (LowPowerMode) should match with the low power state used: in this example LowPower.sleep()
  LowPower.attachInterruptWakeup(Button, repetitionsIncrease, FALLING, SLEEP_MODE);
  //LowPower.attachInterruptWakeup(PA9,Charging,RISING,SLEEP_MODE);
  LowPower.deepSleep();

}

void loop() {
  Light_Init();
  while(mode_Count <= 3) {
    if(mode_Count == 0) {
      for (int i = 0; i < LED_COUNT; i++) {
        int heat = random(100, 255);

        int r = 50;
        int g = (heat * 2) / 100;
        int b = heat;

        strip.setPixelColor(i, strip.Color(r, g, b));
    }

    strip.show();
    delay(100);
    }
    if(mode_Count == 1) {
      for (int i = 0; i < LED_COUNT; i++) {
        int heat = random(100, 255);

        int r = (heat * 2) / 100;
        int g = 100;
        int b = heat;;

        strip.setPixelColor(i, strip.Color(r, g, b));
      }

    strip.show();
    delay(100);
    }

    if(mode_Count == 2) {
      for (int i = 0; i < LED_COUNT; i++) {
      int heat = random(100, 255);

      int r = (heat * 2) / 100;;
      int g = 250;
      int b = (heat * 2) / 100;

      strip.setPixelColor(i, strip.Color(r, g, b));
      }

    strip.show();
    delay(100);
    }

    if(mode_Count == 10){
      pinMode(PPR,INPUT);
      delayMicroseconds(10000);
      while(digitalRead(PPR) == LOW) {
        for (int i = 0; i < LED_COUNT; i++) {
          int heat = random(100, 255);

          int r = (heat * 4) / 100;
          int g = (heat * 2) / 100;
          int b = (heat * 6) / 100;

          strip.setPixelColor(i, strip.Color(r, g, b));
        }
        strip.show();
        delay(100);  
      }
      mode_Count = 3;
      delay(250);
      attachInterrupt(digitalPinToInterrupt(PPR),Charge_Detect,FALLING);

    }

    if(mode_Count == 3) {
      delay(100);
      LowPower.attachInterruptWakeup(Button, repetitionsIncrease, FALLING, SLEEP_MODE);
      LowPower.deepSleep();
    }
  }
}

void repetitionsIncrease() {
  // This function will be called once on device wakeup
  // You can do some little operations here (like changing variables which will be used in the loop)
  // Remember to avoid calling delay() and long running functions since this functions executes in interrupt context
  mode_Count = 0;
  pinMode(Button, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(Button),mode_test,FALLING);
  Light_Init();
  repetitions ++;
}

void mode_test(){
  mode_Count++;
}

void Charge_Detect(){
  mode_Count = 10;
}


void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}

void whiteOverRainbow(int whiteSpeed, int whiteLength) {

  if(whiteLength >= strip.numPixels()) whiteLength = strip.numPixels() - 1;

  int      head          = whiteLength - 1;
  int      tail          = 0;
  int      loops         = 3;
  int      loopNum       = 0;
  uint32_t lastTime      = millis();
  uint32_t firstPixelHue = 0;

  for(;;) { // Repeat forever (or until a 'break' or 'return')
    for(int i=0; i<strip.numPixels(); i++) {  // For each pixel in strip...
      if(((i >= tail) && (i <= head)) ||      //  If between head & tail...
         ((tail > head) && ((i >= tail) || (i <= head)))) {
        strip.setPixelColor(i, strip.Color(0, 0, 0, 255)); // Set white
      } else {                                             // else set rainbow
        int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
        strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
      }
    }

    strip.show(); // Update strip with new contents
    // There's no delay here, it just runs full-tilt until the timer and
    // counter combination below runs out.

    firstPixelHue += 40; // Advance just a little along the color wheel

    if((millis() - lastTime) > whiteSpeed) { // Time to update head/tail?
      if(++head >= strip.numPixels()) {      // Advance head, wrap around
        head = 0;
        if(++loopNum >= loops) return;
      }
      if(++tail >= strip.numPixels()) {      // Advance tail, wrap around
        tail = 0;
      }
      lastTime = millis();                   // Save time of last movement
    }
  }
}

void pulseWhite(uint8_t wait) {
  for(int j=0; j<256; j++) { // Ramp up from 0 to 255
    // Fill entire strip with white at gamma-corrected brightness level 'j':
    strip.fill(strip.Color(0, 0, 0, strip.gamma8(j)));
    strip.show();
    delay(wait);
  }

  for(int j=255; j>=0; j--) { // Ramp down from 255 to 0
    strip.fill(strip.Color(0, 0, 0, strip.gamma8(j)));
    strip.show();
    delay(wait);
  }
}

void rainbowFade2White(int wait, int rainbowLoops, int whiteLoops) {
  int fadeVal=0, fadeMax=100;

  // Hue of first pixel runs 'rainbowLoops' complete loops through the color
  // wheel. Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to rainbowLoops*65536, using steps of 256 so we
  // advance around the wheel at a decent clip.
  for(uint32_t firstPixelHue = 0; firstPixelHue < rainbowLoops*65536;
    firstPixelHue += 256) {

    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...

      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      uint32_t pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());

      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the three-argument variant, though the
      // second value (saturation) is a constant 255.
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue, 255,
        255 * fadeVal / fadeMax)));
    }

    strip.show();
    delay(wait);

    if(firstPixelHue < 65536) {                              // First loop,
      if(fadeVal < fadeMax) fadeVal++;                       // fade in
    } else if(firstPixelHue >= ((rainbowLoops-1) * 65536)) { // Last loop,
      if(fadeVal > 0) fadeVal--;                             // fade out
    } else {
      fadeVal = fadeMax; // Interim loop, make sure fade is at max
    }
  }

  for(int k=0; k<whiteLoops; k++) {
    for(int j=0; j<256; j++) { // Ramp up 0 to 255
      // Fill entire strip with white at gamma-corrected brightness level 'j':
      strip.fill(strip.Color(0, 0, 0, strip.gamma8(j)));
      strip.show();
    }
    delay(1000); // Pause 1 second
    for(int j=255; j>=0; j--) { // Ramp down 255 to 0
      strip.fill(strip.Color(0, 0, 0, strip.gamma8(j)));
      strip.show();
    }
  }

  delay(500); // Pause 1/2 second
}

void Light_Init() {
    strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
    strip.show();            // Turn OFF all pixels ASAP
    strip.setBrightness(BRIGHTNESS);
}
