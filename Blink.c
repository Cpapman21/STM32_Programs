#include "main.h"
#include "stm32f4xx.h"
void delayMs(int n);

 int main(void) {
RCC->AHB1ENR |= 1; /* enable GPIOA clock */
GPIOA->MODER &= ~0x00000C00; /* clear pin mode */
GPIOA->MODER |= 0x00000400; /* set pin to output mode */
 /* Configure SysTick */
SysTick->LOAD = 0xFFFFFF; /* reload with max value */
SysTick->CTRL= 5; /* enable it, no interrupt, use system clock */
 while (1) {
 /* take bit 23 of SysTick current value and shift it to bit 5 then
 write it to PortA */
GPIOA->ODR = (SysTick->VAL >> (23 - 5)) & 0x00000020; }
}
