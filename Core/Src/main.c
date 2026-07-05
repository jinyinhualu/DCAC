/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
#include "oled.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PI 3.1416f

#define PWM_GUARD_COUNTS 0U
#define PWM_ARR 3600U
#define SINE_TABLE_SIZE 200U

#define ADC_BUFFER_SIZE 2U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint32_t duty = 0U;
static uint32_t pwm_arr = 0U;
static uint32_t pwm_duty_span = 0U;

static float sin_table[SINE_TABLE_SIZE] = {0.0f};

static uint16_t adc_buffer[ADC_BUFFER_SIZE] = {0};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void SinglePhase(void);
static void SetActiveSpwmOutput(uint8_t positive_half_cycle);
static void UpdateSpwmDuty(uint16_t index);
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
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_ADC1_Init();
  MX_SPI3_Init();
  /* USER CODE BEGIN 2 */
  pwm_duty_span = (PWM_ARR > (2U * PWM_GUARD_COUNTS)) ? (PWM_ARR - (2U * PWM_GUARD_COUNTS)) : 0U;

  SinglePhase(); // Generate sine wave values

  __HAL_TIM_ENABLE_OCxPRELOAD(&htim1, TIM_CHANNEL_1);
  UpdateSpwmDuty(0U);
  HAL_TIM_GenerateEvent(&htim1, TIM_EVENTSOURCE_UPDATE);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1); // Start PWM on TIM1 Channel 1
  HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1); // Start complementary PWM on TIM1 Channel 1
  SetActiveSpwmOutput(1U);
  HAL_TIM_Base_Start_IT(&htim2); // Start TIM2 in interrupt mode

  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, ADC_BUFFER_SIZE); // Start ADC1 in DMA mode
  __HAL_DMA_DISABLE_IT(hadc1.DMA_Handle, DMA_IT_HT | DMA_IT_TC); // Disable DMA half/full transfer interrupts

  HAL_Delay(100); // Wait for 100ms before initializing OLED
  OLED_Init(); // Initialize OLED
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    OLED_NewFrame();
    OLED_PrintASCIIString(0, 0, "ADC1:", &afont16x8, OLED_COLOR_NORMAL);
    OLED_PrintNumber(48, 0, adc_buffer[0], OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(0, 16, "ADC2:", &afont16x8, OLED_COLOR_NORMAL);
    OLED_PrintNumber(48, 16, adc_buffer[1], OLED_COLOR_NORMAL);
    OLED_ShowFrame();
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2) // Check if the interrupt is from TIM2
  {
    static uint16_t index = 0;

    UpdateSpwmDuty(index);
    index++;
    if (index >= SINE_TABLE_SIZE * 2U) // Wrap around after completing one full sine wave cycle
    {
      index = 0U;
    }
  }
}

static void SinglePhase(void)
{
  for(uint16_t i = 0; i < SINE_TABLE_SIZE; i++ )
  {
    sin_table[i] = sinf((2.0f * PI * (float)i) / (float)SINE_TABLE_SIZE);
  }
}

static void SetActiveSpwmOutput(uint8_t positive_half_cycle)
{
  uint32_t ccer = htim1.Instance->CCER;

  ccer &= ~(TIM_CCER_CC1E | TIM_CCER_CC1NE);
  ccer |= positive_half_cycle ? TIM_CCER_CC1E : TIM_CCER_CC1NE;

  htim1.Instance->CCER = ccer;
}

static void UpdateSpwmDuty(uint16_t index)
{
  uint8_t positive_half_cycle = (index < SINE_TABLE_SIZE) ? 1U : 0U;
  float magnitude = positive_half_cycle ? sin_table[index] : -sin_table[index];

  if (magnitude < 0.0f)
  {
    magnitude = 0.0f;
  }
  if (magnitude > 1.0f)
  {
    magnitude = 1.0f;
  }

  duty = PWM_GUARD_COUNTS + (uint32_t)(magnitude * (float)pwm_duty_span);
  SetActiveSpwmOutput(positive_half_cycle);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
