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
uint32_t local_time = 0;
uint32_t sensor_time = 0;
uint16_t distance = 0;

// 마이크로초 단위 딜레이 함수 (TIM1 사용)
void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    while ((__HAL_TIM_GET_COUNTER(&htim1)) < us);
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
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  // TIM1 타이머 시작
  HAL_TIM_Base_Start(&htim1);
  printf("Ultrasonic Sensor Ready!\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  // 1. TRIG 핀(PA9)을 10us 동안 HIGH로 유지하여 초음파 발생
	      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
	      delay_us(2);
	      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
	      delay_us(10);
	      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

	      // 2. ECHO 핀(PA8)이 HIGH가 될 때까지 대기
	      uint32_t wait_timeout = 0;
	      while (!(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8)))
	      {
	          wait_timeout++;
	          if(wait_timeout > 500000) break; // 센서 응답 없음 타임아웃
	      }

	      // 3. ECHO 핀이 HIGH인 시간 측정 시작
	      // 여기서 딱 한 번만 타이머를 0으로 초기화합니다.
	      __HAL_TIM_SET_COUNTER(&htim1, 0);

	      // ECHO 핀이 LOW로 떨어질 때까지 대기 (내부에서 delay_us 절대 호출 금지)
	      while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8))
	      {
	          // 30000us(30ms)가 넘어가면 측정 거리 초과(약 5m)이므로 루프 탈출
	          if (__HAL_TIM_GET_COUNTER(&htim1) > 30000) break;
	      }

	      // 4. 측정한 시간(us) 가져오기
	      sensor_time = __HAL_TIM_GET_COUNTER(&htim1);

	      // 5. 거리 계산 (음속: 340m/s = 0.034cm/us, 왕복이므로 2로 나눔)
	      distance = (sensor_time * 0.034) / 2;

	      // 6. 결과 출력
	      if (wait_timeout > 500000)
	      {
	          printf("Error: Sensor not responding\r\n");
	      }
	      else if (distance > 0 && distance < 400) // HC-SR04 스펙: 2~400cm
	      {
	          printf("Distance: %d cm\r\n", distance);
	      }
	      else
	      {
	          // 디버깅을 위해 측정된 시간도 함께 출력
	          printf("Out of range (Time: %lu us)\r\n", sensor_time);
	      }

	      // 0.5초 주기로 반복 측정
	      HAL_Delay(500);
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
