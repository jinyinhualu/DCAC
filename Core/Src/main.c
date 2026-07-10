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
typedef struct SOGI{
    float ualfa_0;      // 输入信号 alpha 分量
    float SOGI_Ualfa;   // 输出正交 alpha
    float SOGI_Ubeta;   // 输出正交 beta

    float integral_2;   // alpha内部积分
    float integral_3;   // beta内部积分

    float Ugird_W0;     // ω0 = 2πf
    float samp_t;       // 采样周期
} SOGI_t;

typedef struct {
    float kp;
    float ki;
    float kd;
    float ek;
    float ek_1;
    float ek_2;
    float uk;
    float uk_1;
} PID_t;


typedef struct {
    float wt;       // 当前相位
    float w0;       // 当前角频�??????????????????
    PID_t pid;      // PLL的PID
    SOGI_t sogi;    // SOGI结构
    float sin_wt;
    float cos_wt;
    float frequency_hz;
} PLL_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PI 3.1415926f
#define ROOT_2 1.4142f

#define PWM_TIMER_ARR 8400U
#define AMP_TO_PERCENT 0.2f
#define HALF_PERIOD_PARTS 200U

#define ADC_BUFFER_SIZE 2U
#define TIM2_UPDATE_HZ 10000.0f
#define POWER_FREQ_HZ 50.0f
#define SOGI_SAMPLE_TIME (1.0f / TIM2_UPDATE_HZ)
#define SOGI_K ROOT_2

#define PLL_BANDWIDTH_HZ 20.0f
#define PLL_DAMPING 0.707f
#define PLL_MIN_FREQ_HZ 45.0f
#define PLL_MAX_FREQ_HZ 55.0f
#define PLL_MIN_SIGNAL 0.01f

#define ADC_OFFSET_ALPHA 0.0002f
#define ADC_MAX_COUNTS 4095.0f
#define ADC_OFFSET_INIT_COUNTS (ADC_MAX_COUNTS * 0.5f)
#define ADC_VREF 3.3f
#define UO_ADC_INDEX 0U
#define UO_SENSOR_GAIN 1.0f

#define OLED_REFRESH_MS 200U
#define DISPLAY_FILTER_ALPHA 0.15f
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint32_t duty = PWM_TIMER_ARR / 2U; // Initial duty cycle set to 50%

static float sin_1[HALF_PERIOD_PARTS] = {0};

static volatile uint16_t adc_buffer[ADC_BUFFER_SIZE] = {0};
static PLL_t uo_pll;

volatile float UO = 0.0f; // Output voltage
volatile float UO_PLL_THETA = 0.0f;
volatile float UO_PLL_FREQ = POWER_FREQ_HZ;
volatile float UO_ADC_COUNTS = ADC_OFFSET_INIT_COUNTS;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void SinglePhase(void);
static void SOGI_Init(SOGI_t *sogi);
static void SOGI_Update(SOGI_t *sogi, float input);
static float SOGI_GetRms(const SOGI_t *sogi);
static void PID_Init(PID_t *pid, float kp, float ki, float kd);
static float PID_Update(PID_t *pid, float error, float output_min, float output_max);
static void PLL_Init(PLL_t *pll);
static void PLL_Update(PLL_t *pll, float input);
static float ClampFloat(float value, float min_value, float max_value);
static float DisplayLowPass(float display_value, float input_value);
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
  PLL_Init(&uo_pll);

  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1); // Start PWM on TIM1 Channel 1
  HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1); // Start complementary PWM on TIM1 Channel 1

  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, ADC_BUFFER_SIZE); // Start ADC1 in DMA mode
  __HAL_DMA_DISABLE_IT(hadc1.DMA_Handle, DMA_IT_HT | DMA_IT_TC); // Disable DMA half/full transfer interrupts
  HAL_TIM_Base_Start_IT(&htim2); // Start TIM2 in interrupt mode

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
    static uint8_t display_initialized = 0U;
    static float display_uo = 0.0f;
    static float display_freq = POWER_FREQ_HZ;
    static float display_adc_counts = ADC_OFFSET_INIT_COUNTS;

    if ((HAL_GetTick() - last_oled_refresh) >= OLED_REFRESH_MS)
    {
      last_oled_refresh = HAL_GetTick();

      if (display_initialized == 0U)
      {
        display_uo = UO;
        display_freq = UO_PLL_FREQ;
        display_adc_counts = UO_ADC_COUNTS;
        display_initialized = 1U;
      }
      else
      {
        display_uo = DisplayLowPass(display_uo, UO);
        display_freq = DisplayLowPass(display_freq, UO_PLL_FREQ);
        display_adc_counts = DisplayLowPass(display_adc_counts, UO_ADC_COUNTS);
      }

      OLED_NewFrame();
      OLED_PrintASCIIString(0, 0, "U0:", &afont16x8, OLED_COLOR_NORMAL);
      OLED_PrintFloat(48, 0, display_uo, 3U, &afont16x8, OLED_COLOR_NORMAL);
      OLED_PrintASCIIString(0, 16, "F:", &afont16x8, OLED_COLOR_NORMAL);
      OLED_PrintFloat(48, 16, display_freq, 2U, &afont16x8, OLED_COLOR_NORMAL);
      OLED_PrintASCIIString(0, 32, "ADC:", &afont16x8, OLED_COLOR_NORMAL);
      OLED_PrintFloat(48, 32, display_adc_counts, 0U, &afont16x8, OLED_COLOR_NORMAL);
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
    static float uo_offset_counts = ADC_OFFSET_INIT_COUNTS;

    float uo_sample_counts = (float)adc_buffer[UO_ADC_INDEX];
    UO_ADC_COUNTS = uo_sample_counts;
    uo_offset_counts += ADC_OFFSET_ALPHA * (uo_sample_counts - uo_offset_counts);

    float uo_sample_ac = (uo_sample_counts - uo_offset_counts) * ADC_VREF / ADC_MAX_COUNTS;
    PLL_Update(&uo_pll, uo_sample_ac);

    UO = SOGI_GetRms(&uo_pll.sogi) * UO_SENSOR_GAIN; // ADC_CHANNEL_6 / PA6
    UO_PLL_THETA = uo_pll.wt;
    UO_PLL_FREQ = uo_pll.frequency_hz;

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

static void SOGI_Init(SOGI_t *sogi)
{
  sogi->ualfa_0 = 0.0f;
  sogi->SOGI_Ualfa = 0.0f;
  sogi->SOGI_Ubeta = 0.0f;
  sogi->integral_2 = 0.0f;
  sogi->integral_3 = 0.0f;
  sogi->Ugird_W0 = 2.0f * PI * POWER_FREQ_HZ;
  sogi->samp_t = SOGI_SAMPLE_TIME;
}

static void SOGI_Update(SOGI_t *sogi, float input)
{
  float alpha_error = 0.0f;

  sogi->ualfa_0 = input;
  alpha_error = sogi->ualfa_0 - sogi->SOGI_Ualfa;

  sogi->integral_2 += (SOGI_K * alpha_error - sogi->SOGI_Ubeta)
                    * sogi->Ugird_W0
                    * sogi->samp_t;
  sogi->SOGI_Ualfa = sogi->integral_2;

  sogi->integral_3 += sogi->SOGI_Ualfa
                    * sogi->Ugird_W0
                    * sogi->samp_t;
  sogi->SOGI_Ubeta = sogi->integral_3;
}

static float SOGI_GetRms(const SOGI_t *sogi)
{
  float peak = sqrtf(sogi->SOGI_Ualfa * sogi->SOGI_Ualfa + sogi->SOGI_Ubeta * sogi->SOGI_Ubeta);

  return peak / ROOT_2;
}

static void PID_Init(PID_t *pid, float kp, float ki, float kd)
{
  pid->kp = kp;
  pid->ki = ki;
  pid->kd = kd;
  pid->ek = 0.0f;
  pid->ek_1 = 0.0f;
  pid->ek_2 = 0.0f;
  pid->uk = 0.0f;
  pid->uk_1 = 0.0f;
}

static float PID_Update(PID_t *pid, float error, float output_min, float output_max)
{
  pid->ek = error;
  pid->uk = pid->uk_1
          + pid->kp * (pid->ek - pid->ek_1)
          + pid->ki * pid->ek
          + pid->kd * (pid->ek - 2.0f * pid->ek_1 + pid->ek_2);
  pid->uk = ClampFloat(pid->uk, output_min, output_max);

  pid->ek_2 = pid->ek_1;
  pid->ek_1 = pid->ek;
  pid->uk_1 = pid->uk;

  return pid->uk;
}

static void PLL_Init(PLL_t *pll)
{
  float pll_bandwidth = 2.0f * PI * PLL_BANDWIDTH_HZ;

  SOGI_Init(&pll->sogi);
  PID_Init(&pll->pid,
           2.0f * PLL_DAMPING * pll_bandwidth,
           pll_bandwidth * pll_bandwidth * SOGI_SAMPLE_TIME,
           0.0f);

  pll->wt = 0.0f;
  pll->w0 = 2.0f * PI * POWER_FREQ_HZ;
  pll->sin_wt = 0.0f;
  pll->cos_wt = 1.0f;
  pll->frequency_hz = POWER_FREQ_HZ;
}

static void PLL_Update(PLL_t *pll, float input)
{
  float alpha = 0.0f;
  float beta = 0.0f;
  float signal_peak = 0.0f;
  float phase_error = 0.0f;
  float omega_nom = 2.0f * PI * POWER_FREQ_HZ;
  float omega_min = 2.0f * PI * PLL_MIN_FREQ_HZ;
  float omega_max = 2.0f * PI * PLL_MAX_FREQ_HZ;
  float omega_correction = 0.0f;

  SOGI_Update(&pll->sogi, input);
  alpha = pll->sogi.SOGI_Ualfa;
  beta = pll->sogi.SOGI_Ubeta;

  signal_peak = sqrtf(alpha * alpha + beta * beta);
  if (signal_peak > PLL_MIN_SIGNAL)
  {
    phase_error = (alpha * pll->cos_wt + beta * pll->sin_wt) / signal_peak;
    phase_error = ClampFloat(phase_error, -1.0f, 1.0f);
  }

  omega_correction = PID_Update(&pll->pid,
                                phase_error,
                                omega_min - omega_nom,
                                omega_max - omega_nom);
  pll->w0 = ClampFloat(omega_nom + omega_correction, omega_min, omega_max);

  pll->wt += pll->w0 * SOGI_SAMPLE_TIME;
  while (pll->wt >= (2.0f * PI))
  {
    pll->wt -= 2.0f * PI;
  }
  while (pll->wt < 0.0f)
  {
    pll->wt += 2.0f * PI;
  }

  pll->sin_wt = sinf(pll->wt);
  pll->cos_wt = cosf(pll->wt);
  pll->frequency_hz = pll->w0 / (2.0f * PI);
}

static float ClampFloat(float value, float min_value, float max_value)
{
  if (value < min_value)
  {
    return min_value;
  }
  if (value > max_value)
  {
    return max_value;
  }

  return value;
}

static float DisplayLowPass(float display_value, float input_value)
{
  return display_value + DISPLAY_FILTER_ALPHA * (input_value - display_value);
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
