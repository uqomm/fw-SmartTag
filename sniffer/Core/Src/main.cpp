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
#include <Lora.hpp>
#include "GpioHandler.hpp"
#include "UartHandler.hpp"
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
uint8_t lora_rcv_bytes = 0;
uint8_t Data_reciv_test[256] = {0};
double *distance_ptr;
TAG_t *tag;
int size = 0;
uint8_t running_device = DEV_UWB3000F27;
Uwb_HW_t uwb_hw_a;
Uwb_HW_t uwb_hw_b;
Uwb_HW_t *hw;
dwt_local_data_t *pdw3000local;
uint8_t crcTable[256];
uint8_t recvChar[255] = {0};
TAG_t *tag_ptr;
DistanceHandler distance_a = DistanceHandler(DISTANCE_READINGS);
DistanceHandler distance_b = DistanceHandler(DISTANCE_READINGS);
uint32_t tiempo_max = 5;
GpioHandler gpio_handler = GpioHandler();

uint8_t lora_rcv_buffer[256] = {0};

std::vector<uint8_t> message_composed;
std::vector<uint8_t> tx_vect;



uint8_t bytes_reciv;
uint8_t data_reciv[255] = { 0 };
UartHandler uart_cfg = UartHandler(&huart1);
uint8_t FinalData[20];
uint8_t RxData[20];
uint8_t temp[2];
int indx = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_I2C3_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_SPI3_Init(void);

void sendMessageWithRetry(const Gpio &lora_rx_led,
						  const Gpio &lora_tx_led,
						  Lora &txlora,
						  std::vector<uint8_t> &message_composed)
{
	do
	{
		gpio_handler.off(lora_rx_led);
		bool DATA_SIZE = txlora.channel_detection();
		gpio_handler.on(lora_rx_led);
		if (DATA_SIZE == 0)
		{
			gpio_handler.off(lora_tx_led);
			if (txlora.transmit(message_composed.data(),
								message_composed.size(), LINKMODE::UPLINK) == HAL_OK)
			{
			}
			gpio_handler.on(lora_tx_led);
			break;
		}
		else
		{
			uint32_t delay_ms = (rand() % tiempo_max) + 230;
			HAL_Delay(delay_ms);
		}
	} while (true);
}

size_t calculate_serialize_size(uint32_t sniffer_id,
								int tag_map_size,
								uint8_t serialized_tag_size,
								uint8_t max_tag_number){
	size_t tags_size;

	if (tag_map_size >= max_tag_number)
		tags_size = sizeof(sniffer_id) + sizeof(uint8_t) * 2
				+ (max_tag_number * serialized_tag_size);
	else
		tags_size = sizeof(sniffer_id) + sizeof(uint8_t) * 2
				+ (tag_map_size * serialized_tag_size);
	return tags_size;
}


void sendLoRaMessage(std::map<uint32_t, TAG_t> &tag_map,
					 uint8_t max_tag_number,
					 uint8_t command_id,
					 uint8_t serialized_tag_size,
					 uint32_t lora_send_ticks,
					 uint32_t lora_send_timeout,
					 uint32_t sniffer_id,
					 const Gpio &lora_rx_led,
					 const Gpio &lora_tx_led,
					 Lora &txlora,
					 Sniffer_State interfaz_state)
{

	CommandMessage cmd_lora_send(static_cast<uint8_t>(MODULE_FUNCTION::SNIFFER), 0x00);
	print_qty_tags(tag_map.size());
	print_serialized_cplusplus(&tag_map, max_tag_number, tag_map.size(),
							   sniffer_id, interfaz_state, serialized_tag_size);

	// Prepare serialization
	size_t tags_size = calculate_serialize_size(sniffer_id,
												tag_map.size(),
												serialized_tag_size,
												max_tag_number);
	uint8_t serialized_tags[tags_size] = {0};
	uint8_t total_tags = tag_map.size();

	while (tag_map.size() > 0)
	{
		serialize_limit_cplusplus(&tag_map, serialized_tags, max_tag_number,
								  total_tags, sniffer_id, interfaz_state);
		erase_limit_map(&tag_map, max_tag_number);


		for (int i = 0; i < tags_size; i++)
			tx_vect.push_back(serialized_tags[i]);


		 cmd_lora_send.setCommandId(command_id);
		 cmd_lora_send.composeMessage(&tx_vect);
		tx_vect.clear();
		message_composed = cmd_lora_send.get_composed_message();
		sendMessageWithRetry(lora_rx_led, lora_tx_led, txlora,
							 message_composed);
	}
	message_composed.clear();
	cmd_lora_send.reset();

	lora_send_ticks = HAL_GetTick();
}

bool is_ready_to_send(uint32_t lora_send_ticks, uint32_t lora_send_timeout,
                      int size, Sniffer_State interfaz_state, Sniffer_State expected_state) {
    return (HAL_GetTick() - lora_send_ticks > lora_send_timeout) &&
           (size > 0) &&
           (interfaz_state == expected_state);
}

void send_lora_message_if_ready(uint32_t *lora_send_ticks, uint32_t lora_send_timeout, std::map<uint32_t, TAG_t> &tag_map,
                             Sniffer_State interfaz_state, Sniffer_State expected_state, int max_tags, 
                             uint8_t command_id, int tag_size, uint32_t sniffer_id, 
							 const Gpio& lora_rx_led, const Gpio& lora_tx_led,
							 Lora& txlora) { // Assuming these are the correct types for your arguments

    if (is_ready_to_send(*lora_send_ticks, lora_send_timeout, tag_map.size(), interfaz_state, expected_state)) {
        sendLoRaMessage(tag_map, max_tags, command_id, tag_size, *lora_send_ticks, lora_send_timeout,
                        sniffer_id, lora_rx_led, lora_tx_led, txlora, interfaz_state);
        tag_map.clear(); // Clear the tag map *after* sending.
        *lora_send_ticks = HAL_GetTick();

    }
}

void send_lora_message_if_not_ready(uint32_t *lora_send_ticks, uint32_t lora_send_timeout, uint32_t sniffer_id,
							 const Gpio& lora_rx_led, const Gpio& lora_tx_led,
							 Lora& txlora) { // Assuming these are the correct types for your arguments

	if (HAL_GetTick() - *lora_send_ticks > lora_send_timeout){
		CommandMessage cmd_lora_send(static_cast<uint8_t>(MODULE_FUNCTION::SNIFFER),
					0x00);
			print_qty_tags(0);

			uint8_t tags_size = sizeof(sniffer_id) + sizeof(uint8_t) * 2;
			uint8_t serialized_tags[tags_size] = {0};
			serialize_header(0, serialized_tags, 0, 0, sniffer_id);

			for (int i = 0; i < tags_size; i++)
				tx_vect.push_back(serialized_tags[i]);

			cmd_lora_send.setCommandId(ONE_DETECTION);
			cmd_lora_send.composeMessage(&tx_vect);
			tx_vect.clear();
			message_composed = cmd_lora_send.get_composed_message();
			sendMessageWithRetry(lora_rx_led, lora_tx_led, txlora, message_composed);

			message_composed.clear();
			cmd_lora_send.reset();

			*lora_send_ticks = HAL_GetTick();
	}
}


uint8_t save_map_and_clear_tag(TAG_t *tag, DistanceHandler *distance_a, DistanceHandler *distance_b, std::map<uint32_t, TAG_t> *tag_map_od)
{ // list como puntero
	uint8_t _found = 0;
	_found = insert_tag_cplusplus(tag_map_od, tag);
	reset_TAG_values(tag);
	distance_a->clear();
	distance_b->clear();
	return _found;
}

void save_two_maps_and_clear_tag(DistanceHandler &distance_a, DistanceHandler &distance_b, std::map<uint32_t, TAG_t> &tag_map_od, TAG_t &tag, uint32_t &lora_send_ticks, std::map<uint32_t, TAG_t> &tag_map)
{
	uint8_t _found_new = 0;

	_found_new = save_at_least_one_distance(&tag, &distance_a, &distance_b, &tag_map);
	if (_found_new == 0)
		lora_send_ticks = HAL_GetTick();

	_found_new = save_map_and_clear_tag(&tag, &distance_a, &distance_b, &tag_map_od);
	if (_found_new == 0)
		lora_send_ticks = HAL_GetTick();
}

/* USER CODE BEGIN PFP */

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
	MX_USART1_UART_Init();
	MX_I2C3_Init();
	MX_SPI1_Init();
	MX_SPI2_Init();
	MX_SPI3_Init();
	/* USER CODE BEGIN 2 */
	pdw3000local = new dwt_local_data_t;

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
	tag.sleep_time_recived = 15; //time in seconds
	tag.sleep_time_not_recived = 5;  //(time in seconds)*10
	tag.ship_mode = SHIP_MODE_OFF;
	bool switch_ship_mode = 0;

	uint32_t sniffer_id = _dwt_otpread(PARTID_ADDRESS);

	Sniffer_State interfaz_state = MASTER_ONE_DETECTION; // MASTER_MULTIPLE_DETECTION      MASTER_ONE_DETECTION									  // tiempo de sleep
	TAG_STATUS_t tag_status = TAG_DISCOVERY;
	uint32_t lora_send_timeout = 5000;
	uint32_t lora_send_timeout_for_not_detection = 10000;
	uint32_t lora_send_ticks = HAL_GetTick();
	uint32_t query_timeout = 1000;
	uint32_t query_ticks;
	uint8_t type = 0;

	DistanceHandler *distance_ptr;

	Memory eeprom = Memory(&hi2c3);


	Gpio rx_lora_nss = Gpio(LORA_RX_NSS_GPIO_Port, LORA_RX_NSS_Pin);
	Gpio rx_lora_rst = Gpio(LORA_RX_NRST_GPIO_Port, LORA_RX_NRST_Pin);
	Gpio tx_lora_nss = Gpio(LORA_TX_NSS_GPIO_Port, LORA_TX_NSS_Pin);
	Gpio tx_lora_rst = Gpio(LORA_TX_NRST_GPIO_Port, LORA_TX_NRST_Pin);
	Lora rxlora = Lora(rx_lora_nss, rx_lora_rst, &hspi2, &eeprom, type);
	Lora txlora = Lora(tx_lora_nss, tx_lora_rst, &hspi1, &eeprom);
//	rxlora.set_lora_settings(LoraBandWidth::BW_500KHZ, CodingRate::CR_4_6,
//							 SpreadFactor::SF_7, DOWNLINK_FREQ, UPLINK_FREQ);
//	txlora.set_lora_settings(LoraBandWidth::BW_500KHZ, CodingRate::CR_4_6,
//							 SpreadFactor::SF_7, DOWNLINK_FREQ, UPLINK_FREQ);

	txlora.check_already_store_data();
	rxlora.check_already_store_data();

	rxlora.set_rx_continuous_mode(LINKMODE::DOWNLINK);

	Gpio lora_rx_led = Gpio(LORA_RX_LED_GPIO_Port, LORA_RX_LED_Pin);
	Gpio lora_tx_led = Gpio(LORA_TX_LED_GPIO_Port, LORA_TX_LED_Pin);
	Gpio switch_ship_mode_state = Gpio(SWITCH_SHIP_MODE_GPIO_Port,SWITCH_SHIP_MODE_Pin);

	uint8_t recive_test[255] = {0};

	std::map<uint32_t, TAG_t> tag_map;
	std::map<uint32_t, TAG_t> tag_map_od;

	uart_cfg.enable_receive_interrupt();


	CommandMessage command = CommandMessage(
				static_cast<uint8_t>(MODULE_FUNCTION::SNIFFER), 0x00);

	message_composed.reserve(255);
	tx_vect.reserve(255);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{

		//---------------- Busqueda y guardado de tags ----------------

		if (tag_status == TAG_DISCOVERY)
		{

			switch_hw(&tag, distance_ptr, hw, &distance_a, &distance_b);
			uint8_t tx_buffer[TX_DISCOVERY_SIZE] = {0};
			uint8_t rx_buffer[FRAME_LEN_MAX_EX] = {0};
			uint32_t rx_buffer_size = 0;
			tag.command = TAG_ID_QUERY;

			tx_buffer[0] = tag.command;
			tx_buffer[1] = interfaz_state;
			tx_buffer[2] = tag.sleep_time_recived;
			tx_buffer[3] = DEV_UWB3000F27;


			tag_status = setup_and_transmit(&tag, tx_buffer,
											TX_DISCOVERY_SIZE, rx_buffer, &rx_buffer_size);

			if (tag_status == TAG_RX_CRC_VALID)
			{
				if (rx_buffer[3] == DEV_UWB3000F27)
				{
					tag_status = TAG_DISCOVERY;
				}
				else
				{
					if (tag.command == rx_buffer[0])
					{
						tag.id = *(const uint32_t *)(rx_buffer + 1);
//						debug_distance_new(tag, tag_status, distance_a, distance_b);

						const int poll_rx_offset = 5;
						const int resp_tx_offset = 9;

						// Get timestamps embedded in response message
						uint32_t poll_rx_timestamp = *(uint32_t *)(rx_buffer + poll_rx_offset);
						uint32_t resp_tx_timestamp = *(uint32_t *)(rx_buffer + resp_tx_offset);

						// Compute time of flight and distance, using clock offset ratio to correct for differing local and remote clock rates
						uint64_t rtd_init = get_rx_timestamp_u64() - get_tx_timestamp_u64();
						uint64_t rtd_resp = resp_tx_timestamp - poll_rx_timestamp;

						// Read carrier integrator value and calculate clock offset ratio
						float clockOffsetRatio = dwt_readclockoffset() / (float)(1 << 26);

						distance_ptr->save(rtd_init, rtd_resp,
										   clockOffsetRatio);

						tag.readings++;

						tag.Battery_Voltage = *(const uint16_t *)(rx_buffer + BATTERY_VOLTAGE_RAW_OFFSET);

						if (interfaz_state == MASTER_ONE_DETECTION)
						{
							tag_status = TAG_ONE_DETECTION;
						}
						else
							tag_status = TAG_SEND_TIMESTAMP_QUERY;
					}
					else
						tag_status = TAG_RX_NO_COMMAND;
				}
			}

			if ((tag_status != TAG_SEND_TIMESTAMP_QUERY) && (tag_status != TAG_ONE_DETECTION))
				tag_status = TAG_DISCOVERY;
			else
				query_ticks = HAL_GetTick();
			converse_Tag_Battery_Voltage(&tag);
		}
		else if (tag_status == TAG_SEND_TIMESTAMP_QUERY)
		{

			if (tag.readings < distance_ptr->get_total_readings_for_two_transcievers() - 1)
			{
				switch_hw_timestamp_query(&tag, distance_ptr, hw, &distance_a,
										  &distance_b);
				tag.command = TAG_TIMESTAMP_QUERY;
			}
			else
			{
				tag.command = TAG_SET_SLEEP_MODE;
				switch_hw_timestamp_query(&tag, distance_ptr, hw, &distance_a,
										  &distance_b);
			}

			uint8_t rx_buffer[FRAME_LEN_MAX_EX] = {0}; // verificar el size del buffer de recepcion.

			tag_status = tag_receive_cmd(&tag, rx_buffer, distance_a, distance_b);

			if (tag_status == TAG_RX_CRC_VALID)
			{
				tag.command = rx_buffer[0];

				const int poll_rx_offset = 1;
				const int resp_tx_offset = 1 + 4;

				// Get timestamps embedded in response message
				uint32_t poll_rx_timestamp = *(uint32_t *)(rx_buffer + poll_rx_offset);
				uint32_t resp_tx_timestamp = *(uint32_t *)(rx_buffer + resp_tx_offset);
				uint64_t rtd_init = get_rx_timestamp_u64() - get_tx_timestamp_u64();
				uint64_t rtd_resp = resp_tx_timestamp - poll_rx_timestamp;

				// Read carrier integrator value and calculate clock offset ratio
				float clockOffsetRatio = dwt_readclockoffset() / (float)(1 << 26);

				switch (tag.command)
				{
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
				if ((tag_status == TAG_END_READINGS))
				{					//					uint8_t found = saveTagIfNeeded(&tag, &distance_a, &distance_b, &list,&list_od);
					save_two_maps_and_clear_tag(distance_a, distance_b, tag_map_od, tag, lora_send_ticks, tag_map);
					tag_status = TAG_DISCOVERY;
				}
				else if (tag_status == TAG_SEND_TIMESTAMP_QUERY)
				{
					tag.readings++;
				}
				else
				{
					//					uint8_t found = saveTagIfNeeded(&tag, &distance_a, &distance_b, &list, &list_od);
					save_two_maps_and_clear_tag(distance_a, distance_b, tag_map_od, tag, lora_send_ticks, tag_map);
					tag_status = TAG_DISCOVERY;
				}
			}
			else
			{
				distance_ptr->error_crc_increment();
				tag_status = TAG_SEND_TIMESTAMP_QUERY;
			}
			if (HAL_GetTick() - query_ticks > query_timeout)
			{
				save_two_maps_and_clear_tag(distance_a, distance_b, tag_map_od, tag, lora_send_ticks, tag_map);
				tag_status = TAG_DISCOVERY;
			}
		}
		else if (tag_status == TAG_ONE_DETECTION)
		{
			tag.command = TAG_SET_SLEEP_MODE;
			HAL_Delay(1);
			tag_status = tag_response(&tag);
//			debug_distance_new(tag, tag_status, distance_a, distance_b);
			uint8_t _found_new = save_map_and_clear_tag(&tag, &distance_a, &distance_b, &tag_map_od);
			if (_found_new == 0)
				lora_send_ticks = HAL_GetTick();

			tag_status = TAG_DISCOVERY;
		}

		//---------------- ENVIO DE FRAME CON DATA DE TAGS----------------

		uint8_t MAX_TAG_NUMBER_MULTIPLE_DETECTTION = 26;
		uint8_t MAX_TAG_NUMBER_ONE_DETECTTION = 48;

		if (tag_status == TAG_DISCOVERY)
		{

			if ((tag_map.size() == 0) && (tag_map_od.size() == 0)) {

				send_lora_message_if_not_ready(&lora_send_ticks,
						lora_send_timeout_for_not_detection, sniffer_id,
						lora_rx_led, lora_tx_led, txlora);

			} if (interfaz_state == MASTER_MULTIPLE_DETECTION){

				send_lora_message_if_ready(&lora_send_ticks, lora_send_timeout,
						tag_map, interfaz_state, MASTER_MULTIPLE_DETECTION,
						MAX_TAG_NUMBER_MULTIPLE_DETECTTION, MULTIPLE_DETECTION,
						SERIALIZED_TAG_SIZE, sniffer_id, lora_rx_led,
						lora_tx_led, txlora);

			} else if (interfaz_state == MASTER_ONE_DETECTION) {

				send_lora_message_if_ready(&lora_send_ticks, lora_send_timeout,
						tag_map_od, interfaz_state, MASTER_ONE_DETECTION,
						MAX_TAG_NUMBER_ONE_DETECTTION, ONE_DETECTION,
						SERIALIZED_TAG_SIZE_ONE_DETECTION, sniffer_id,
						lora_rx_led, lora_tx_led, txlora);
			}



			//---------------- Recepción por lora  ----------------
			lora_rcv_bytes = rxlora.read_data_after_lora_rx_done(lora_rcv_buffer);
			if ((lora_rcv_bytes > 0) && (lora_rcv_bytes <= 256))
			{
				gpio_handler.on(lora_rx_led);
				CommandMessage cmd = CommandMessage(
					static_cast<uint8_t>(MODULE_FUNCTION::SNIFFER), 0x00);
				STATUS status_data = cmd.validate_crc_ptrotocol(lora_rcv_buffer, lora_rcv_bytes);
				if (status_data == STATUS::RDSS_DATA_OK)
				{
					cmd.save_frame(lora_rcv_buffer, lora_rcv_bytes); // llegan 197 bytes
					std::vector<uint8_t> recived_data_lora = cmd.get_composed_message();
					uint32_t recived_id_from_lora;
					if (recived_data_lora.size() >= 4)
					{
						std::memcpy(&recived_id_from_lora, &recived_data_lora[0], sizeof(uint32_t));
						if (recived_id_from_lora == sniffer_id)
						{
							switch (cmd.getCommandId()){
							case CHANGE_SLEEP_TIME_RECIVED:{
								uint8_t _sleep_time_recived = recived_data_lora[4];
								if ((_sleep_time_recived <= 25)  && (_sleep_time_recived >= 5))
									tag.sleep_time_recived = _sleep_time_recived;
								break;
							}
							case CHANGE_SLEEP_NOT_RECIVED:{
								uint8_t _sleep_time_not_recived = recived_data_lora[4];
								if ((_sleep_time_not_recived < 10) && (_sleep_time_not_recived >= 5))
									tag.sleep_time_not_recived = recived_data_lora[4];
								break;
							}
							case ONE_DETECTION:{
								interfaz_state = MASTER_ONE_DETECTION;
								break;
							}
							case MULTIPLE_DETECTION:{
								interfaz_state = MASTER_MULTIPLE_DETECTION;
								break;
							}
							case CHANGE_TAG_READINGS:{
								uint8_t _total_readings = recived_data_lora[5];
								if (_total_readings % 2 == 0) {
									distance_a.change_total_readings(_total_readings);
									distance_b.change_total_readings(_total_readings);
								}
								break;
							}
							}
						}
					}
					recived_data_lora.clear();
				}
				cmd.reset();
				memset(lora_rcv_buffer, 0, sizeof(lora_rcv_buffer));
				gpio_handler.off(lora_rx_led);
			}

			//Activacion SHIP_MODE
			switch_ship_mode = gpio_handler.state(switch_ship_mode_state);
			if (switch_ship_mode == 0)
				tag.ship_mode = SHIP_MODE_OFF;
			else
				tag.ship_mode = SHIP_MODE_ON;

			//Configuracion Lora por UART

			uart_cfg.enable_receive_interrupt();
			if (bytes_reciv > 0) {

				STATUS status_data = command.validate(data_reciv, bytes_reciv);
				if (status_data == STATUS::CONFIG_FRAME) {

					switch (command.getCommandId()) {

					case QUERY_RX_FREQ: {
						// convertir float a arreglog the bytes (en este caso son 4 bytes)
						uint8_t freq_array[4];
						txlora.read_settings();
						uint32_t rx_freq = txlora.get_rx_frequency();
						float freqOut;
						freqOut = rx_freq / 1000000.0f;
						memcpy(freq_array, &freqOut, sizeof(freqOut));
						// crear commandMessage con trama y datos (en este caso con 4 bytes)
						command.set_message(freq_array, sizeof(freq_array));
						std::vector<uint8_t> message_composed =
								command.get_composed_message();
						command.composeMessage(&message_composed);
						message_composed = command.get_composed_message();
						uart_cfg.transmitMessage(message_composed.data(),
								message_composed.size());
						command.reset(1);
						message_composed.clear();
						break;
					}
					case QUERY_TX_FREQ: {
						uint8_t freq_array[4];
						rxlora.read_settings();
						uint32_t tx_freq = rxlora.get_tx_frequency();
						float freqOut;
						freqOut = tx_freq / 1000000.0f;
						memcpy(freq_array, &freqOut, sizeof(freqOut));
						command.set_message(freq_array, sizeof(freq_array));
						std::vector<uint8_t> message_composed =
								command.get_composed_message();
						command.composeMessage(&message_composed);
						message_composed = command.get_composed_message();
						uart_cfg.transmitMessage(message_composed.data(),
								message_composed.size());
						command.reset(1);
						message_composed.clear();
						break;
					}
					case QUERY_SPREAD_FACTOR: {
						uint8_t spread_array[1];
						txlora.read_settings();
						uint8_t spread_factor = txlora.get_spread_factor();
						spread_factor = spread_factor - SPREAD_FACTOR_OFFSET;
						memcpy(spread_array, &spread_factor,
								sizeof(spread_factor));
						command.set_message(spread_array, sizeof(spread_array));
						std::vector<uint8_t> message_composed =
								command.get_composed_message();
						command.composeMessage(&message_composed);
						message_composed = command.get_composed_message();
						uart_cfg.transmitMessage(message_composed.data(),
								message_composed.size());
						command.reset(1);
						message_composed.clear();
						break;
					}
					case QUERY_CODING_RATE: {
						uint8_t coding_array[1];
						txlora.read_settings();
						uint8_t coding_rate = txlora.get_coding_rate();
						memcpy(coding_array, &coding_rate, sizeof(coding_rate));
						command.set_message(coding_array, sizeof(coding_array));
						std::vector<uint8_t> message_composed =
								command.get_composed_message();
						command.composeMessage(&message_composed);
						message_composed = command.get_composed_message();
						uart_cfg.transmitMessage(message_composed.data(),
								message_composed.size());
						command.reset(1);
						message_composed.clear();
						break;
					}
					case QUERY_BANDWIDTH: {
						uint8_t bw_array[1];
						txlora.read_settings();
						uint8_t bw = txlora.get_bandwidth();
						bw = bw + BANDWIDTH_OFFSET;
						memcpy(bw_array, &bw, sizeof(bw));
						command.set_message(bw_array, sizeof(bw_array));
						std::vector<uint8_t> message_composed =
								command.get_composed_message();
						command.composeMessage(&message_composed);
						message_composed = command.get_composed_message();
						uart_cfg.transmitMessage(message_composed.data(),
								message_composed.size());
						command.reset(1);
						message_composed.clear();
						break;
					}
					case SET_TX_FREQ: {
						uint32_t freq = command.getDataAsUint32();
						int freq_int = command.freqDecode();
						freq = (uint32_t) freq_int;
						rxlora.set_tx_freq(freq);
						rxlora.save_settings();
						rxlora.configure_modem();
						uint8_t freq_array[4];
						rxlora.read_settings();
						uint32_t tx_freq = rxlora.get_tx_frequency();
						float freqOut;
						freqOut = tx_freq / 1000000.0f;
						memcpy(freq_array, &freqOut, sizeof(freqOut));
						command.set_message(freq_array, sizeof(freq_array));
						std::vector<uint8_t> message_composed =
								command.get_composed_message();
						command.composeMessage(&message_composed);
						message_composed = command.get_composed_message();
						uart_cfg.transmitMessage(message_composed.data(),
								message_composed.size());
						command.reset(1);
						message_composed.clear();

						break;
					}
					case SET_RX_FREQ: {
						uint32_t freq = command.getDataAsUint32();
						int freq_int = command.freqDecode();
						freq = (uint32_t) freq_int;
						txlora.set_rx_freq(freq);
						txlora.save_settings();
						txlora.configure_modem();
						uint8_t freq_array[4];
						txlora.read_settings();
						uint32_t rx_freq = txlora.get_rx_frequency();
						float freqOut;
						freqOut = rx_freq / 1000000.0f;
						memcpy(freq_array, &freqOut, sizeof(freqOut));
						command.set_message(freq_array, sizeof(freq_array));
						std::vector<uint8_t> message_composed =
								command.get_composed_message();
						command.composeMessage(&message_composed);
						message_composed = command.get_composed_message();
						uart_cfg.transmitMessage(message_composed.data(),
								message_composed.size());
						command.reset(1);
						message_composed.clear();
						break;
					}
					case SET_BANDWIDTH: {
						uint8_t bd = command.getDataAsUint8();
						bd = bd - BANDWIDTH_OFFSET;
						txlora.set_bandwidth(bd);
						txlora.save_settings();
						txlora.configure_modem();
						rxlora.set_bandwidth(bd);
						rxlora.save_settings();
						rxlora.configure_modem();
						uint8_t bw_array[1];
						txlora.read_settings();
						uint8_t bw = txlora.get_bandwidth();
						bw = bw + BANDWIDTH_OFFSET;
						memcpy(bw_array, &bw, sizeof(bw));
						command.set_message(bw_array, sizeof(bw_array));
						std::vector<uint8_t> message_composed =
								command.get_composed_message();
						command.composeMessage(&message_composed);
						message_composed = command.get_composed_message();
						uart_cfg.transmitMessage(message_composed.data(),
								message_composed.size());
						command.reset(1);
						message_composed.clear();
						break;
					}
					case SET_SPREAD_FACTOR: {
						uint8_t sf = command.getDataAsUint8();
						sf = sf + SPREAD_FACTOR_OFFSET;
						txlora.set_spread_factor(sf);
						txlora.save_settings();
						txlora.configure_modem();
						txlora.set_spread_factor(sf);
						rxlora.save_settings();
						rxlora.configure_modem();
						uint8_t spread_array[1];
						txlora.read_settings();
						uint8_t spread_factor = txlora.get_spread_factor();
						spread_factor = spread_factor - SPREAD_FACTOR_OFFSET;
						memcpy(spread_array, &spread_factor,
								sizeof(spread_factor));
						command.set_message(spread_array, sizeof(spread_array));
						std::vector<uint8_t> message_composed =
								command.get_composed_message();
						command.composeMessage(&message_composed);
						message_composed = command.get_composed_message();
						uart_cfg.transmitMessage(message_composed.data(),
								message_composed.size());
						command.reset(1);
						message_composed.clear();
						break;
					}
					case SET_CODING_RATE: {
						uint8_t cr = command.getDataAsUint8();
						txlora.set_coding_rate(cr);
						txlora.save_settings();
						txlora.configure_modem();
						rxlora.set_coding_rate(cr);
						rxlora.save_settings();
						rxlora.configure_modem();
						uint8_t coding_array[1];
						rxlora.read_settings();
						uint8_t coding_rate = txlora.get_coding_rate();
						memcpy(coding_array, &coding_rate, sizeof(coding_rate));
						command.set_message(coding_array, sizeof(coding_array));
						std::vector<uint8_t> message_composed =
								command.get_composed_message();
						command.composeMessage(&message_composed);
						message_composed = command.get_composed_message();
						uart_cfg.transmitMessage(message_composed.data(),
								message_composed.size());
						command.reset(1);
						message_composed.clear();
						break;
					}
					case SET_UART_BAUDRATE:{
						txlora.set_default_parameters();
						rxlora.set_default_parameters();
					}
					default:
						break;
					}

				}
				memset(data_reciv, 0, bytes_reciv);
				bytes_reciv = 0;
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
	HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief I2C3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C3_Init(void)
{

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
	if (HAL_I2C_Init(&hi2c3) != HAL_OK)
	{
		Error_Handler();
	}

	/** Configure Analogue filter
	 */
	if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
	{
		Error_Handler();
	}

	/** Configure Digital filter
	 */
	if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
	{
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
	huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
	huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart1) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
	{
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
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
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

	/*Configure GPIO pin : SWITCH_SHIP_MODE_Pin */
	GPIO_InitStruct.Pin = SWITCH_SHIP_MODE_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(SWITCH_SHIP_MODE_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : LORA_TX_DIO0_Pin LORA_TX_LED_Pin LORA_TX_NRST_Pin LORA_RX_NSS_Pin */
	GPIO_InitStruct.Pin = LORA_TX_DIO0_Pin | LORA_TX_LED_Pin | LORA_TX_NRST_Pin | LORA_RX_NSS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : LORA_RX_DIO0_IRQ_Pin */
	GPIO_InitStruct.Pin = LORA_RX_DIO0_IRQ_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(LORA_RX_DIO0_IRQ_GPIO_Port, &GPIO_InitStruct);

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


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	bytes_reciv = uart_cfg.read_timeout_new(data_reciv);

}


//void USART1_IRQHandler(void) {
//	bytes_reciv = uart_cfg.read_timeout(data_reciv, 1);
//}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
int keepAliveStartTicks = 0;
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();

	while (1)
	{
		if (keepAliveStartTicks > 50000)
		{
			keepAliveStartTicks = 0;
			HAL_GPIO_WritePin(KA_GPIO_Port, KA_Pin, GPIO_PIN_SET);
		}
		else if (keepAliveStartTicks > 25000)
			HAL_GPIO_WritePin(KA_GPIO_Port, KA_Pin, GPIO_PIN_RESET);
		keepAliveStartTicks++;
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
