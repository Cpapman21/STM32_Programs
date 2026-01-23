#include "STM32LowPower.h"

// Blink sequence number
// Declare it volatile since it's incremented inside an interrupt
volatile int repetitions = 1;

// Pin used to trigger a wakeup
#ifndef USER_BTN
#define USER_BTN pinNametoDigitalPin(PC13)
#endif

//const int pin = PC13;
const int pin = PA0;
//int int LED_Pin = PA5;
int LED_Pin = PC15;
volatile int mode_Count = 0;

void setup() {

  pinMode(LED_Pin, OUTPUT);
  //pinMode(PA9 , INPUT_PULLDOWN);
  // Set pin as INPUT_PULLUP to avoid spurious wakeup
  // Warning depending on boards, INPUT_PULLDOWN may be required instead
  pinMode(pin, INPUT_PULLUP);
  // Configure low power
  LowPower.begin();
  // Attach a wakeup interrupt on pin, calling repetitionsIncrease when the device is woken up
  // Last parameter (LowPowerMode) should match with the low power state used: in this example LowPower.sleep()
  LowPower.attachInterruptWakeup(pin, repetitionsIncrease, FALLING, SLEEP_MODE);
  //LowPower.attachInterruptWakeup(PA9,Charging,RISING,SLEEP_MODE);
  LowPower.sleep();

}

void loop() {
  pinMode(pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pin),mode_test,FALLING);
  while(mode_Count < 3){
    if(mode_Count == 0) {
      digitalWrite(LED_Pin, HIGH);
      delay(1000);
      digitalWrite(LED_Pin, LOW);
      delay(1000);
    }
    if(mode_Count == 1) {
      digitalWrite(LED_Pin, HIGH);
      delay(500);
      digitalWrite(LED_Pin, LOW);
      delay(500);
    }

    if(mode_Count == 2) {
      digitalWrite(LED_Pin, HIGH);
      delay(1000);
      digitalWrite(LED_Pin, LOW);
      delay(1000);
    }
  }
  delay(100);
  /*
  if(mode_Count == 5){
    pinMode(PA9,INPUT);
    int Charge = digitalRead(PA9);
    delayMicroseconds(500);
    while(Charge == HIGH){
      digitalWrite(LED_Pin, HIGH);
      delay(100);
      digitalWrite(LED_Pin, LOW);
      delay(100);
      Charge = digitalRead(PA9);
    }
    mode_Count = 0;
    pinMode(PA9 , INPUT_PULLDOWN);
    LowPower.attachInterruptWakeup(PA9,Charging,RISING,SLEEP_MODE);
    LowPower.sleep();
   
  }
   */
  // Triggers an infinite sleep (the device will be woken up only by the registered wakeup sources)
  // The power consumption of the chip will drop consistently
  mode_Count = 0;
  LowPower.attachInterruptWakeup(pin, repetitionsIncrease, FALLING, SLEEP_MODE);
  LowPower.sleep();
}

void repetitionsIncrease() {
  // This function will be called once on device wakeup
  // You can do some little operations here (like changing variables which will be used in the loop)
  // Remember to avoid calling delay() and long running functions since this functions executes in interrupt context
  repetitions ++;
}

void mode_test(){
  mode_Count++;
}

void Charging(){
  mode_Count = 5;
}
