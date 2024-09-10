/*
 * sniffer_tag.h
 *
 *  Created on: May 2, 2024
 *      Author: uqommdev
 */

#ifndef INC_SNIFFER_TAG_HPP_
#define INC_SNIFFER_TAG_HPP_

#ifdef __cplusplus // Check if compiling with a C++ compiler
extern "C" {
#endif

#include "main.h"
#include "uwb3000Fxx.h"
#include "SX1278.h"
#include "eeprom.h"
#include "rdss.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

extern UART_HandleTypeDef huart1;
extern Uwb_HW_t uwb_hw_a;
extern Uwb_HW_t uwb_hw_b;
extern Uwb_HW_t *hw;

/* Length of the common part of the message (up to and including the function code, see NOTE 2 below). */
#define INITIAL_COMUNICATION_DATA_SIZE 5
// Define the size of the serialized TAG_t structure
#define SERIALIZED_TAG_SIZE (sizeof(uint32_t) + 4 * sizeof(uint16_t))
#define POLL_RX_OFFSET 5
#define RESP_TX_OFFSET (POLL_RX_OFFSET+sizeof(uint32_t))
#define BATTERY_VOLTAGE_RAW_OFFSET (RESP_TX_OFFSET+sizeof(uint32_t))
#define BATTERY_VOLTAGE_CALIBRATED_OFFSET (BATTERY_VOLTAGE_RAW_OFFSET+sizeof(uint8_t))
#define TEMPERATURE_RAW_OFFSET (BATTERY_VOLTAGE_CALIBRATED_OFFSET+sizeof(uint8_t))
#define TEMPERATURE_CALIBRATED_OFFSET (TEMPERATURE_RAW_OFFSET+sizeof(uint8_t))

typedef struct {
	double readings[DISTANCE_READINGS/2];
	uint16_t value;
	uint8_t error_times;
	uint8_t counter;
} Distance_t;
//
//typedef struct {
//	uint8_t calibrated;
//	uint8_t raw;
//	uint16_t real;
//} Mesurement_data_t;

typedef struct {
	uint32_t id;
	int readings;
	uint8_t command;
	Distance_t *distance;
	Distance_t distance_a;
	Distance_t distance_b;
	uint16_t Battery_Voltage;
	float Float_Battery_Voltage;
	uint8_t Estado_Final;
//	Mesurement_data_t temperature;
//	Mesurement_data_t battery_voltage;
} TAG_t;

typedef struct tag_node {
	TAG_t tag;
	struct tag_node *next;
    int count;
} TAG_Node;

typedef struct {
    TAG_Node *head;
    uint8_t count;
} TAG_List;

typedef struct buffer {
	uint8_t *buffer;
	uint8_t buffer_size;
	int delay;
	int rx_timeout;
	int preamble_timeout;
	uint32_t resp_tx_timestamp;
	uint32_t poll_rx_timestamp;
} TX_BUFFER_t;

typedef enum {
	TAG_NO_RESPONSE,
	TAG_NO_RXCG_DETECTED,
	TAG_RX_FRAME_TIMEOUT,
	TAG_RX_PREAMBLE_DETECTION_TIMEOUT,
	TAG_RX_CRC_VALID,
	TAG_RX_ERROR,
	TAG_RX_DATA_ZERO,
	TAG_RX_NO_COMMAND,
	TAG_TX_ERROR,
	TAG_HUMAN_DISTANCE_OK,
	TAG_END_READINGS,
	TAG_DISCOVERY,
	TAG_SEND_TIMESTAMP_QUERY,
	TAG_SEND_SET_SLEEP,
	TAG_UNKNOWN,
} TAG_STATUS_t;

#define TX_BUFFER_SIZE (sizeof(uint8_t) + sizeof(uint32_t))
#define TX_DISCOVERY_SIZE (sizeof(uint8_t))
#define TX_TIMESTAMP_SIZE (sizeof(uint8_t) + sizeof(uint32_t))
#define TAG_TIMESTAMP_QUERY 0x11
#define TAG_SET_SLEEP_MODE 0x12
#define TAG_ID_QUERY 0x13

#define TIMESTAMP_QUERY_TIMEOUT_MS 2000 // Timeout for TIMESTAMP_QUERY state in milliseconds

void init_uwb_device(Uwb_HW_t * hwb_hw,SPI_HandleTypeDef *hspi, GPIO_TypeDef *nssPort,
		uint16_t nssPin, GPIO_TypeDef *nrstPort, uint16_t nrstPin);
TAG_t* create_TAG();
void reset_TAG_values(TAG_t *tag);
TAG_STATUS_t tag_discovery(TAG_t *tag);
TAG_STATUS_t tag_send_timestamp_query(TAG_t *tag);
TAG_STATUS_t process_queried_tag_information(TAG_t *tag);
double calculate_distance_human_tag(uint8_t *rx_buffer, Distance_t *distance);
TAG_STATUS_t send_message_with_timestamps();
uint32_t send_response_with_timestamps(uint8_t *tx_resp_msg, uint8_t size,
		uint32_t frame_seq_nb);
uint32_t allocate_and_read_received_frame(uint8_t **rx_buffer);
double distance_moving_average(Distance_t *distance);
int start_transmission_inmediate_with_response_expected(TX_BUFFER_t tx);
TAG_STATUS_t wait_rx_data();
void debug_status(TAG_STATUS_t status);
void debug(TAG_t tag, TAG_STATUS_t status);
void debug_tag(TAG_t tag);
double distance_smooth(Distance_t *distance);
//void set_battery_voltage(Mesurement_data_t *battery_voltage);
//void set_temperature(Mesurement_data_t *temperature);
void int_to_float_Tag_Battery_Voltage(TAG_t *tag);
// Function to insert a new TAG_t node into the linked list
void insert_tag(TAG_List *list, TAG_t new_tag);
void delete_tag(TAG_List *list, uint32_t id);
void free_tag_list(TAG_List *list);
void print_all_tags(TAG_List *list, TAG_STATUS_t status);
void print_serialized_tags(TAG_List *list);
void serialize_tag_list(TAG_List *list, uint8_t *buffer);
void media_tag_distance(Distance_t *distance);

void process_discovery(TAG_t *tag, TAG_STATUS_t *tag_status, uint32_t *query_ticks);
void process_timestamp_query(TAG_t *tag, TAG_STATUS_t *tag_status);

void print_tx_hex(uint8_t *tx, uint16_t length);
#ifdef __cplusplus
} // End of extern "C" block
#endif
#endif /* INC_SNIFFER_TAG_HPP_ */

