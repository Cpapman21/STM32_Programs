#include "main.h"
#include "stm32f4xx.h"

/* Private function prototypes -----------------------------------------------*/
//void SystemClock_Config(void);
//static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */

  /*
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();

  int current_time = 0;
  int previous_time = 0;
  int delay = 100;

  while (1)
  {
	  current_time = uwTick;
	  if(current_time - previous_time > delay){
		  HAL_GPIO_TogglePin (GPIOA, GPIO_PIN_5);
		  previous_time = current_time;
	  }
  }
 	*/
	RCC->AHB1ENR |= 1; /* enable GPIOA clock */
	GPIOA->MODER &= ~0x00000C00; /* clear pin mode */
	GPIOA->MODER |= 0x00000400; /* set pin to output mode */ /*
	Configure SysTick */
	SysTick->LOAD = 3200000 - 1; /* reload with number of clocks per
	second */ SysTick->VAL = 0;
	SysTick->CTRL = 5; /* enable it, no interrupt, use system clock */
	while(1){
		if(SysTick -> CTRL & 0x10000){
			GPIOA -> ODR ^= 0x00000020;
		}
	}
}
