#include "main.h"
#include "stm32f4xx.h"



void delay1ms(void){
	SysTick->LOAD = 16000;
	SysTick->VAL = 0; /* clear current value register */ SysTick->CTRL = 0x5;
	/* Enable the timer */
	while((SysTick->CTRL & 0x10000) == 0){ }; /* wait until the COUNTFLAG */
	SysTick->CTRL = 0; /* Stop the timer (Enable = 0) */
 }

void PWM_Test(int PWM_Period , int PWM_OnTime){
	SysTick->LOAD = PWM_Period;
	SysTick->VAL = 0; /* clear current value register */
	SysTick->CTRL = 0x5;
	/* Enable the timer */
	while((SysTick->CTRL & 0x10000) == 0){
		if(SysTick->VAL > PWM_OnTime){
				GPIOA->ODR &= ~(1 << 5);
			}
		if(SysTick->VAL < PWM_OnTime){
				GPIOA->ODR |= (1 << 5);
			}
	}; /* wait until the COUNTFLAG */
	SysTick->CTRL = 0; /* Stop the timer (Enable = 0) */
	GPIOA->ODR &= ~(1 << 5);
 }


 int main(void) {
RCC->AHB1ENR |= 1; /* enable GPIOA clock */
GPIOA->MODER &= ~0x00000C00; /* clear pin mode */
GPIOA->MODER |= 0x00000400; /* set pin to output mode */
int PWM_OnTime = 400;
int PWM_OffTime = 400;
int PWM_Period = PWM_OnTime + PWM_OffTime;
 /* Configure SysTick */
SysTick->LOAD = 0xFFFFFF; /* reload with max value */
SysTick->CTRL= 5; /* enable it, no interrupt, use system clock */
 while (1) {
 /* take bit 23 of SysTick current value and shift it to bit 5 then
 write it to PortA */
	for(int x = 0; x <= PWM_OnTime*2 ; x++){
		PWM_Test(PWM_Period , x);
		delay1ms();
	}
 }
}
