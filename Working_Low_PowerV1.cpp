#include "STM32LowPower.h"

// Blink sequence number
// Declare it volatile since it's incremented inside an interrupt
volatile int repetitions = 1;

// Pin used to trigger a wakeup
#ifndef USER_BTN
#define USER_BTN pinNametoDigitalPin(PC13)
#endif

const int pin = PC13;
int LED_Pin = PA5;
int Charging = PA8;
volatile int mode_Count = 0;

void setup() {

  pinMode(LED_Pin, OUTPUT);
  pinMode(Charging,INPUT_PULLUP);
  pinMode(pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(Charging),Charge_Detect,FALLING);
  LowPower.begin();
  // Attach a wakeup interrupt on pin, calling repetitionsIncrease when the device is woken up
  // Last parameter (LowPowerMode) should match with the low power state used: in this example LowPower.sleep()
  LowPower.attachInterruptWakeup(pin, repetitionsIncrease, FALLING, SLEEP_MODE);
  //LowPower.attachInterruptWakeup(PA9,Charging,RISING,SLEEP_MODE);
  LowPower.deepSleep();

}

void loop() {

  while(mode_Count <= 3) {
    if(mode_Count == 0) {
      digitalWrite(LED_Pin, HIGH);
      delay(1000);
      digitalWrite(LED_Pin, LOW);
      delay(1000);
    }
    if(mode_Count == 1) {
      digitalWrite(LED_Pin, HIGH);
      delay(200);
      digitalWrite(LED_Pin, LOW);
      delay(200);
    }

    if(mode_Count == 2) {
      digitalWrite(LED_Pin, HIGH);
      delay(500);
      digitalWrite(LED_Pin, LOW);
      delay(500);
    }

    if(mode_Count == 10){
      pinMode(Charging,INPUT);
      delayMicroseconds(10000);
      while(digitalRead(Charging) == LOW) {
        digitalWrite(LED_Pin, HIGH);
        delay(100);
        digitalWrite(LED_Pin, LOW);
        delay(100);
      }
      mode_Count = 3;
      delay(250);
      attachInterrupt(digitalPinToInterrupt(Charging),Charge_Detect,FALLING);

    }

    if(mode_Count == 3) {
      delay(100);
      LowPower.attachInterruptWakeup(pin, repetitionsIncrease, FALLING, SLEEP_MODE);
      LowPower.deepSleep();
    }
  }
}

void repetitionsIncrease() {
  // This function will be called once on device wakeup
  // You can do some little operations here (like changing variables which will be used in the loop)
  // Remember to avoid calling delay() and long running functions since this functions executes in interrupt context
  mode_Count = 0;
  pinMode(pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pin),mode_test,FALLING);
  repetitions ++;
}

void mode_test(){
  mode_Count++;
}

void Charge_Detect(){
  mode_Count = 10;
}
