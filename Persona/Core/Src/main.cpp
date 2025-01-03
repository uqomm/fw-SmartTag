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
#include "bq25150.hpp"
#include "PcbLed.hpp"
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

I2C_HandleTypeDef hi2c1;

LPTIM_HandleTypeDef hlptim1;

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim3;
DMA_HandleTypeDef handle_GPDMA1_Channel2;

/* USER CODE BEGIN PV */
uint32_t Counter_INT = 0;
uint8_t Counter_INT_Led3 = 0;
bool Power_Good = 1;

uint16_t adc_reg = 0;
GPIO_PinState NPG = GPIO_PIN_SET;
float vbat = 0;
Gpio NCE(CE_GPIO_Port, CE_Pin);
Gpio MR(MR_GPIO_Port, MR_Pin);
Gpio PG(PG_GPIO_Port, PG_Pin);
Gpio NLP(LP_GPIO_Port, LP_Pin);

GpioHandler pins;
Bq25155 battery_charger(MR, NCE, &hi2c1);
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_GPDMA1_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI2_Init(void);
static void MX_FLASH_Init(void);
static void MX_LPTIM1_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Callback: timer has rolled over
uint16_t pwmData2[24];

void Led_OnOff(const Gpio &VddLed, Ws2812Color led[1], PcbLed &pcb_led,
		bool _charger_state) {

	if (_charger_state == true) {
		pins.on(VddLed);
		if (tag->Voltaje_Bat <= 0xA222)
			pcb_led.set_and_send_led_color(led, 1, 15, Color::RED);
		else if ((0xA222 < tag->Voltaje_Bat) && (tag->Voltaje_Bat <= 0xAAAA))
			pcb_led.set_and_send_led_color(led, 1, 15, Color::YELLOW);
		else
			pcb_led.set_and_send_led_color(led, 1, 15, Color::GREEN);
		pins.off(VddLed);
	}

}

void sleep_in_out(dwt_config_t defatult_dwt_config,
		dwt_txconfig_t defatult_dwt_txconfig, dwt_local_data_t dwt_local_data,
		uint32_t sleep_time) {
	HAL_GPIO_WritePin(hw.nrstPort, hw.nrstPin, GPIO_PIN_RESET); /* Target specific drive of RSTn line into DW IC low for a period. */

	// Calcula el periodo del LPTIM para el sleep_time
	uint32_t lpt_period = (uint32_t) (sleep_time * 0.25f); //sleep_time in ms
	HAL_LPTIM_TimeOut_Start_IT(&hlptim1, lpt_period);

	// Entra en modo STOP
	HAL_SuspendTick();
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	HAL_ResumeTick();
	SystemClock_Config();
	HAL_GPIO_WritePin(hw.nrstPort, hw.nrstPin, GPIO_PIN_SET);
	if (tag_init(&defatult_dwt_config, &defatult_dwt_txconfig, &dwt_local_data,
			running_device, RATE_6M8) == 1)
		Error_Handler();
	tag->readings = 0;
	tag->sniffer_state = MASTER_ONE_DETECTION;
	tag->distance_a = 0;
	tag->distance_b = 0;
	tag_status = TAG_WAIT_FOR_FIRST_DETECTION;
}

void sleep_in_out_new(dwt_config_t defatult_dwt_config,
		dwt_txconfig_t defatult_dwt_txconfig, dwt_local_data_t dwt_local_data,
		uint8_t sleep_time) {
	HAL_GPIO_WritePin(hw.nrstPort, hw.nrstPin, GPIO_PIN_RESET); /* Target specific drive of RSTn line into DW IC low for a period. */
	HAL_LPTIM_TimeOut_Start_IT(&hlptim1,125);
	// Entra en modo STOP
	HAL_SuspendTick();
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	HAL_ResumeTick();
	SystemClock_Config();
	HAL_GPIO_WritePin(hw.nrstPort, hw.nrstPin, GPIO_PIN_SET);
	if (tag_init(&defatult_dwt_config, &defatult_dwt_txconfig, &dwt_local_data,
			running_device, RATE_6M8) == 1)
		Error_Handler();
	tag->readings = 0;
	tag->sniffer_state = MASTER_ONE_DETECTION;
	tag->distance_a = 0;
	tag->distance_b = 0;
	tag_status = TAG_WAIT_FOR_FIRST_DETECTION;
}

void battery_charging_led_on_off(const Gpio &VddLed, uint8_t STAT0_Reg,
		uint8_t charge_const, Ws2812Color led[1], uint8_t charge_done,
		PcbLed &pcb_led) {
	if (Power_Good == false) {
		battery_charger.register_init_all_2();
		HAL_Delay(1);
		pins.off(NCE);
		pins.on(VddLed);
		STAT0_Reg = battery_charger.register_STAT0();
		if (charge_const & STAT0_Reg)
			pcb_led.set_and_send_led_color(led, 1, 0, Color::YELLOW);
		else if (charge_done & STAT0_Reg)
			pcb_led.set_and_send_led_color(led, 1, 0, Color::GREEN);
		else
			pcb_led.set_and_send_led_color(led, 1, 0, Color::RED);
	} else
		pins.off(VddLed);
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
  MX_GPDMA1_Init();
  MX_I2C1_Init();
  MX_SPI2_Init();
  MX_FLASH_Init();
  MX_LPTIM1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
//	pins.off(NCE);
	pins.on(NLP);

	__HAL_RCC_LPTIM1_CLKAM_ENABLE();
	__HAL_RCC_LPTIM3_CLKAM_ENABLE();

	HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);
	HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN2);
	HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN3);
	HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN4);
	HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN5);
	HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN6);
	HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN7);
	HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN8);

	HAL_PWR_DisablePVD();

	//battery_charger.register_init_all();
	battery_charger.register_init_all_2();
	//battery_charger.register_Factory_Reset();




	//HAL_GPIO_WritePin(GPIOA, DW3000_RST_Pin, GPIO_PIN_SET);
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

	HAL_GPIO_WritePin(hw.nrstPort, hw.nrstPin, GPIO_PIN_RESET);/* Target specific drive of RSTn line into DW IC low for a period. */
	HAL_Delay(1);
	HAL_GPIO_WritePin(hw.nrstPort, hw.nrstPin, GPIO_PIN_SET);
	if (tag_init(&defatult_dwt_config, &defatult_dwt_txconfig, &dwt_local_data,
			running_device, RATE_6M8) == 1)
		Error_Handler();

	/* Frames used in the ranging process. See NOTE 2 below. */

	/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
	tag = (TAG_t*) malloc(sizeof(TAG_t));
	if (tag == NULL)
		Error_Handler();

	GpioHandler g = GpioHandler();
	PcbLed pcb_led = PcbLed(&htim3, TIM_CHANNEL_2);
	Ws2812Color led[1] = { Ws2812Color() };
	Gpio VddLed(LED_BAT_GPIO_Port, LED_BAT_Pin);

	tag->readings = 0;
	tag->id = 0;
	tag->resp_tx_time = 0;
	tag->resp_tx_timestamp = 0;
	tag->poll_rx_timestamp = 0;
	tag->Voltaje_Bat = 0;
	tag->sniffer_state = MASTER_ONE_DETECTION;
	tag_status = TAG_WAIT_FOR_FIRST_DETECTION;
	tag->id = _dwt_otpread(PARTID_ADDRESS);
	dwt_configuresleep(DWT_RUNSAR, DWT_WAKE_WUP);
	dwt_configuresleepcnt(4095);
	tag->sleep_time = 1000;
	tag->sleep_time_not_recived = 500;
	tag->sleep_time_recived = 15000;
	tag->ship_mode = SHIP_MODE_OFF;

	uint32_t sleep_time_not_recived = 500;
	uint32_t sleep_time_recived = 15000;
	uint32_t sleep_time = 1000;

	uint8_t STAT0_Reg = 0;
	uint8_t charge_done = 0x20;
	uint8_t charge_const = 0x40;

	uint32_t query_timeout = 1000;
	uint32_t query_ticks;

	battery_charger.enter_ship_mode();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		switch (tag_status) {
		case TAG_WAIT_FOR_FIRST_DETECTION:
			battery_charger.manual_read_adc_bat(); //manual_read_adc_bat();
			HAL_Delay(1);
			tag->Voltaje_Bat = battery_charger.register_adc_bat();

			tag_status = process_first_tag_information(tag);
			if (tag_status == TAG_WAIT_FOR_TIMESTAMPT_QUERY)
				query_ticks = HAL_GetTick();

			if ((tag_status != TAG_WAIT_FOR_TIMESTAMPT_QUERY)
					&& (tag_status != TAG_WAIT_SEND_TX) && (tag_status != TAG_SLEEP_NOT_RECIVED)) {
				tag_status = TAG_SLEEP;
			}
			break;

		case TAG_WAIT_SEND_TX:
			HAL_Delay(1); // TODO quizas hall delay 2
			tag_status = process_second(tag);
			if (tag_status == TAG_SLEEP)
				tag_status = TAG_SLEEP_RECIVED;
			else if (tag_status != TAG_SHIP_MODE_SET)
				tag_status = TAG_SLEEP_NOT_RECIVED;
			break;

		case TAG_WAIT_FOR_TIMESTAMPT_QUERY:
			tag_status = process_queried_tag_information(tag);

			if (tag_status == TAG_TX_SUCCESS) {
				if (tag->command == TAG_TIMESTAMP_QUERY)
					tag_status = TAG_WAIT_FOR_TIMESTAMPT_QUERY;
				else if (tag->command == TAG_SET_SLEEP_MODE) {
					tag_status = TAG_SLEEP_RECIVED;
				}
			} else if (tag_status == TAG_RX_COMMAND_ERROR) {
				HAL_GPIO_WritePin(hw.nrstPort, hw.nrstPin, GPIO_PIN_RESET);
				HAL_Delay(1);
				HAL_GPIO_WritePin(hw.nrstPort, hw.nrstPin, GPIO_PIN_SET);
				if (tag_init(&defatult_dwt_config, &defatult_dwt_txconfig,
						&dwt_local_data, running_device, RATE_6M8) == 1)
					Error_Handler();
				tag_status = TAG_WAIT_FOR_FIRST_DETECTION;
			}

			else if (tag_status != TAG_SLEEP) {
				HAL_Delay(1);
				tag_status = TAG_WAIT_FOR_TIMESTAMPT_QUERY;
			}

			if (HAL_GetTick() - query_ticks > query_timeout) {
				if ((tag->distance_a > 0) && (tag->distance_b > 0))
					tag_status = TAG_SLEEP_RECIVED;
				else
					tag_status = TAG_SLEEP;
			}
			break;

		case TAG_SHIP_MODE_SET:
			battery_charger.enter_ship_mode();
			break;

		case TAG_SLEEP_RECIVED:
			battery_charging_led_on_off(VddLed, STAT0_Reg, charge_const, led, charge_done, pcb_led);

			Counter_INT_Led3 = 0;
			Led_OnOff(VddLed, led, pcb_led, Power_Good);

			sleep_in_out(defatult_dwt_config, defatult_dwt_txconfig,
					dwt_local_data, sleep_time_recived);
			break;

		case TAG_SLEEP_NOT_RECIVED:
			battery_charging_led_on_off(VddLed, STAT0_Reg, charge_const, led,
					charge_done, pcb_led);

			if ((Counter_INT_Led3) > 14) {
				Counter_INT_Led3 = 0;
				Led_OnOff(VddLed, led, pcb_led, Power_Good);
			}
			sleep_in_out(defatult_dwt_config, defatult_dwt_txconfig,
					dwt_local_data, sleep_time_not_recived);
			break;

		default:
			battery_charging_led_on_off(VddLed, STAT0_Reg, charge_const, led, charge_done, pcb_led);

			if ((Counter_INT_Led3) > 14) {
				Counter_INT_Led3 = 0;
				Led_OnOff(VddLed, led, pcb_led, Power_Good);
			}
			sleep_in_out_new(defatult_dwt_config, defatult_dwt_txconfig,
					dwt_local_data, sleep_time);

			Counter_INT_Led3++;
			break;

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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_0;
  RCC_OscInitStruct.LSIDiv = RCC_LSI_DIV1;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV4;
  RCC_OscInitStruct.PLL.PLLM = 3;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 1;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV4;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief FLASH Initialization Function
  * @param None
  * @retval None
  */
static void MX_FLASH_Init(void)
{

  /* USER CODE BEGIN FLASH_Init 0 */

  /* USER CODE END FLASH_Init 0 */

  /* USER CODE BEGIN FLASH_Init 1 */

  /* USER CODE END FLASH_Init 1 */
  if (HAL_FLASH_Unlock() != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_FLASH_Lock() != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN FLASH_Init 2 */

  /* USER CODE END FLASH_Init 2 */

}

/**
  * @brief GPDMA1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPDMA1_Init(void)
{

  /* USER CODE BEGIN GPDMA1_Init 0 */

  /* USER CODE END GPDMA1_Init 0 */

  /* Peripheral clock enable */
  __HAL_RCC_GPDMA1_CLK_ENABLE();

  /* GPDMA1 interrupt Init */
    HAL_NVIC_SetPriority(GPDMA1_Channel2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel2_IRQn);

  /* USER CODE BEGIN GPDMA1_Init 1 */

  /* USER CODE END GPDMA1_Init 1 */
  /* USER CODE BEGIN GPDMA1_Init 2 */

  /* USER CODE END GPDMA1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00303D5B;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief LPTIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPTIM1_Init(void)
{

  /* USER CODE BEGIN LPTIM1_Init 0 */

  /* USER CODE END LPTIM1_Init 0 */

  /* USER CODE BEGIN LPTIM1_Init 1 */

  /* USER CODE END LPTIM1_Init 1 */
  hlptim1.Instance = LPTIM1;
  hlptim1.Init.Clock.Source = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
  hlptim1.Init.Clock.Prescaler = LPTIM_PRESCALER_DIV128;
  hlptim1.Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
  hlptim1.Init.Period = 30000;
  hlptim1.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
  hlptim1.Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;
  hlptim1.Init.Input1Source = LPTIM_INPUT1SOURCE_GPIO;
  hlptim1.Init.Input2Source = LPTIM_INPUT2SOURCE_GPIO;
  hlptim1.Init.RepetitionCounter = 0;
  if (HAL_LPTIM_Init(&hlptim1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPTIM1_Init 2 */

  /* USER CODE END LPTIM1_Init 2 */

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

  SPI_AutonomousModeConfTypeDef HAL_SPI_AutonomousMode_Cfg_Struct = {0};

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
  hspi2.Init.CRCPolynomial = 0x7;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  hspi2.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
  hspi2.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerState = SPI_AUTO_MODE_DISABLE;
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerSelection = SPI_GRP1_GPDMA_CH0_TCF_TRG;
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerPolarity = SPI_TRIG_POLARITY_RISING;
  if (HAL_SPIEx_SetConfigAutonomousMode(&hspi2, &HAL_SPI_AutonomousMode_Cfg_Struct) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
	htim3.Instance = TIM3;
	htim3.Init.Prescaler = 0;
	htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim3.Init.Period = 39;
	htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK) {
		Error_Handler();
	}
	if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 0;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2)
			!= HAL_OK) {
		Error_Handler();
	}
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, CE_Pin|LP_Pin|MR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DW3000_RST_RCV_GPIO_Port, DW3000_RST_RCV_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_BAT_GPIO_Port, LED_BAT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : CE_Pin LP_Pin MR_Pin */
  GPIO_InitStruct.Pin = CE_Pin|LP_Pin|MR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PG_Pin */
  GPIO_InitStruct.Pin = PG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(PG_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : DW3000_RST_RCV_Pin */
  GPIO_InitStruct.Pin = DW3000_RST_RCV_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DW3000_RST_RCV_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI2_CS_Pin */
  GPIO_InitStruct.Pin = SPI2_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SPI2_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_BAT_Pin */
  GPIO_InitStruct.Pin = LED_BAT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_BAT_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI5_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// Función de callback para manejar la interrupción
void HAL_LPTIM_CompareMatchCallback(LPTIM_HandleTypeDef *hlptim) {
	HAL_LPTIM_Counter_Stop_IT(&hlptim1);
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
	HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_2);
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin) {
	Power_Good = 0;
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin) {
	Power_Good = 1;
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
