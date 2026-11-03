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
#include <string.h>
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef hlpuart1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim6;

/* USER CODE BEGIN PV */
#define CORRECT_PASSWORD "6666"
char rx_buffer[10];
uint8_t rx_data;
uint8_t rx_index = 0;
uint8_t error_count = 0;
volatile uint32_t lock_timer = 0; // 30秒鎖定倒數計時器（需加 volatile 供中斷修改）

typedef enum {
    STATE_LOCKED = 0,    // 鎖定中（紅燈1呼吸）
    STATE_LOCKOUT_30S,   // 錯誤3次，鎖定30秒（紅1與紅2交互閃爍並倒數）
    STATE_UNLOCKED       // 解鎖狀態（綠燈1呼吸）
} SystemState_t;

volatile SystemState_t current_state = STATE_LOCKED;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM6_Init(void);
/* USER CODE BEGIN PFP */
void UART_SendString(char *str);
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
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_LPUART1_UART_Init();
  MX_TIM2_Init();
  MX_TIM6_Init();

  /* USER CODE BEGIN 2 */
  // 啟動 TIM2 CH1 (PA0 紅燈1) 與 CH2 (PA1 綠燈1) 的 PWM 輸出
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);

  // 啟動 TIM6 定時器中斷 (精準 1 秒中斷一次)
  HAL_TIM_Base_Start_IT(&htim6);

  // 開啟 LPUART1 中斷接收
  HAL_UART_Receive_IT(&hlpuart1, &rx_data, 1);

  UART_SendString("System Initialized. Please enter password:\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

      if (current_state == STATE_LOCKED)
      {
          __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0); // 關閉綠燈

          // 紅燈 1 (PA0) 呼吸燈
          for (int i = 0; i < 1000; i += 10)
          {
              __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, i);
              HAL_Delay(5);
              if (current_state != STATE_LOCKED) break;
          }
          for (int i = 1000; i > 0; i -= 10)
          {
              __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, i);
              HAL_Delay(5);
              if (current_state != STATE_LOCKED) break;
          }
      }
      else if (current_state == STATE_LOCKOUT_30S)
      {
          // 30 秒鎖定期間：紅燈 1 (PA0) 與紅燈 2 (PB0) 交互閃爍
          __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 1000);
          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
          HAL_Delay(300);

          __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
          HAL_Delay(300);
      }
      else if (current_state == STATE_UNLOCKED)
      {
          __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0); // 關閉紅燈1

          // 綠燈 1 (PA1) 呼吸燈
          for (int i = 0; i < 1000; i += 10)
          {
              __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, i);
              HAL_Delay(5);
              if (current_state != STATE_UNLOCKED) break;
          }
          for (int i = 1000; i > 0; i -= 10)
          {
              __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, i);
              HAL_Delay(5);
              if (current_state != STATE_UNLOCKED) break;
          }
      }

    /* USER CODE BEGIN 3 */
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 169;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim2);
}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 17000 - 1; // 170MHz / 17000 = 10kHz (每 0.1ms 跳動一次)
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 10000 - 1;    // 10000 * 0.1ms = 1 秒精準中斷一次
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED_RED_2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_RED_2_Pin */
  GPIO_InitStruct.Pin = LED_RED_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
void UART_SendString(char *str)
{
    HAL_UART_Transmit(&hlpuart1, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}

/* TIM6 定時器中斷回呼函式 (每 1 秒觸發一次) */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        if (current_state == STATE_LOCKOUT_30S)
        {
            if (lock_timer > 0)
            {
                lock_timer--;
                char msg[50];
                sprintf(msg, "Lockout remaining: %d seconds...\r\n", (int)lock_timer);
                UART_SendString(msg);
            }

            if (lock_timer == 0)
            {
                // 倒數結束，恢復鎖定狀態
                HAL_GPIO_WritePin(GPIOB, LED_RED_2_Pin, GPIO_PIN_RESET); // 關閉紅燈2
                error_count = 0;
                current_state = STATE_LOCKED;
                UART_SendString("Lockout expired. Please enter password:\r\n");
            }
        }
    }
}

/* LPUART1 接收中斷回呼函式 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == LPUART1)
    {
        // 鎖定 30 秒期間直接拒絕輸入
        if (current_state == STATE_LOCKOUT_30S)
        {
            UART_SendString("System Locked! Please wait for countdown.\r\n");
            HAL_UART_Receive_IT(&hlpuart1, &rx_data, 1);
            return;
        }

        if (rx_data == '\r' || rx_data == '\n')
        {
            // 【修正重點】只有當 rx_index > 0 (確實有輸入字元) 時才處理
            if (rx_index > 0)
            {
                rx_buffer[rx_index] = '\0';

                if (current_state == STATE_LOCKED)
                {
                    if (strcmp(rx_buffer, CORRECT_PASSWORD) == 0)
                    {
                        current_state = STATE_UNLOCKED;
                        error_count = 0;
                        UART_SendString("Password Correct! Unlocked.\r\n");
                    }
                    else
                    {
                        error_count++;
                        char msg[50];
                        sprintf(msg, "Wrong Password! Error count: %d/3\r\n", error_count);
                        UART_SendString(msg);

                        if (error_count >= 3)
                        {
                            current_state = STATE_LOCKOUT_30S;
                            lock_timer = 30; // 啟動 30 秒倒數
                            UART_SendString("Too many errors! System locked for 30 seconds.\r\n");
                        }
                    }
                }
                rx_index = 0; // 處理完才重置索引
            }
            // 若 rx_index == 0，代表這是 \r\n 帶來的第二個換行字元，直接忽略不計
        }
        else
        {
            if (rx_index < 4)
            {
                rx_buffer[rx_index++] = rx_data;
            }
        }

        HAL_UART_Receive_IT(&hlpuart1, &rx_data, 1);
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
__weak void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
