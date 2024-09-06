/*
 * sniffer_tag.h
 *
 *  Created on: May 2, 2024
 *      Author: uqommdev
 */

#ifndef INC_HUMAN_TAG_H_
#define INC_HUMAN_TAG_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "main.h"
#include "stdlib.h"
#include "stdio.h"
#include "uwb3000Fxx.h"

//extern UART_HandleTypeDef huart1;

/* Length of the common part of the message (up to and including the function code, see NOTE 2 below). */
#define INITIAL_COMUNICATION_DATA_SIZE 5
#define RESPONSE_TX_TIME_MASK_VALUE 0xFFFFFFFEUL
#define RESPONSE_TX_TIME_SHIFT_AMOUNT 8

extern char *TAG_MESSAGES[];

typedef struct {
	uint32_t id;
	uint64_t readings;
	uint8_t command;
	uint16_t Voltaje_Bat;
//	uint16_t raw_battery_voltage; // IC V bat read during production and stored in OTP (Vmeas @ 3V3)
//	uint8_t raw_temperature; // IC temp read during production and stored in OTP (Tmeas @ 23C)
//	uint8_t calibrated_battery_voltage; // IC V bat read during production and stored in OTP (Vmeas @ 3V3)
//	uint8_t calibrateds_temperature; // IC temp read during production and stored in OTP (Tmeas @ 23C)
	uint32_t resp_tx_timestamp;
	uint32_t poll_rx_timestamp;
	uint32_t resp_tx_time;
} TAG_t;

typedef struct buffer {
    uint8_t *buffer;
    uint8_t buffer_size;
    int delay;
    int rx_timeout;
    int preamble_timeout;
	uint64_t resp_tx_timestamp;
	uint64_t poll_rx_timestamp;
} TX_BUFFER_t;

typedef enum{
	TAG_NO_RESPONSE,
	TAG_NO_RXCG_DETECTED,
	TAG_RX_TIMEOUT,
	TAG_RX_CRC_VALID,
	TAG_RX_ERROR,
	TAG_RX_DATA_ZERO,
	TAG_RX_COMMAND_ERROR,
	TAG_TX_ERROR,
	TAG_SLEEP,
	TAG_WAKE_UP,
	TAG_WAIT_FOR_FIRST_DETECTION,
	TAG_WAIT_FOR_TIMESTAMPT_QUERY,
	TAG_TX_SUCCESS,
	TAG_WRONG_ID_MATCH,
	TAG_UNKNOWN

}TAG_STATUS_t;
#define TX_BUFFER_SIZE (sizeof(uint8_t) + 3 * sizeof(uint32_t) + sizeof(uint16_t)) //Modificar para tamaño de batería de 16bits unsigned
#define TAG_TIMESTAMP_QUERY 0x11
#define TAG_SET_SLEEP_MODE 0x12
#define TAG_ID_QUERY 0x13
#define WAIT_FOR_TIMESTAMP_QUERY_TIMEOUT_MS 2000 // Timeout for transitioning from WAIT_FOR_TIMESTAMP_QUERY to WAIT_FOR_FIRST_DETECTION state in milliseconds

TAG_STATUS_t tag_discovery(TAG_t *tag);
TAG_STATUS_t process_queried_tag_information(TAG_t *tag);
TAG_STATUS_t process_first_tag_information(TAG_t *tag);
TAG_STATUS_t send_message_with_timestamps();
uint32_t send_response_with_timestamps(uint8_t *tx_resp_msg, uint8_t size,uint32_t frame_seq_nb);
uint8_t read_human_tag_first_message(uint8_t *rx_buffer);
//void uart_transmit_hexa_to_text(uint8_t *message,uint8_t size);
//void uart_transmit_float_to_text(float distanceValue);
//void uart_transmit_int_to_text(int distanceValue);
//int uart_transmit_string(char *message);
void start_tag_reception_inmediate(uint8_t preamble_timeout,uint8_t rx_timeout);
TAG_STATUS_t wait_rx_data();
uint32_t allocate_and_read_received_frame(uint8_t **rx_buffer);
uint32_t create_message_and_alloc_buffer(TX_BUFFER_t *tx,TAG_t *tag);
int start_transmission_delayed_with_response_expected(TX_BUFFER_t tx);
//void debug(TAG_t *tag,TAG_STATUS_t status);
TAG_STATUS_t process_response(TAG_t *tag);

#ifdef __cplusplus
}
#endif

#endif /* INC_HUMAN_TAG_H_ */
