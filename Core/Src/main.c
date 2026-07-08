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
#define PI 3.1415926f
#define ROOT_2 1.4142f

#define PWM_TIMER_ARR 8400U
#define AMP_TO_PERCENT 0.2f
#define HALF_PERIOD_PARTS 200U

#define ADC_BUFFER_SIZE 2U
#define ADC_FILTER_SAMPLES 16U
#define ADC_PEAK_HOLD_WINDOWS 20U
#define ADC_PEAK_DECAY_STEP 8U
#define OLED_REFRESH_MS 50U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint32_t duty = PWM_TIMER_ARR / 2U; // Initial duty cycle set to 50%

static float sin_1[HALF_PERIOD_PARTS] = {0};

static volatile uint16_t adc_buffer[ADC_BUFFER_SIZE] = {0};
static volatile uint16_t adc_display_buffer[ADC_BUFFER_SIZE] = {0};

volatile float AIMUO = 0.0f; // Output current
volatile float UO = 0.0f; // Output voltage
volatile float AIMIO = 0.0f; // Output current
volatile float IO = 0.0f; // Output voltage
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void SinglePhase(void);
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
  SinglePhase(); // Generate sine wave values

  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1); // Start PWM on TIM1 Channel 1
  HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1); // Start complementary PWM on TIM1 Channel 1
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

    // 将均值滤波移至主循环，降低定时器高频中断的CPU占用
    static uint32_t last_oled_refresh = 0U;

    if ((HAL_GetTick() - last_oled_refresh) >= OLED_REFRESH_MS)
    {
      last_oled_refresh = HAL_GetTick();

      OLED_NewFrame();
      OLED_PrintASCIIString(0, 0, "AIMU0:", &afont16x8, OLED_COLOR_NORMAL);
      OLED_PrintFloat(60, 0, AIMUO, 3U, &afont16x8, OLED_COLOR_NORMAL);
      OLED_PrintASCIIString(0, 16, "U0:", &afont16x8, OLED_COLOR_NORMAL);
      OLED_PrintFloat(60, 16, UO, 3U, &afont16x8, OLED_COLOR_NORMAL);
    // OLED_DrawImage(32, 0, &xiaomiImg, OLED_COLOR_NORMAL);
      OLED_ShowFrame();
    }
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
  RCC_OscInitStruct.PLL.PLLN = 168;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
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
    static uint16_t adc_peak[ADC_BUFFER_SIZE] = {0U};
    static uint16_t adc_sample_count = 0U;
    static uint16_t adc_hold_count[ADC_BUFFER_SIZE] = {0U};

    if (adc_buffer[0] > adc_peak[0])
    {
      adc_peak[0] = adc_buffer[0];
    }
    if (adc_buffer[1] > adc_peak[1])
    {
      adc_peak[1] = adc_buffer[1];
    }
    adc_sample_count++;

    if (adc_sample_count >= ADC_FILTER_SAMPLES)
    {
      for (uint8_t i = 0U; i < ADC_BUFFER_SIZE; i++)
      {
        if (adc_peak[i] >= adc_display_buffer[i])
        {
          adc_display_buffer[i] = adc_peak[i];
          adc_hold_count[i] = 0U;
        }
        else if (adc_hold_count[i] < ADC_PEAK_HOLD_WINDOWS)
        {
          adc_hold_count[i]++;
        }
        else if (adc_display_buffer[i] > ADC_PEAK_DECAY_STEP)
        {
          adc_display_buffer[i] -= ADC_PEAK_DECAY_STEP;
        }
        else
        {
          adc_display_buffer[i] = 0U;
        }
      }

      adc_peak[0] = 0U;
      adc_peak[1] = 0U;
      adc_sample_count = 0U;
    }

    AIMUO = 3.3f / ROOT_2; // Convert ADC value to current for output current
    UO = (float)adc_display_buffer[0] * 3.3f / (4095.0f * ROOT_2); // Convert ADC value to voltage for output voltage
    AIMIO = 3.3f / ROOT_2;
    IO = (float)adc_display_buffer[1] * 3.3f / (4095.0f * ROOT_2);

    float sine_value = sin_1[index++];
    float duty_ratio = 0.5f + (sine_value - 0.5f) * AMP_TO_PERCENT;

    duty = (uint32_t)(duty_ratio * (float)PWM_TIMER_ARR); // Keep frequency unchanged, only adjust duty swing

    if (index >= HALF_PERIOD_PARTS)
    {
      index = 0;
    }

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty); // Update PWM duty cycle
  }
}

static void SinglePhase(void)
{
  for(uint16_t i = 0; i < HALF_PERIOD_PARTS; i++ )
  {
    float value = sinf(PI * i / (HALF_PERIOD_PARTS - 1U));
    sin_1[i] = (value > 0.0f) ? value : 0.0f;
  }
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
