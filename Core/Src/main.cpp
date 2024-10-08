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
#include <sniffer_tag.hpp>
#include "Gpio.hpp"
#include "Memory.hpp"
#include "CommandMessage.hpp"
#include <Lora.hpp>
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
I2C_HandleTypeDef hi2c3;
SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;
SPI_HandleTypeDef hspi3;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

uint8_t Data_tx_1[11] = { 0x7E, 0x01, 0x04, 0x13, 0x00, 0x02, 0x15, 0x50, 0x5B, 0x9C, 0x7F};
uint8_t Data_tx_2[11] = { 0x7E, 0x03, 0x02, 0x13, 0x00, 0x02, 0x15, 0x50, 0x59, 0x71, 0x7F};
uint8_t Data_tx_3[11] = { 0x7E, 0x05, 0x07, 0x13, 0x00, 0x02, 0x15, 0x50, 0x7D, 0x93, 0x7F};

std::vector<uint8_t> data_compose;

uint8_t *data_tx[3] = { Data_tx_1, Data_tx_2, Data_tx_3 };

uint8_t Data_reciv_test[256] = { 0 };

double *distance_ptr;
TAG_t *tag;
int size = 0;
uint8_t running_device = DEV_UWB3000F27;
Uwb_HW_t uwb_hw_a;
Uwb_HW_t uwb_hw_b;
Uwb_HW_t *hw;
dwt_local_data_t *pdw3000local;
uint8_t crcTable[256];
uint8_t recvChar;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_I2C3_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_SPI3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

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
	MX_USART1_UART_Init();
	MX_I2C3_Init();
	MX_SPI1_Init();
	MX_SPI2_Init();
	MX_SPI3_Init();
	/* USER CODE BEGIN 2 */
	pdw3000local = new dwt_local_data_t;
	TAG_List list = { NULL, 0 };

	HAL_GPIO_WritePin(DW3000_B_WKUP_GPIO_Port, DW3000_B_WKUP_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DW3000_A_WKUP_GPIO_Port, DW3000_A_WKUP_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DW3000_A_CS_GPIO_Port, DW3000_A_CS_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DW3000_B_CS_GPIO_Port, DW3000_B_CS_Pin, GPIO_PIN_RESET);

	init_uwb_device(&uwb_hw_a, &hspi3, DW3000_A_CS_GPIO_Port,
	DW3000_A_CS_Pin,
	DW3000_A_RST_GPIO_Port, DW3000_A_RST_Pin);

	init_uwb_device(&uwb_hw_b, &hspi3, DW3000_B_CS_GPIO_Port, DW3000_B_CS_Pin,
			DW3000_B_RST_GPIO_Port, DW3000_B_RST_Pin);

	TAG_t tag;
	reset_TAG_values(&tag);

	uint32_t query_timeout = 1000;
	uint32_t query_ticks;
	uint32_t debug_count = 0;

	tag.master_state = MASTER_MULTIPLE_DETECTION; //MASTER_MULTIPLE_DETECTION      MASTER_ONE_DETECTION
	TAG_STATUS_t tag_status = TAG_DISCOVERY;
	uint32_t lora_send_timeout = 500;
	uint32_t lora_send_ticks = HAL_GetTick();

	Memory eeprom = Memory(&hi2c3);

	CommandMessage cmd = CommandMessage(
			static_cast<uint8_t>(MODULE_FUNCTION::SNIFFER), 0x00);

	Gpio rx_lora_nss = Gpio(LORA_RX_NSS_GPIO_Port, LORA_RX_NSS_Pin);
	Gpio rx_lora_rst = Gpio(LORA_RX_NRST_GPIO_Port, LORA_RX_NRST_Pin);
	Gpio tx_lora_nss = Gpio(LORA_TX_NSS_GPIO_Port, LORA_TX_NSS_Pin);
	Gpio tx_lora_rst = Gpio(LORA_TX_NRST_GPIO_Port, LORA_TX_NRST_Pin);
	Lora rxlora = Lora(rx_lora_nss, rx_lora_rst, &hspi2, &eeprom);
	Lora txlora = Lora(tx_lora_nss, tx_lora_rst, &hspi1, &eeprom);
	rxlora.set_lora_settings(LoraBandWidth::BW_125KHZ, CodingRate::CR_4_6,
			SpreadFactor::SF_10, DOWNLINK_FREQ, UPLINK_FREQ);
	txlora.set_lora_settings(LoraBandWidth::BW_125KHZ, CodingRate::CR_4_6,
			SpreadFactor::SF_10, DOWNLINK_FREQ, UPLINK_FREQ);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {

		if (tag_status == TAG_DISCOVERY) {
			debug_count++;
			if (hw == &uwb_hw_a) {
				hw = &uwb_hw_b;
				tag.distance = &(tag.distance_b);
			} else {
				hw = &uwb_hw_a;
				tag.distance = &(tag.distance_a);
			}

			tag_status = tag_discovery(&tag);

			if ((tag_status != TAG_SEND_TIMESTAMP_QUERY)
					&& (tag_status != TAG_ONE_DETECTION))
				tag_status = TAG_DISCOVERY;
			else
				query_ticks = HAL_GetTick();
			//			set_battery_voltage(&(tag.battery_voltage));
			//			set_temperature(&(tag.temperature));
			converse_Tag_Battery_Voltage(&tag);
			if ((debug_count == 1) || (tag_status == TAG_ONE_DETECTION)) {
				debug(tag, tag_status);
			}

		} else if (tag_status == TAG_SEND_TIMESTAMP_QUERY) {
			if (tag.readings < DISTANCE_READINGS) {
				if ((hw == &uwb_hw_a)
						&& (tag.distance_b.counter < DISTANCE_READINGS / 2)) {
					hw = &uwb_hw_b;
					tag.distance = &(tag.distance_b);
				} else {
					hw = &uwb_hw_a;
					tag.distance = &(tag.distance_a);
				}
				tag.command = TAG_TIMESTAMP_QUERY;
			}
			if ((tag.readings == DISTANCE_READINGS - 3)) { //|| (tag.command == TAG_SET_SLEEP_MODE)
				tag.command = TAG_SET_SLEEP_MODE;
				tag_status = TAG_SEND_SET_SLEEP;
				tag.Estado_Final = 1;
			}
			debug(tag, tag_status);
			tag_status = tag_send_timestamp_query(&tag);

			if ((tag_status == TAG_SEND_TIMESTAMP_QUERY)
					|| (tag.Estado_Final == 0)) {
				tag.readings++;
				tag_status = TAG_SEND_TIMESTAMP_QUERY;

			} else if (tag.Estado_Final == 1) {
				double distance_a_sum = 0;
				double distance_b_sum = 0;
				for (uint8_t i = 0; i < tag.distance_a.counter; i++)
					distance_a_sum += tag.distance_a.readings[i];

				for (uint8_t i = 0; i < tag.distance_b.counter; i++)
					distance_b_sum += tag.distance_b.readings[i];

				tag.distance_a.value = (uint16_t) ((distance_a_sum * 100)
						/ tag.distance_a.counter);
				tag.distance_b.value = (uint16_t) ((distance_b_sum * 100)
						/ tag.distance_b.counter);
				insert_tag(&list, tag);
				tag_status = TAG_DISCOVERY;
				reset_TAG_values(&tag);
				debug_count = 0;
			} else {
				tag_status = TAG_SEND_TIMESTAMP_QUERY;
			}
			debug(tag, tag_status);
			if (HAL_GetTick() - query_ticks > query_timeout) {
				tag_status = TAG_DISCOVERY;
				reset_TAG_values(&tag);
				debug_count = 0;
			}

		} else if (tag_status == TAG_ONE_DETECTION) {
			debug(tag, tag_status);
			insert_tag(&list, tag);
			tag_status = TAG_DISCOVERY;
			reset_TAG_values(&tag);
			debug_count = 0;
		}


//				 uint8_t rx_bytes = rxlora.receive(Data_reciv_test, LinkMode::DOWNLINK);
//		if (rx_bytes > 0) {
//			STATUS status_data = command.validate(Data_reciv_test, rx_bytes);
//			if (status_data == STATUS::VALID_FRAME) {
//				uint8_t *rx = Data_reciv_test;
//				size_t temp_value = 1 + list.count * SERIALIZED_TAG_SIZE; // Or use uint32_t
//				uint8_t tag_bytes = (uint8_t) temp_value;
//				uint8_t response_length = calculate_frame_length(tag_bytes);
//				uint8_t tx[response_length];
//				memset(tx, 0, response_length);
//				memcpy(tx, rx, LORA_DATA_LENGHT_INDEX_1);
//				memcpy(tx + LORA_DATA_LENGHT_INDEX_1, &tag_bytes,
//						sizeof(tag_bytes));
//				uint8_t *tx_data = tx + LORA_DATA_START_INDEX;
//				tx_data[0] = list.count;
//				tx_data++;
//				serialize_tag_list(&list, tx_data);
//				uint8_t CRC_INDEX = LORA_DATA_START_INDEX
//						+ tx[LORA_DATA_LENGHT_INDEX_1];
//				uint8_t END_INDEX = CRC_INDEX + CRC_SIZE;
//				setCrc(tx, CRC_INDEX);
//				tx[END_INDEX] = LTEL_END_MARK;
//				HAL_GPIO_WritePin(LORA_TX_LED_GPIO_Port, LORA_TX_LED_Pin,
//						GPIO_PIN_RESET);
//				txlora.transmit(tx, response_length, LinkMode::UPLINK);
//				HAL_GPIO_WritePin(LORA_TX_LED_GPIO_Port, LORA_TX_LED_Pin,
//						GPIO_PIN_SET);
//				serialize_tag_list(&list, tx_data);
//				free(tx);
//				print_all_tags(&list, tag_status);
//				print_serialized_tags(&list);
//				free_tag_list(&list);
//			}


		if (((HAL_GetTick() - lora_send_ticks) > lora_send_timeout)
				&& tag_status == TAG_DISCOVERY) {

			if (list.count > 0) { // Or use uint32_t
				size_t temp_value = list.count * SERIALIZED_TAG_SIZE;
				uint8_t tx[temp_value];
				serialize_tag_list(&list, tx);
				std::vector<uint8_t> tx_data_s(tx, tx + temp_value);
				cmd.composeMessage(&tx_data_s);
				std::vector<uint8_t> data_compose = cmd.get_composed_vector();
				txlora.transmit(data_compose.data(),data_compose.size(), LINKMODE::UPLINK);
				lora_send_ticks = HAL_GetTick();
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
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief I2C3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C3_Init(void) {

	/* USER CODE BEGIN I2C3_Init 0 */

	/* USER CODE END I2C3_Init 0 */

	/* USER CODE BEGIN I2C3_Init 1 */

	/* USER CODE END I2C3_Init 1 */
	hi2c3.Instance = I2C3;
	hi2c3.Init.Timing = 0x00303D5B;
	hi2c3.Init.OwnAddress1 = 0;
	hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c3.Init.OwnAddress2 = 0;
	hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
	hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c3) != HAL_OK) {
		Error_Handler();
	}

	/** Configure Analogue filter
	 */
	if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE)
			!= HAL_OK) {
		Error_Handler();
	}

	/** Configure Digital filter
	 */
	if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C3_Init 2 */

	/* USER CODE END I2C3_Init 2 */

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
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
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
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 7;
  hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi3.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

}
static void MX_USART1_UART_Init(void) {

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 250000;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
	huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart1) != HAL_OK) {
		Error_Handler();
	}
	if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8)
			!= HAL_OK) {
		Error_Handler();
	}
	if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8)
			!= HAL_OK) {
		Error_Handler();
	}
	if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */
	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA,
			DW3000_A_RST_Pin | KA_Pin | DW3000_A_CS_Pin | DW3000_A_WKUP_Pin
					| DW3000_B_WKUP_Pin | DW3000_B_RST_Pin | DW3000_B_CS_Pin
					| LORA_RX_LED_Pin | LORA_TX_NSS_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB,
	LORA_TX_DIO0_Pin | LORA_TX_LED_Pin | LORA_TX_NRST_Pin | LORA_RX_NSS_Pin,
			GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LORA_RX_DIO0_GPIO_Port, LORA_RX_DIO0_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LORA_RX_NRST_GPIO_Port, LORA_RX_NRST_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pins : DW3000_A_RST_Pin KA_Pin DW3000_A_CS_Pin DW3000_A_WKUP_Pin
	 DW3000_B_WKUP_Pin DW3000_B_RST_Pin DW3000_B_CS_Pin LORA_RX_LED_Pin
	 LORA_TX_NSS_Pin */
	GPIO_InitStruct.Pin = DW3000_A_RST_Pin | KA_Pin | DW3000_A_CS_Pin
			| DW3000_A_WKUP_Pin | DW3000_B_WKUP_Pin | DW3000_B_RST_Pin
			| DW3000_B_CS_Pin | LORA_RX_LED_Pin | LORA_TX_NSS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : DW3000_A_IRQ_Pin DW3000_B_IRQ_Pin */
	GPIO_InitStruct.Pin = DW3000_A_IRQ_Pin | DW3000_B_IRQ_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : LORA_TX_DIO0_Pin LORA_TX_LED_Pin LORA_TX_NRST_Pin LORA_RX_NSS_Pin */
	GPIO_InitStruct.Pin = LORA_TX_DIO0_Pin | LORA_TX_LED_Pin | LORA_TX_NRST_Pin
			| LORA_RX_NSS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : LORA_RX_DIO0_Pin */
	GPIO_InitStruct.Pin = LORA_RX_DIO0_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LORA_RX_DIO0_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : LORA_RX_NRST_Pin */
	GPIO_InitStruct.Pin = LORA_RX_NRST_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LORA_RX_NRST_GPIO_Port, &GPIO_InitStruct);

	/* USER CODE BEGIN MX_GPIO_Init_2 */
	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
int keepAliveStartTicks = 0;
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();

	while (1) {
		if (keepAliveStartTicks > 50000) {
			keepAliveStartTicks = 0;
			HAL_GPIO_WritePin(KA_GPIO_Port, KA_Pin, GPIO_PIN_SET);
		} else if (keepAliveStartTicks > 25000)
			HAL_GPIO_WritePin(KA_GPIO_Port, KA_Pin, GPIO_PIN_RESET);
		keepAliveStartTicks++;
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
