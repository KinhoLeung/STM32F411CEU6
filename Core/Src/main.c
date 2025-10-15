/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "SEGGER_RTT.h"
#include "rotary.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    ROTARY_MODE_INTERRUPT, //中断
    ROTARY_MODE_TIMER, //定时器
    ROTARY_MODE_POLLING, //轮询
    ROTARY_MODE_COUNT //数量
} Rotary_Mode;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint8_t key_press = 0;
volatile uint8_t rotary_key_press = 0;
volatile int32_t rotary_count = 0;
Rotary_Mode rotary_mode = ROTARY_MODE_INTERRUPT;
Rotary_HandleTypeDef hrotary;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
bool read_rotary_a(void *data) {
    return HAL_GPIO_ReadPin(ROTARY_CLK_GPIO_Port, ROTARY_CLK_Pin) == GPIO_PIN_SET;
}
bool read_rotary_b(void *data) {
    return HAL_GPIO_ReadPin(ROTARY_DT_GPIO_Port, ROTARY_DT_Pin) == GPIO_PIN_SET;
}
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
  MX_TIM5_Init();
  MX_USART1_UART_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  SEGGER_RTT_Init();
  Rotary_Init(&hrotary, read_rotary_a, NULL, read_rotary_b, NULL);
  HAL_TIM_Base_Start_IT(&htim5);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if(key_press)
    {
        key_press=0;
        HAL_Delay(10);
        if (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET) {      
            rotary_mode = (rotary_mode+1) % ROTARY_MODE_COUNT;
            printf("rotary_mode=%d\r\n",rotary_mode);
            HAL_GPIO_DeInit(GPIOB, ROTARY_DT_Pin|ROTARY_CLK_Pin);
            GPIO_InitTypeDef GPIO_InitStruct = {0};
            if(rotary_mode == ROTARY_MODE_INTERRUPT)
            {
                GPIO_InitStruct.Pin = ROTARY_DT_Pin|ROTARY_CLK_Pin;
                GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
                GPIO_InitStruct.Pull = GPIO_PULLUP;
                HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

                HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
                HAL_NVIC_EnableIRQ(EXTI1_IRQn);
                HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
                HAL_NVIC_EnableIRQ(EXTI2_IRQn);
            }
            else
            {
                GPIO_InitStruct.Pin = ROTARY_DT_Pin|ROTARY_CLK_Pin;
                GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
                GPIO_InitStruct.Pull = GPIO_PULLUP;
                HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

                HAL_NVIC_DisableIRQ(EXTI1_IRQn);
                HAL_NVIC_DisableIRQ(EXTI2_IRQn);
            }
            while (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET) {HAL_Delay(1);}
        }
    }
    if(rotary_key_press)
    {
        rotary_key_press=0;
        HAL_Delay(10);
        if (HAL_GPIO_ReadPin(ROTARY_SW_GPIO_Port, ROTARY_SW_Pin) == GPIO_PIN_RESET) {      
            printf("rotary_count=%d\r\n",rotary_count);
            while (HAL_GPIO_ReadPin(ROTARY_SW_GPIO_Port, ROTARY_SW_Pin) == GPIO_PIN_RESET) {HAL_Delay(1);}
        }
    }
    if(rotary_mode == ROTARY_MODE_POLLING)
    {
        uint8_t result = Rotary_Process(&hrotary);
        if (result == ROTARY_DIR_CW) 
        {
            rotary_count++;
        }
        else if (result == ROTARY_DIR_CCW)
        {
            rotary_count--;
        }
    }
    /* USER CODE END WHILE */

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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 192;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == KEY_Pin)
    {
        static uint32_t key_last_tick = 0;
        uint32_t now = HAL_GetTick();
        if (now - key_last_tick < 50) {
            return; //消抖50ms
        }
        key_last_tick = now;
        key_press = 1;
    }

    if(GPIO_Pin == ROTARY_SW_Pin)
    {
        static uint32_t rotary_key_last_tick = 0;
        uint32_t now = HAL_GetTick();
        if (now - rotary_key_last_tick < 50) {
            return; //消抖50ms
        }
        rotary_key_last_tick = now;
        rotary_key_press = 1;
    }
    
    if(rotary_mode == ROTARY_MODE_INTERRUPT)
    {
        if(GPIO_Pin == ROTARY_CLK_Pin || GPIO_Pin == ROTARY_DT_Pin) //外部中断触发
        {
            static uint32_t last_interrupt_time = 0;
            uint32_t current_time = HAL_GetTick();
            // 简单的软件防抖：忽略1ms内的重复中断
            if (current_time - last_interrupt_time >= 1)
            {
                last_interrupt_time = current_time;
                uint8_t result = Rotary_Process(&hrotary);
                
                if (result == ROTARY_DIR_CW)
                {
                    rotary_count++;
                }
                else if (result == ROTARY_DIR_CCW)
                {
                    rotary_count--;
                }
            }
        }
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if(htim->Instance == TIM5) //1ms触发一次
  {
      static uint16_t led_counter = 0;
      led_counter++;
      if(led_counter >= 500)
      {
          led_counter = 0;
          HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
      }

      if(rotary_mode == ROTARY_MODE_TIMER)
      {
        // 调用编码器处理函数
        uint8_t result = Rotary_Process(&hrotary);
        // 根据返回值更新计数器
        if (result == ROTARY_DIR_CW) 
        {
            rotary_count++;  // 顺时针，计数+1
        }
        else if (result == ROTARY_DIR_CCW)
        {
            rotary_count--;  // 逆时针，计数-1
        }
        // result == ROTARY_DIR_NONE时不做处理
      }
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

#ifdef  USE_FULL_ASSERT
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
