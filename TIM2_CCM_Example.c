int main(void)
{

		RCC->AHB1ENR |= 1; /* enable GPIOA clock */
		RCC -> APB1ENR |= 0x01; /* Enable CLK source to TIM2 */
		int CC1s = 0x00; /* CC1 channel is configured as output.*/
		int OC1FE = 0x00; /* CC1 behaves normally depending on counter and CCR1 values even when the trigger is
			ON. The minimum delay to activate CC1 output when an edge occurs on the trigger input is
			5 clock cycles.
		TIM2 ->CCMR1 = 0x00000000
		TIM2 -> CNT = 0;
		TIM2 -> CR1 = 1; /*Enable Timer2 with an UP Counter */
		int OC1PE = 0x00; /*Preload register on TIMx_CCR1 disabled. TIMx_CCR1 can be written at anytime, the
		new value is taken in account immediately.*/
		int OC1M =  0x30; /* Toggle - OC1REF toggles when TIMx_CNT=TIMx_CCR1.*/
		TIM2 -> PSC = 1600 - 1;
		TIM2 -> ARR = 10000 - 1; /* Counts up to this Value */
		TIM2->CNT = 0; /* clear counter */
		TIM2->CR1 = 1; /* enable TIM2 */
		TIM2 -> CCMR1 = CC1s | OC1FE | OC1PE | OC1M;
		TIM2 -> CCR1 = 5000; /* set match value */
		TIM2 -> CCER = 0x01;
		GPIOA->MODER |= 0x00000800; /* set pin to alternate function */
		GPIOA->AFR[0] &= 0x00F00000; /* clear pin AF bits */
		GPIOA->AFR[0]= 0x00100000; /* set pin to AF1 for TIM2 CH1 */

		while(1){

		}
}
