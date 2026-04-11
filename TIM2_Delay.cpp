
#include "main.h"
#include "stm32f4xx.h"

int main(void)
{

	RCC->AHB1ENR |= 1; /* enable GPIOA clock */
	RCC -> APB1ENR |= 1; /* Enable CLK source to TIM2 */
	TIM2 -> PSC = 1600 - 1; 
	TIM2 -> ARR = 10000;
	TIM2 -> CNT = 0;
	TIM2 -> CR1 = 1; /*Enable Timer2 with an UP Counter */
	GPIOA->MODER &= ~0x00000C00; /* clear pin mode */
	GPIOA->MODER |= 0x00000400; /* set pin to output mode */

	while(1){
		if(TIM2 ->SR & 1){
			TIM2->SR &= ~1; // Write to Register to Clear flag
			GPIOA -> ODR ^= 0x00000020;
		}
	}
}
