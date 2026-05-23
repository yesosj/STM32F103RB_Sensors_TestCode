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
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h> // printf 사용을 위한 헤더 추가
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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
// printf 출력을 UART2로 리다이렉션하기 위한 함수
#ifdef __GNUC__
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// 타이머 및 거리 계산을 위한 전역 변수
uint32_t sensor_time = 0;
uint16_t distance = 0;

// 마이크로초 단위 딜레이 함수 (TIM1 사용)
void delay_us(uint16_t us)
{
    uint32_t timeout = 0;
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    while ((__HAL_TIM_GET_COUNTER(&htim1)) < us)
    {
        timeout++;
        if(timeout > 50000) break;
    }
}

// 스마트 휴지통 제어를 위한 변수들
uint32_t prev_sonar_time = 0;
uint32_t lid_opened_time = 0; // 물체가 마지막으로 감지된 시간 기록
uint8_t is_lid_open = 0;      // 현재 뚜껑 상태 (0: 닫힘, 1: 열림)
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
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();

  /* USER CODE BEGIN 2 */
  // 필수! 타이머 및 PWM 시작
  HAL_TIM_Base_Start(&htim1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

  // 시작할 때 뚜껑을 닫힌 상태(0도)로 초기화
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 500);

  printf("Smart Trash Can Ready!\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // 현재 시스템 경과 시간(밀리초 단위) 가져오기
    uint32_t current_tick = HAL_GetTick();

    // =========================================================
    // 1. 초음파 센서 실시간 측정 (100ms = 0.1초마다 실행)
    // =========================================================
    if (current_tick - prev_sonar_time >= 100)
    {
        prev_sonar_time = current_tick; // 측정 시간 갱신

        // TRIG 발사
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
        delay_us(2);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
        delay_us(10);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

        // ECHO 대기
        uint32_t wait_timeout = 0;
        while (!(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8)))
        {
            wait_timeout++;
            if(wait_timeout > 50000) break;
        }

        // ECHO 시간 측정
        __HAL_TIM_SET_COUNTER(&htim1, 0);
        while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8))
        {
            if (__HAL_TIM_GET_COUNTER(&htim1) > 30000) break;
        }

        sensor_time = __HAL_TIM_GET_COUNTER(&htim1);
        distance = (sensor_time * 0.034) / 2;

        // =========================================================
        // 2. 스마트 휴지통 동작 로직
        // =========================================================
        if (distance > 0 && distance <= 10) // 10cm 이내에 손(물체) 감지됨
        {
            if (is_lid_open == 0) // 뚜껑이 닫혀있었다면 엽니다.
            {
                printf("Hand Detected! Opening Lid... (Dist: %d cm)\r\n", distance);
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 2500); // 180도 (열림)
                is_lid_open = 1;
            }

            // 물체가 계속 감지되는 동안은 '열린 시간'을 현재 시간으로 계속 갱신 (리셋)
            lid_opened_time = current_tick;
        }
        else // 물체가 10cm 밖에 있거나 없음
        {
            // 뚜껑이 열려있고 && 마지막으로 물체를 본 지 2초(2000ms)가 지났다면
            if (is_lid_open == 1 && (current_tick - lid_opened_time >= 2000))
            {
                printf("Closing Lid...\r\n");
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 500); // 0도 (닫힘)
                is_lid_open = 0;
            }
        }
    } // end of if (current_tick ...)
  } // end of while (1)
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  * where the assert_param error has occurred.
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
