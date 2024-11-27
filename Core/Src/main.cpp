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
#include "GpioHandler.hpp"
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
uint8_t recvChar[255] = { 0 };
TAG_t *tag_ptr;
DistanceHandler distance_a = DistanceHandler(DISTANCE_READINGS);
DistanceHandler distance_b = DistanceHandler(DISTANCE_READINGS);
uint32_t tiempo_max = 5;
GpioHandler gpio_handler();
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
	TAG_List list_od = { NULL, 0 };

	HAL_GPIO_WritePin(DW3000_B_WKUP_GPIO_Port, DW3000_B_WKUP_Pin,
			GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DW3000_A_WKUP_GPIO_Port, DW3000_A_WKUP_Pin,
			GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DW3000_A_CS_GPIO_Port, DW3000_A_CS_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DW3000_B_CS_GPIO_Port, DW3000_B_CS_Pin, GPIO_PIN_RESET);

	init_uwb_device(&uwb_hw_a, &hspi3, DW3000_A_CS_GPIO_Port,
	DW3000_A_CS_Pin,
	DW3000_A_RST_GPIO_Port, DW3000_A_RST_Pin);

	init_uwb_device(&uwb_hw_b, &hspi3, DW3000_B_CS_GPIO_Port, DW3000_B_CS_Pin,
	DW3000_B_RST_GPIO_Port, DW3000_B_RST_Pin);

	TAG_t tag;
	reset_TAG_values(&tag);
	tag_ptr = &tag;

	uint32_t sniffer_id = _dwt_otpread(PARTID_ADDRESS);

	uint8_t hw_device =0;
	tag.envios_ = 0;
	Sniffer_State interfaz_state = MASTER_MULTIPLE_DETECTION;
	tag.sleep_time = 1; //tiempo de sleep
	tag.master_state = MASTER_ONE_DETECTION; //MASTER_MULTIPLE_DETECTION      MASTER_ONE_DETECTION
	TAG_STATUS_t tag_status = TAG_DISCOVERY;
	uint32_t lora_send_timeout = 5000;
	uint32_t lora_send_ticks = HAL_GetTick();
	uint32_t query_timeout = 1000;
	uint32_t query_ticks;
	uint32_t debug_count = 0;

	DistanceHandler *distance_ptr;

	Memory eeprom = Memory(&hi2c3);

	CommandMessage cmd = CommandMessage(
			static_cast<uint8_t>(MODULE_FUNCTION::SNIFFER), 0x00);
	CommandMessage cmd_od = CommandMessage(
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

			hw_device = switch_hw(&tag, distance_ptr, hw, &distance_a, &distance_b);
			uint8_t tx_buffer[TX_DISCOVERY_SIZE] = { 0 };
			uint8_t rx_buffer[FRAME_LEN_MAX_EX] = { 0 };
			uint32_t rx_buffer_size = 0;
			tag.command = TAG_ID_QUERY;

			tx_buffer[0] = tag.command;
			tx_buffer[1] = interfaz_state;
			tx_buffer[2] = tag.sleep_time;
			tx_buffer[3] = DEV_UWB3000F27;
			tx_buffer[4] = hw_device;
			tx_buffer[5] = debug_count;

			tag_status = setup_and_transmit(&tag, tx_buffer,
			TX_DISCOVERY_SIZE, rx_buffer, &rx_buffer_size);

			if (tag_status == TAG_RX_CRC_VALID) {
				if (rx_buffer[3] == DEV_UWB3000F27) {
					tag_status =  TAG_DISCOVERY;
				}
				else {
					if (tag.command == rx_buffer[0]) {
						tag.id = *(const uint32_t*) (rx_buffer + 1);

						const int poll_rx_offset = 5;
						const int resp_tx_offset = 9;

						// Get timestamps embedded in response message
						uint32_t poll_rx_timestamp = *(uint32_t*) (rx_buffer
								+ poll_rx_offset);
						uint32_t resp_tx_timestamp = *(uint32_t*) (rx_buffer
								+ resp_tx_offset);

						// Compute time of flight and distance, using clock offset ratio to correct for differing local and remote clock rates
						uint64_t rtd_init = get_rx_timestamp_u64()
								- get_tx_timestamp_u64();
						uint64_t rtd_resp = resp_tx_timestamp
								- poll_rx_timestamp;

						// Read carrier integrator value and calculate clock offset ratio
						float clockOffsetRatio = dwt_readclockoffset()
								/ (float) (1 << 26);

						distance_ptr->save(rtd_init, rtd_resp,
								clockOffsetRatio);

						tag.readings++;

						tag.Battery_Voltage = *(const uint16_t*) (rx_buffer
								+ BATTERY_VOLTAGE_RAW_OFFSET);

						debug_distance_new(tag, tag_status, distance_a, distance_b);

						if (interfaz_state == MASTER_ONE_DETECTION) {
							tag_status = TAG_ONE_DETECTION;
						}
						else
							tag_status = TAG_SEND_TIMESTAMP_QUERY;
					}
					else
						tag_status =TAG_RX_NO_COMMAND;
				}
			}

			if ((tag_status != TAG_SEND_TIMESTAMP_QUERY)
					&& (tag_status != TAG_ONE_DETECTION))
				tag_status = TAG_DISCOVERY;
			else
				query_ticks = HAL_GetTick();
			converse_Tag_Battery_Voltage(&tag);

		} else if (tag_status == TAG_SEND_TIMESTAMP_QUERY) {

			if (tag.readings < DISTANCE_READINGS - 1) {
				hw_device = switch_hw_timestamp_query(&tag, distance_ptr, hw, &distance_a,
						&distance_b);
				tag.command = TAG_TIMESTAMP_QUERY;
			} else {
				tag.command = TAG_SET_SLEEP_MODE;
				hw_device = switch_hw_timestamp_query(&tag, distance_ptr, hw, &distance_a,
						&distance_b);
			}

			uint8_t rx_buffer[FRAME_LEN_MAX_EX] = { 0 }; // verificar el size del buffer de recepcion.

			tag_status = tag_receive_cmd(&tag, rx_buffer, distance_a, distance_b);

			if (tag_status == TAG_RX_CRC_VALID) {
				tag.command = rx_buffer[0];

				const int poll_rx_offset = 1;
				const int resp_tx_offset = 1 + 4;

				// Get timestamps embedded in response message
				uint32_t poll_rx_timestamp = *(uint32_t*) (rx_buffer
						+ poll_rx_offset);
				uint32_t resp_tx_timestamp = *(uint32_t*) (rx_buffer
						+ resp_tx_offset);
				uint64_t rtd_init = get_rx_timestamp_u64()
						- get_tx_timestamp_u64();
				uint64_t rtd_resp = resp_tx_timestamp - poll_rx_timestamp;

				// Read carrier integrator value and calculate clock offset ratio
				float clockOffsetRatio = dwt_readclockoffset()
						/ (float) (1 << 26);

				switch (tag.command) {
				case TAG_TIMESTAMP_QUERY:
					distance_ptr->save(rtd_init, rtd_resp, clockOffsetRatio);
					tag_status = TAG_SEND_TIMESTAMP_QUERY;
					break;
				case TAG_SET_SLEEP_MODE:
					distance_ptr->save(rtd_init, rtd_resp, clockOffsetRatio);
					tag_status = TAG_END_READINGS;
					break;
				default:
					tag_status = TAG_RX_NO_COMMAND;
					break;
				}
				if ((tag_status == TAG_END_READINGS)) {

					debug_distance_new(tag, tag_status, distance_a, distance_b);
					saveTagIfNeeded(&tag, &distance_a, &distance_b, &list,
							&list_od);
					tag_status = TAG_DISCOVERY;
					debug_count = 0;
				} else if (tag_status == TAG_SEND_TIMESTAMP_QUERY) {
					tag.readings++;
					debug_distance_new(tag, tag_status, distance_a, distance_b);
				} else {
					saveTagIfNeeded(&tag, &distance_a, &distance_b, &list,
							&list_od);
					tag_status = TAG_DISCOVERY;
				}

			} else {

//				debug_distance_new(tag, tag_status, distance_a, distance_b);
				distance_ptr->error_crc_increment();
				tag_status = TAG_SEND_TIMESTAMP_QUERY;
			}

			if (HAL_GetTick() - query_ticks > query_timeout) {
				debug_distance_new(tag, tag_status, distance_a, distance_b);
				saveTagIfNeeded(&tag, &distance_a, &distance_b, &list,
						&list_od);
				tag_status = TAG_DISCOVERY;
				debug_count = 0;
			}

		} else if (tag_status == TAG_ONE_DETECTION) {
			tag.command = TAG_SET_SLEEP_MODE;
			HAL_Delay(1);
			tag_status = tag_response(&tag);

			lora_send_ticks = HAL_GetTick();
			insert_tag(&list_od, tag);
			tag_status = TAG_DISCOVERY;
			reset_TAG_values(&tag);
			debug_count = 0;
		}

//----------------ENVIO DE FRAME CON DATA DE TAGS----------------

		uint8_t MAX_TAG_NUMBER_MULTIPLE_DETECTTION = 27;
		uint8_t MAX_TAG_NUMBER_ONE_DETECTTION = 48;

		if ((((HAL_GetTick() - lora_send_ticks) > lora_send_timeout)
				&& (list.count > 0))) {

			print_all_tags(&list, tag_status);
			print_serialized_tags(&list);
			size_t tags_size = (list.count * SERIALIZED_TAG_SIZE)
					+ sizeof(list.count) * 2;
			uint8_t tx_arr[tags_size];
			uint8_t total_tags = list.count;

			while (list.head != NULL) {
				serialize_tag_list_limit(&list, tx_arr,
						MAX_TAG_NUMBER_MULTIPLE_DETECTTION, total_tags);
				std::vector<uint8_t> tx_vect(tx_arr, tx_arr + tags_size);
				cmd.composeMessage(&tx_vect);
				std::vector<uint8_t> message_composed =
						cmd.get_composed_message();
				do {
//				gpio_handler.off(lora_rx_led);
					uint8_t DATA_SIZE = txlora.receive(recvChar,
							LINKMODE::UPLINK);
//				gpio_handler.on(lora_rx_led);
					if (DATA_SIZE == 0) {
//					gpio_handler.on(lora_tx_led);
						txlora.transmit(message_composed.data(),
								message_composed.size(), LINKMODE::UPLINK);
//					gpio_handler.off(lora_tx_led);
						memset(recvChar, 0, sizeof(recvChar));
						break;
					} else {
						uint32_t delay_ms = rand() % tiempo_max + 1;
						HAL_Delay(delay_ms);
					}
				} while (true);
				tx_vect.clear();
				free_tag_list_limit(&list, MAX_TAG_NUMBER_MULTIPLE_DETECTTION);
			}
			lora_send_ticks = HAL_GetTick();
		}

		else if (((HAL_GetTick() - lora_send_ticks) > lora_send_timeout)
				&& ((list_od.count > 0))) {

			print_all_tags(&list_od, tag_status);
			print_serialized_tags(&list_od);
			size_t tags_size = sizeof(sniffer_id) + sizeof(list_od.count) * 2
					+ (list_od.count * SERIALIZED_TAG_SIZE_ONE_DETECTION);
			uint8_t tx_arr[tags_size];
			uint8_t total_tags = list_od.count;

			while (list_od.head != NULL) {
				serialize_tag_list_limit_od(&list_od, tx_arr,
						MAX_TAG_NUMBER_MULTIPLE_DETECTTION, total_tags,
						sniffer_id);
				std::vector<uint8_t> tx_vect(tx_arr, tx_arr + tags_size);
				cmd.setCommandId(ONE_DETECTION);
				cmd.composeMessage(&tx_vect);
				std::vector<uint8_t> message_composed =
						cmd.get_composed_message();
				do {
//				gpio_handler.off(lora_rx_led);
					uint8_t DATA_SIZE = txlora.receive(recvChar,
							LINKMODE::UPLINK);
//				gpio_handler.on(lora_rx_led);
					if (DATA_SIZE == 0) {
//					gpio_handler.on(lora_tx_led);
						txlora.transmit(message_composed.data(),
								message_composed.size(), LINKMODE::UPLINK);
//					gpio_handler.off(lora_tx_led);
						memset(recvChar, 0, sizeof(recvChar));
						break;
					} else {
						uint32_t delay_ms = rand() % tiempo_max + 1;
						HAL_Delay(delay_ms);
					}
				} while (true);
				tx_vect.clear();
				free_tag_list_limit(&list_od, MAX_TAG_NUMBER_ONE_DETECTTION);
			}
			lora_send_ticks = HAL_GetTick();
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
static void MX_SPI1_Init(void) {

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
	if (HAL_SPI_Init(&hspi1) != HAL_OK) {
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
static void MX_SPI2_Init(void) {

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
	if (HAL_SPI_Init(&hspi2) != HAL_OK) {
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
static void MX_SPI3_Init(void) {

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
	if (HAL_SPI_Init(&hspi3) != HAL_OK) {
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
