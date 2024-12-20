/*
 * UartHandler.hpp
 *
 *  Created on: Jun 18, 2024
 *      Author: artur
 */

#ifndef INC_COMMANDMESSAGE_HPP_
#define INC_COMMANDMESSAGE_HPP_

#include "main.h"
#include <cstring>
#include <vector>
// Abstract class Function

#include <cstdint> // For uint8_t

// Frame constants
constexpr uint8_t RDSS_FRAME_SIZE = 14;
constexpr uint8_t SIGMA_FRAME_SIZE = 14;
constexpr uint8_t RDSS_START_MARK = 0x7E;
constexpr uint8_t RDSS_END_MARK = 0x7F;
constexpr uint8_t RDSS_BUFFER_SIZE = 50;
constexpr uint8_t LTEL_SET_LENGTH = 13;
constexpr uint8_t LTEL_QUERY_LENGTH = 9;
constexpr uint8_t MINIMUN_FRAME_LEN = 6;
constexpr uint8_t CRC_HIGH_BYTE_OFFSET = 2;
constexpr uint8_t CRC_LOW_BYTE_OFFSET = 3;
constexpr uint8_t FRAME_HEADER_SIZE = 4;

// Bytes number
constexpr uint8_t CRC_BYTES = 2;

// Query and Set commands
//constexpr uint8_t QUERY_MASTER_STATUS_BYTES = 16;
//constexpr uint8_t QUERY_MODULE_ID = 0x10;
//constexpr uint8_t QUERY_STATUS = 0x11;
//constexpr uint8_t SET_VLAD_ATTENUATION = 0x13;
//constexpr uint8_t QUERY_MASTER_STATUS = 0x14;
//constexpr uint8_t QUERY_UART1 = 0x15;
////constexpr uint8_t SET_UART1 = 0x16;
//
//constexpr uint8_t NONE = 0x00;  // No command
//
//// Query commands
//constexpr uint8_t QUERY_TX_FREQ = 0x20;        // Query transmit frequency
//constexpr uint8_t QUERY_RX_FREQ = 0x21;        // Query receive frequency
//constexpr uint8_t QUERY_UART_BAUDRATE = 0x22;  // Query UART baud rate
//constexpr uint8_t QUERY_BANDWIDTH = 0x23;       // Query bandwidth
//constexpr uint8_t QUERY_SPREAD_FACTOR = 0x24;   // Query spread factor
//constexpr uint8_t QUERY_CODING_RATE = 0x25;     // Query coding rate
//constexpr uint8_t QUERY_PARAMETER_PdBm = 0x26; // Query parameter PdBm
//
//// Set commands (base value 0x90)
//constexpr uint8_t SET_MODULE_ID = 0x90;       // Set module ID
//constexpr uint8_t SET_TX_FREQ = 0xB0;          // Set transmit frequency
//constexpr uint8_t SET_RX_FREQ = 0xB1;          // Set receive frequency
//constexpr uint8_t SET_UART_BAUDRATE = 0xB2;    // Set UART baud rate
//constexpr uint8_t SET_BANDWIDTH = 0xB3;       // Set bandwidth
//constexpr uint8_t SET_SPREAD_FACTOR = 0xB4;   // Set spread factor
//constexpr uint8_t SET_CODING_RATE = 0xB5;     // Set coding rate
//constexpr uint8_t SET_OUT = 0xB6;             // Set output control
//constexpr uint8_t SET_AOUT_0_10V = 0xB7;       // Set analog output 0-10V
//constexpr uint8_t SET_AOUT_4_20mA = 0xB8;      // Set analog output 4-20mA
//constexpr uint8_t SET_AOUT_0_20mA = 0xB9;      // Set analog output 0-20mA
//constexpr uint8_t SET_DOUT1 = 0xBA;           // Set digital output 1
//
//constexpr uint8_t SET_VLAD_MODE = 0xC0;           // Set VLAD mode
//constexpr uint8_t SET_PARAMETER_FREQOUT = 0x31;  // Set parameter frequency output
//constexpr uint8_t SET_PARAMETERS = 0xC2;        // Set multiple parameters
//constexpr uint8_t SET_PARAMETER_FREQBASE = 0xC3; // Set parameter frequency base

// LTEL Protocol constants
constexpr uint8_t LTEL_START_MARK = 0x7E;
constexpr uint8_t LTEL_END_MARK = 0x7F;
constexpr uint8_t MIN_FRAME_HEADER_SIZE = 9;

constexpr uint8_t QUERY_PARAMETER_LTEL = 0x11;
constexpr uint8_t QUERY_PARAMETER_SIGMA = 0x12;
constexpr uint8_t QUERY_PARAMETER_STR = 0x15;
constexpr uint8_t QUERY_PARAMETER_ADC = 0x16;
constexpr uint8_t ONE_DETECTION = 0x17;
constexpr uint8_t MULTIPLE_DETECTION = 0x18;
constexpr uint8_t CHANGE_SLEEP_TIME_RECIVED = 0x19;
constexpr uint8_t CHANGE_SLEEP_NOT_RECIVED = 0x20;
constexpr uint8_t CHANGE_TAG_READINGS = 0x21;

constexpr uint8_t SET_ATT_LTEL = 0x20;
constexpr uint8_t SET_POUT_MAX = 0x24;
constexpr uint8_t SET_POUT_MIN = 0x23;


enum class STATUS {
	RDSS_DATA_OK,
	START_READING,
	VALID_FRAME,
	NOT_VALID_FRAME,
	WRONG_MODULE_FUNCTION,
	WRONG_MODULE_ID,
	CRC_ERROR,
	DONE,
	WAITING,
	VALID_MODULE,
	CHECK_LORA_DATA,
	LORA_RECEIVE,
	LORA_SEND,
	UART_SEND,
	UART_VALID,
	CONFIG_FRAME,
	RETRANSMIT_FRAME
};

enum class INDEX {
	START, MODULE_TYPE, MODULE_ID, CMD, DATA_LENGHT1, DATA_LENGHT2, DATA_START
};

enum class MODULE_FUNCTION {
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
	SNIFFER = 0x10

};

class CommandMessage {
public:
	CommandMessage(uint8_t _module_function, uint8_t _module_id,
			uint8_t max_size);
	CommandMessage(uint8_t _module_function, uint8_t _module_id); // Constructor que delega
	CommandMessage(uint8_t max_size);
	CommandMessage();
	virtual ~CommandMessage();

	STATUS validate(uint8_t *buffer, uint8_t length);
	STATUS validate_crc_ptrotocol(uint8_t *buffer, uint8_t length);
	void save_frame(uint8_t *buffer, uint8_t length);
	uint16_t crc_get(uint8_t *buffer, uint8_t buff_len);

	size_t getDataSize() const;
	uint8_t getDataAsUint8() const;
	uint16_t getDataAsUint16() const;
	uint32_t getDataAsUint32() const;
	float getDataAsFloat() const;

	void set_message(uint8_t *arr, uint8_t size);
	// Getters y Setters
	uint8_t getModuleFunction() const {
		return module_function;
	}
	void setModuleFunction(uint8_t _module_function) {
		module_function = _module_function;
	}

	uint8_t getModuleId() const {
		return module_id;
	}
	void setModuleId(uint8_t _module_id) {
		module_id = _module_id;
	}

	uint8_t getCommandId() const {
		return command_id;
	}
	void setCommandId(uint8_t _command_id) {
		command_id = _command_id;
	}

	bool isListening() const {
		return listening;
	}

	bool isReady() const {
		return ready;
	}

	void setMaxSize(uint8_t max_size) {
		max_message_size = max_size;
	}

	uint8_t getMaxSize() const {
		return max_message_size;
	}

	std::vector<uint8_t> get_composed_message() {
		return message;
	}

	uint8_t getLTELStartMark() const {
		return LTEL_START_MARK;
	}
	uint8_t getLTELEndMark() const {
		return LTEL_END_MARK;
	}
	uint8_t getMinFrameHeaderSize() const {
		return MIN_FRAME_HEADER_SIZE;
	}

	void check_byte(uint8_t number);
	std::vector<uint8_t> getData();
	bool composeMessage(std::vector<uint8_t> *data);
	bool composeMessage();
	void reset(bool init);
	void reset();

protected:
	void setVars();
	bool prepareTxData(const char *message);
	void handleRxData(uint8_t data);
	bool validateChecksum();
	bool check_crc();
	uint16_t crc_calc(uint8_t start, uint8_t end);

	void storeData(size_t dataLength, const void *dataPtr);

	//METODOS SACADOS DE rdss_protocol
	STATUS validate_protocol(uint8_t *frame, uint8_t length);
	STATUS validate_module(uint8_t *frame, uint8_t length);
	STATUS validate_crc(uint8_t *frame, uint8_t len);

	uint8_t max_message_size;
	uint8_t module_function;
	uint8_t module_id;
	uint8_t command_id;
	uint8_t num_byte_data;
	uint8_t data_frame;
	size_t dataSize;

	uint8_t cmd;
	uint8_t *buff;
	uint8_t buffSize;
	uint16_t crcCalculated;
	uint16_t crcReceived;
	uint8_t idQuery;
	uint8_t idReceived;
	uint8_t id;
	STATUS status;
	STATUS lastStatus;
	uint8_t queryBuffer[30];
	uint32_t lastUpdateTicks;

	std::vector<uint8_t> message;
	bool listening;
	bool ready;
};

#endif /* INC_LORA_HPP_ */
