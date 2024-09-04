/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "human_tag.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

TAG_t *tag;
int size = 0;
uint8_t running_device = DEV_UWB3000F00;
SPI_HW_t hw;
dwt_local_data_t *pdw3000local;
uint8_t crcTable[256];
char recvChar;
TAG_STATUS_t tag_status;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Callback: timer has rolled over
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  // Check which version of the timer triggered this callback and toggle LED
  if (htim == &htim4 )
  {
	  tag_status = TAG_SLEEP;

  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	HAL_DeInit();
	HAL_RCC_DeInit();

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
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_TIM4_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

	HAL_GPIO_WritePin(GPIOA, DW3000_RST_Pin, GPIO_PIN_SET);
	/*Local device data, can be an array to support multiple DW3000 testing applications/platforms */

	dwt_local_data_t dwt_local_data;
	pdw3000local = &dwt_local_data;
	/* Default communication configuration. We use default non-STS DW mode. */
	dwt_config_t defatult_dwt_config = { 5, /* Channel number. */
	DWT_PLEN_128, /* Preamble length. Used in TX only. */
	DWT_PAC8, /* Preamble acquisition chunk size. Used in RX only. */
	9, /* TX preamble code. Used in TX only. */
	9, /* RX preamble code. Used in RX only. */
	1, /* 0 to use standard 8 symbol SFD, 1 to use non-standard 8 symbol, 2 for non-standard 16 symbol SFD and 3 for 4z 8 symbol SDF type */
	DWT_BR_6M8, /* Data rate. */
	DWT_PHRMODE_STD, /* PHY header mode. */
	DWT_PHRRATE_STD, /* PHY header rate. */
	(129 + 8 - 8), /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
	DWT_STS_MODE_OFF, /* STS disabled */
	DWT_STS_LEN_64,/* STS length see allowed values in Enum dwt_sts_lengths_e */
	DWT_PDOA_M0 /* PDOA mode off */
	};

	/* Values for the PG_DELAY and TX_POWER registers reflect the bandwidth and power of the spectrum at the current
	 * temperature. These values can be calibrated prior to taking reference measurements. See NOTE 8 below. */
	static dwt_txconfig_t defatult_dwt_txconfig = { 0x34, /* PG delay. */
	0xfdfdfdfd, /* TX power. */
	0x0 /*PG count*/
	};

	hw.spi = &hspi2;
	hw.nrstPin = DW3000_RST_RCV_Pin;
	hw.nrstPort = DW3000_RST_RCV_GPIO_Port;
	hw.nssPin = SPI2_CS_Pin;
	hw.nssPort = SPI2_CS_GPIO_Port;

	HAL_UART_Transmit(&huart1, (uint8_t*) "\n\rDEV_UWB3000F00 init\n\r",
			(uint16_t) strlen("\n\rDEV_UWB3000F00 init\n\r"),
			HAL_MAX_DELAY);

	HAL_GPIO_WritePin(hw.nrstPort, hw.nrstPin, GPIO_PIN_RESET);/* Target specific drive of RSTn line into DW IC low for a period. */
	HAL_Delay(1);
	HAL_GPIO_WritePin(hw.nrstPort, hw.nrstPin, GPIO_PIN_SET);
	if (tag_init(&defatult_dwt_config, &defatult_dwt_txconfig, &dwt_local_data,
			running_device, RATE_6M8) == 1)
		Error_Handler();

	/* Frames used in the ranging process. See NOTE 2 below. */

	/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
	tag = malloc(sizeof(TAG_t));
	if (tag == NULL)
		Error_Handler();

	tag->readings = 0;
	tag->id = 0;
	tag->resp_tx_time = 0;
	tag->resp_tx_timestamp = 0;
	tag->poll_rx_timestamp = 0;

	tag_status = TAG_WAIT_FOR_FIRST_DETECTION;
	tag->id = _dwt_otpread(PARTID_ADDRESS);
	tag->calibrateds_temperature = _dwt_otpread(VTEMP_ADDRESS);
	tag->calibrated_battery_voltage = _dwt_otpread(VBAT_ADDRESS);
	uint16_t read_temp_vbat = dwt_readtempvbat();
	tag->raw_temperature = (uint8_t) (read_temp_vbat >> 8);
	tag->raw_battery_voltage = (uint8_t) (read_temp_vbat);
	dwt_configuresleep(DWT_RUNSAR, DWT_WAKE_WUP);
	dwt_configuresleepcnt(4095);
	//dwt_setsniffmode(1, 2, 200);

	uint32_t query_timeout = 10000;
	uint32_t query_ticks;
	HAL_TIM_Base_Start_IT(&htim4);
	/* Time-stamps of frames transmission/reception, expressed in device time units. */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
		if (tag_status == TAG_WAIT_FOR_FIRST_DETECTION) {
			debug(tag, tag_status);

			tag_status = process_first_tag_information(tag);
			debug(tag, tag_status);
			if (tag_status != TAG_WAIT_FOR_TIMESTAMPT_QUERY){
				tag_status = TAG_WAIT_FOR_FIRST_DETECTION;
				HAL_GPIO_WritePin(hw.nrstPort, hw.nrstPin, GPIO_PIN_RESET);/* Target specific drive of RSTn line into DW IC low for a period. */
				HAL_Delay(1);
				HAL_GPIO_WritePin(hw.nrstPort, hw.nrstPin, GPIO_PIN_SET);
				if (tag_init(&defatult_dwt_config, &defatult_dwt_txconfig,
						&dwt_local_data, running_device, RATE_6M8) == 1)
					Error_Handler();
			}
			else if (tag_status == TAG_WAIT_FOR_TIMESTAMPT_QUERY)
				query_ticks = HAL_GetTick();

		}
		else if (tag_status == TAG_WAIT_FOR_TIMESTAMPT_QUERY) {
			tag_status = process_queried_tag_information(tag);

			if (tag_status == TAG_TX_SUCCESS) {
				if (tag->command == TAG_TIMESTAMP_QUERY)
					tag_status = TAG_WAIT_FOR_TIMESTAMPT_QUERY;
				else if (tag->command == TAG_SET_SLEEP_MODE)
					tag_status = TAG_SLEEP;
			}
			else if (tag_status == TAG_RX_COMMAND_ERROR){
				HAL_GPIO_WritePin(hw.nrstPort, hw.nrstPin, GPIO_PIN_RESET);/* Target specific drive of RSTn line into DW IC low for a period. */
				HAL_Delay(1);
				HAL_GPIO_WritePin(hw.nrstPort, hw.nrstPin, GPIO_PIN_SET);
				if (tag_init(&defatult_dwt_config, &defatult_dwt_txconfig,
						&dwt_local_data, running_device, RATE_6M8) == 1)
					Error_Handler();
				tag_status = TAG_WAIT_FOR_FIRST_DETECTION;
			}

			else
				tag_status = TAG_WAIT_FOR_TIMESTAMPT_QUERY;

			debug(tag, tag_status);

			if (HAL_GetTick() - query_ticks > query_timeout) {
				tag_status = TAG_SLEEP;
			}

		}
		else if (tag_status == TAG_SLEEP) {

			tag->readings++;
			debug(tag, tag_status);
			tag->readings = 0;
			HAL_GPIO_WritePin(hw.nrstPort, hw.nrstPin, GPIO_PIN_RESET);/* Target specific drive of RSTn line into DW IC low for a period. */
			HAL_Delay(3000);
			HAL_GPIO_WritePin(hw.nrstPort, hw.nrstPin, GPIO_PIN_SET);
			if (tag_init(&defatult_dwt_config, &defatult_dwt_txconfig,
					&dwt_local_data, running_device, RATE_6M8) == 1)
				Error_Handler();
			//dwt_setsniffmode(1, 2, 200);
			tag_status = TAG_WAIT_FOR_FIRST_DETECTION;
		}
		//HAL_Delay(1000);
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL4;
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

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 7200-1;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 65536-1;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */
	HAL_TIM_Base_Start_IT(&htim4);
  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

	HAL_UART_Receive_IT(&huart1, &recvChar, 1);

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, DW3000_RST_Pin|DW3000_RST_RCV_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(WAKEUP_GPIO_Port, WAKEUP_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : DW3000_RST_Pin DW3000_RST_RCV_Pin */
  GPIO_InitStruct.Pin = DW3000_RST_Pin|DW3000_RST_RCV_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI1_CS_Pin */
  GPIO_InitStruct.Pin = SPI1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SPI1_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : DEVICE_SELECT_Pin KEY_UP_Pin KEY_DOWN_Pin KEY_OK_Pin
                           KEY_BACK_Pin */
  GPIO_InitStruct.Pin = DEVICE_SELECT_Pin|KEY_UP_Pin|KEY_DOWN_Pin|KEY_OK_Pin
                          |KEY_BACK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI2_CS_Pin */
  GPIO_InitStruct.Pin = SPI2_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SPI2_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : WAKEUP_Pin */
  GPIO_InitStruct.Pin = WAKEUP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(WAKEUP_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
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
	while (1) {
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
