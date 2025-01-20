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

#define RDSS_FRAME_SIZE 14
#define SIGMA_FRAME_SIZE 14
#define RDSS_START_MARK 0x7e
#define RDSS_END_MARK  0x7f
#define RDSS_BUFFER_SIZE 50
#define LTEL_SET_LENGTH  13
#define LTEL_QUERY_LENGTH  9
#define MINIMUN_FRAME_LEN 6
#define CRC_HIGH_BYTE_OFFSET 2
#define CRC_LOW_BYTE_OFFSET 3
#define FRAME_HEADER_SIZE 4

// Bytes number
#define CRC_BYTES 2

#define QUERY_MASTER_STATUS_BYTES 16
#define QUERY_MODULE_ID  0x10
#define QUERY_STATUS 0x11
#define SET_VLAD_ATTENUATION 0x13
#define QUERY_MASTER_STATUS 0x14
#define QUERY_UART1 0x15
//#define SET_UART1 0x16

#define NONE                        0x00  // No command

// Query commands
#define QUERY_TX_FREQ               0x20  // Query transmit frequency
#define QUERY_RX_FREQ               0x21  // Query receive frequency
#define QUERY_UART_BAUDRATE         0x22  // Query UART baud rate
#define QUERY_BANDWIDTH              0x23  // Query bandwidth
#define QUERY_SPREAD_FACTOR          0x24  // Query spread factor
#define QUERY_CODING_RATE            0x25  // Query coding rate
#define QUERY_PARAMETER_PdBm        0x26  // Query parameter PdBm

// Set commands (base value 0x90)
#define SET_MODULE_ID               (0x90) // Set module ID
#define SET_TX_FREQ                  (0xB0) // Set transmit frequency
#define SET_RX_FREQ                  (0xB1) // Set receive frequency
#define SET_UART_BAUDRATE            (0xB2) // Set UART baud rate
#define SET_BANDWIDTH               (0xB3) // Set bandwidth
#define SET_SPREAD_FACTOR           (0xB4) // Set spread factor
#define SET_CODING_RATE             (0xB5) // Set coding rate
#define SET_OUT                     (0xB6) // Set output control
#define SET_AOUT_0_10V               (0xB7) // Set analog output 0-10V
#define SET_AOUT_4_20mA              (0xB8) // Set analog output 4-20mA
#define SET_AOUT_0_20mA              (0xB9) // Set analog output 0-20mA
#define SET_DOUT1                   (0xBA) // Set digital output 1

#define SET_VLAD_MODE                0xC0  // Set VLAD mode
#define SET_PARAMETER_FREQOUT       0x31  // Set parameter frequency output
#define SET_PARAMETERS               0xC2  // Set multiple parameters
#define SET_PARAMETER_FREQBASE      0xC3  // Set parameter frequency base

// ADC definitions
#define ADC_CHANNELS_NUM 2
#define ADC_EXTRA_DATA 1

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
	START,
	MODULE_TYPE,
	MODULE_ID,
	CMD,
	DATA_LENGHT1,
	DATA_LENGHT2,
	DATA_START
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
	SNIFFER
};


class CommandMessage {
public:
    CommandMessage(uint8_t _module_function, uint8_t _module_id, uint8_t max_size);
    CommandMessage(uint8_t _module_function, uint8_t _module_id); // Constructor que delega
    CommandMessage(uint8_t max_size);
    CommandMessage();
    virtual ~CommandMessage();


    STATUS validate(uint8_t *buffer, uint8_t length);

    uint16_t crc_get(uint8_t *buffer, uint8_t buff_len);


    size_t getDataSize() const;
    uint8_t getDataAsUint8() const;
    uint16_t getDataAsUint16() const;
    uint32_t getDataAsUint32() const;
    float getDataAsFloat() const;
    int freqDecode() const;


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

    bool isQueryParameterLTEL() const {
        return (command_id == QUERY_PARAMETER_LTEL);
    }

    bool isQueryParameterSigma() const {
        return (command_id == QUERY_PARAMETER_SIGMA);
    }

    bool isQueryParameterStr() const {
        return (command_id == QUERY_PARAMETER_STR);
    }

    bool isQueryADC() const {
        return (command_id == QUERY_PARAMETER_ADC);
    }

    bool isSetAttLTEL() const {
        return (command_id == SET_ATT_LTEL);
    }

    bool isSetPoutMax() const {
        return (command_id == SET_POUT_MAX);
    }

    bool isSetPoutMin() const {
        return (command_id == SET_POUT_MIN);
    }

	void checkByte(uint8_t number);
	std::vector<uint8_t> getData();
	bool composeMessage(std::vector<uint8_t> *data);
	bool composeMessage();
	void reset(bool init);
	void reset();
	//METODO PARA DESGLOSAR FRAME QUE LLEGA DESDE RS485
	void saveFrame(uint8_t *buffer, uint8_t length);


protected:
    void setVars();
    bool prepareTxData(const char *message);
    void handleRxData(uint8_t data);
    bool validateChecksum();
    bool checkCRC();
    uint16_t calculateCRC(uint8_t start, uint8_t end);


    //METODO PARA DESGLOSAR FRAME QUE LLEGA DESDE RS485
    void storeData(size_t dataLength, const void* dataPtr);


    //METODOS SACADOS DE rdss_protocol
    STATUS checkFrameValidity(uint8_t *frame, uint8_t length);
    STATUS check_module(uint8_t *frame, uint8_t length);
    STATUS checkCRCValidity(uint8_t *frame, uint8_t len);
    //uint16_t crc_get(uint8_t *buffer, uint8_t buff_len);

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

    const uint8_t LTEL_START_MARK = 0x7e;
    const uint8_t LTEL_END_MARK = 0x7f;
    const uint8_t MIN_FRAME_HEADER_SIZE = 9;

    const uint8_t QUERY_PARAMETER_LTEL = 0x11;
    const uint8_t QUERY_PARAMETER_SIGMA = 0x12;
    const uint8_t QUERY_PARAMETER_STR = 0x15;
    const uint8_t QUERY_PARAMETER_ADC = 0x16;

    const uint8_t SET_ATT_LTEL = 0x20;
    const uint8_t SET_POUT_MAX = 0x24;
    const uint8_t SET_POUT_MIN = 0x23;

    std::vector<uint8_t> message;
    bool listening;
    bool ready;
};

#endif /* INC_LORA_HPP_ */
