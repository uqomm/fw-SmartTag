/*
 * rdss.h
 *
 *  Created on: Sep 28, 2022
 *      Author: sigmadev
 */

#ifndef INC_RDSS_H_
#define INC_RDSS_H_

#include "main.h"
#include "stdbool.h"
#include "uart1.h"
#include "stdio.h"
#include "string.h"

#define LTEL_FRAME_SIZE 14
#define SIGMA_FRAME_SIZE 14
#define LTEL_START_MARK 0x7e
#define RDSS_END_MARK 0x7f
#define RDSS_START_MARK 0x7e
#define LTEL_END_MARK  0x7f
#define RDSS_BUFFER_SIZE 25
#define CRC_SIZE 2
#define LTEL_SET_LENGTH  13
#define LTEL_QUERY_LENGTH  9
#define MINIMUM_FRAME_LEN 6
#define ATTENUATION_VALUE_INDEX 5
#define QUERY_STATUS_BUFFER_SIZE 34
#define MINIMUM_PACKET_BYTES  8

#define START_BYTES_SIZE 1
#define END_BYTES_SIZE 1
#define DATA_LENGTH_BYTES_SIZE 2
#define DEVICE_BYTES_SIZE 1
#define FUNCTION_BYTES_SIZE 1
#define ID_BYTES_SIZE 1  // Assuming 6 bytes for the ID
#define CRC_BYTES_SIZE 2

typedef enum MODULE_FUNCTION {
	SERVER,
	QUAD_BAND,
	PSU,
	TETRA,
	ULADR,
	VLADR,
	BDA,
	LOW_NOISE_AMPLIFIER,
	POWER_AMPLIFIER,
	UHF_TONE,
	SNIFFER
} Function_t;

typedef enum RS485_CMD {
	NONE,
	QUERY_MODULE_ID = 0x10,
	QUERY_STATUS,
	SET_VLAD_ATTENUATION,
	QUERY_MASTER_STATUS,
	QUERY_UART1,
	SET_UART1,

	QUERY_TX_FREQ = 0x20,
	QUERY_RX_FREQ,
	QUERY_UART_BAUDRATE,
	QUERY_BANDWIDTH,
	QUERY_SPREAD_FACTOR,
	QUERY_CODING_RATE,

	SET_MODULE_ID = 0x90,
	SET_TX_FREQ = 0xB0,
	SET_RX_FREQ,
	SET_UART_BAUDRATE,
	SET_BANDWIDTH,
	SET_SPREAD_FACTOR,
	SET_CODING_RATE,
	SET_OUT,
	SET_AOUT_0_10V,
	SET_AOUT_4_20mA,
	SET_AOUT_0_20mA,
	SET_DOUT1,

	SET_VLAD_MODE,
	SET_PARAMETER_FREQOUT = 0x31,
	SET_PARAMETERS,
	SET_PARAMETER_FREQBASE,
	QUERY_PARAMETER_PdBm,
} Rdss_cmd_t;

typedef enum RS485_STATUS {
	DATA_OK,
	START_READING,
	VALID_FRAME,
	NOT_VALID_FRAME,
	WRONG_MODULE_FUNCTION,
	WRONG_MODULE_ID,
	WRONG_CMD_NUMBER,
	CRC_ERROR,
	DONE,
	WAITING,
	VALID_MODULE,
	CHECK_LORA_DATA,
	LORA_RECEIVE,
	LORA_SEND,
	UART_SEND,
	UART_VALID
} RDSS_status_t;

typedef enum RDSS_LORA_PROTOCOL_INDEX {
	LORA_START_INDEX,
	LORA_MODULE_TYPE_INDEX,
	LORA_MODULE_ID_INDEX,
	LORA_CMD_INDEX,
	LORA_DATA_LENGHT_INDEX_1,
	LORA_DATA_LENGHT_INDEX_2,
	LORA_DATA_START_INDEX
} RDSS_LORA_PROTOCOL_INDEX_t;

typedef enum RDSS_UART_PROTOCOL_INDEX {
	UART_START_INDEX,
	UART_MODULE_TYPE_INDEX,
	UART_MODULE_ID_INDEX,
	UART_CMD_INDEX,
	UART_DATA_LENGHT1_INDEX,
	UART_DATA_LENGHT2_INDEX,
	UART_DATA_START_INDEX
} RDSS_UART_PROTOCOL_INDEX_t;

typedef struct RS485 {
	Rdss_cmd_t cmd;
	uint8_t len;
	uint8_t *buff;
	uint8_t buffSize;
	uint16_t crcCalculated;
	uint16_t crcReceived;
	uint8_t idQuery;
	uint8_t idReceived;
	uint8_t id;
	RDSS_status_t status;
	uint8_t queryBuffer[QUERY_STATUS_BUFFER_SIZE];
	uint32_t lastUpdateTicks;
} RDSS_t;

typedef enum MODULE_ID {
	ID0 = 0x00, ID1 = 0x01, ID2 = 0x02, ID4 = 0x04, ID8 = 0x08, ID9 = 0X09
} Id_t;

union floatConverter {
	uint32_t i;
	float f;
};

uint16_t crc_get(uint8_t *buffer, uint8_t buff_len);
RDSS_status_t rdssCheckFrame(RDSS_t *r, UART1_t *u);
RDSS_status_t rs485_check_valid_module(UART1_t *uart1);
RDSS_status_t rs485_check_CRC_module(UART1_t *uart1);
RDSS_status_t checkFrameValidity(uint8_t *frame, uint8_t lenght);
RDSS_status_t checkModuleValidity(uint8_t *frame, uint8_t lenght);
RDSS_status_t checkCRCValidity(uint8_t *frame, uint8_t len);
RDSS_status_t isValidId(RDSS_t *r);
RDSS_status_t validate(uint8_t *buffer,uint8_t length);
RDSS_status_t checkBuffer(RDSS_t *rs485);
RDSS_status_t rdss_validation(uint8_t *data_received, uint8_t len,
		uint8_t id);
void fillValidBuffer(RDSS_t *r, uint8_t *buff, uint8_t len);
RDSS_t* rdssInit(uint8_t id);
RDSS_t* rdss();
void reinit(RDSS_t *rs485);
void encodeVlad(uint8_t* buff);
uint8_t setCrc(uint8_t* buff,uint8_t size);
uint8_t setRdssStartData(RDSS_t *rdss, uint8_t *buffer);
uint8_t calculate_frame_length(uint8_t structure_size);
int freqDecode(uint8_t *buffer);
void freqEncode(uint8_t *buffer, uint32_t freqIn);
RDSS_status_t evaluateRdssStatus(RDSS_t *rdss);
#endif /* INC_RDSS_H_ */

