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
#define RESPONSE_TX_TIME_MASK_VALUE 0xFFFFFFFEUL
#define RESPONSE_TX_TIME_SHIFT_AMOUNT 8

#define TX_BUFFER_SIZE (sizeof(uint8_t) + 3 * sizeof(uint32_t) + sizeof(uint16_t)) //Modificar para tamaño de batería de 16bits unsigned
#define RX_BUFFER_SIZE_TIMESTAMP_QUERY  (sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t))
#define TAG_TIMESTAMP_QUERY 0x11
#define TAG_SET_SLEEP_MODE 0x12
#define TAG_ID_QUERY 0x13
#define WAIT_FOR_TIMESTAMP_QUERY_TIMEOUT_MS 2000 // Timeout for transitioning from WAIT_FOR_TIMESTAMP_QUERY to WAIT_FOR_FIRST_DETECTION state in milliseconds
#define LSI_CLOCK 32000
#define CLK_DIVIDER 128

extern char *TAG_MESSAGES[];
extern uint8_t Discovery_Counter;

typedef struct {
	uint32_t id;
	uint64_t readings;
	uint8_t command;
	uint16_t Voltaje_Bat;
	uint64_t resp_tx_timestamp;
	uint64_t poll_rx_timestamp;
	uint32_t resp_tx_time;
	uint8_t sniffer_state;
	uint8_t sleep_time;
	uint16_t distance_a;
	uint16_t distance_b;
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


typedef enum {
	MASTER_ONE_DETECTION,
	MASTER_MULTIPLE_DETECTION
}Sniffer_State;



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
	TAG_WAIT_SEND_TX,
	TAG_SLEEP_RECIVED,
	TAG_UNKNOWN
}TAG_STATUS_t;


TAG_STATUS_t tag_discovery(TAG_t *tag);
TAG_STATUS_t process_queried_tag_information(TAG_t *tag);
TAG_STATUS_t process_first_tag_information(TAG_t *tag);
TAG_STATUS_t process_second(TAG_t *tag);
uint32_t send_response_with_timestamps(uint8_t *tx_resp_msg, uint8_t size,uint32_t frame_seq_nb);
void start_tag_reception_inmediate(uint8_t preamble_timeout,uint8_t rx_timeout);
TAG_STATUS_t wait_rx_data();
TAG_STATUS_t process_response(TAG_t *tag);

#ifdef __cplusplus
}
#endif

#endif /* INC_HUMAN_TAG_H_ */
