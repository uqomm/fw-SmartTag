/*
 * Function.cpp
 *
 *  Created on: Jun
 *   19, 2024
 *      Author: artur
 */

#include <CommandMessage.hpp> // Include the header file

CommandMessage::CommandMessage(uint8_t _module_function, uint8_t _module_id,
		uint8_t max_size) :
		module_function(_module_function), module_id(_module_id), max_message_size(
				max_size) {
	reset(true);
}

CommandMessage::CommandMessage(uint8_t _module_function, uint8_t _module_id) :
		CommandMessage(_module_function, _module_id, 255) {
	//setVars();
}

CommandMessage::CommandMessage(uint8_t max_size) :
		max_message_size(max_size) {
	reset();
}

CommandMessage::CommandMessage() :
		CommandMessage(255) {
}

CommandMessage::~CommandMessage() {
}

void CommandMessage::setVars() {
#define MODULE_FUNCTION_BYTE 1
#define MODULE_ID_BYTE 2
#define COMMAND_BYTE 3

	if (!ready)
		return;

	command_id = message[COMMAND_BYTE];
	module_id = message[MODULE_ID_BYTE];
	module_function = message[MODULE_FUNCTION_BYTE];
}

std::vector<uint8_t> CommandMessage::getData() {
#define DATA_LENGTH_INDEX 5
#define DATA_INDEX 6

	if (!ready)
		return std::vector<uint8_t>();

	uint8_t length = message[5];
	uint8_t end_index = DATA_INDEX + length;
	return std::vector<uint8_t>(message.begin() + DATA_INDEX,
			message.begin() + end_index);
}

void CommandMessage::reset(bool init) {
	if (!init) {
		module_function = 0;
		module_id = 0;
	}

	command_id = 0;
	ready = false;
	listening = false;
	message.clear();
}

void CommandMessage::reset() {
	reset(false);
}

void CommandMessage::check_byte(uint8_t number) {
	if (listening) {
		message.push_back(number);
		if (number == getLTELEndMark()) {
			listening = false;
			ready = check_crc();
			if (ready) {
				setVars();
			}
		}
		if (message.size() >= max_message_size) {
			reset();
		}
	} else {
		if (number == getLTELStartMark()) {
			message.clear();
			message.push_back(number);
			listening = true;
		}
	}
}

bool CommandMessage::check_crc() {
#define CRC_BYTE_1_BACKWARD 3
#define CRC_BYTE_2_BACKWARD 2

	uint16_t crc;
	uint8_t testframe[2];
	uint8_t crc_frame[2] = { message[message.size() - CRC_BYTE_1_BACKWARD],
			message[message.size() - CRC_BYTE_2_BACKWARD] };

	crc = crc_calc(1, 3);
	memcpy(testframe, &crc, 2);
	if (testframe[1] == crc_frame[1] && testframe[0] == crc_frame[0]) {
		return true;
	}
	return false;
}

uint16_t CommandMessage::crc_calc(uint8_t start, uint8_t end) {
	uint8_t b;
	uint8_t i;
	uint16_t generator = 0x1021; //divisor is 16bit
	uint16_t crc = 0;			 // CRC value is 16bit
	uint8_t end_byte = static_cast<uint8_t>(message.size()) - end;

	for (b = start; b < end_byte; b++) {
		crc ^= ((uint16_t) (message[b] << 8)); // move byte into MSB of 16bit CRC
		for (i = 0; i < 8; i++) {
			if ((crc & 0x8000) != 0) // test for MSB = bit 15
				crc = ((uint16_t) ((crc << 1) ^ generator));
			else
				crc <<= 1;
		}
	}
	return (crc);
}

bool CommandMessage::composeMessage(std::vector<uint8_t> *data) {
	uint8_t size;
	uint16_t crc;
//	if (command_id == 0)
//		return false;

	if (data == nullptr) {
		size = 0;
	} else {
		size = data->size();
		dataSize = data->size();
	}
	message.clear();

	message.push_back(getLTELStartMark());

	message.push_back(module_function);
	message.push_back(module_id);
	message.push_back(command_id);
	message.push_back(0);

	message.push_back(size);
	if (size > 0) {
		message.insert(message.end(), data->begin(), data->end());
	}

	crc = crc_calc(1, 0);
	message.push_back(static_cast<uint8_t>(crc & 0xFF));
	message.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));

	message.push_back(getLTELEndMark());
	size = message.size();
	return true;
}

void CommandMessage::set_message(uint8_t *arr, uint8_t size) {
	for (int i = 0; i < size; i++)
		message.push_back(arr[i]);
}

bool CommandMessage::composeMessage() {
	return composeMessage(nullptr);
}

STATUS CommandMessage::validate(uint8_t *buffer, uint8_t length) {
	STATUS frameStatus = validate_protocol(buffer, length);
	if (frameStatus != (STATUS::VALID_FRAME))
		return (frameStatus);
	STATUS crcStatus = validate_crc(buffer, length);
	if (crcStatus != (STATUS::RDSS_DATA_OK))
		return (crcStatus);

	STATUS moduleStatus = validate_module(buffer, length);
	if (moduleStatus != (STATUS::CONFIG_FRAME)) {
		return (moduleStatus);
	} else {
		return (STATUS::CONFIG_FRAME);
	}
}

STATUS CommandMessage::validate_protocol(uint8_t *frame, uint8_t length) {
	if (length > (MINIMUN_FRAME_LEN)) {
		if (frame[0] == RDSS_START_MARK) {
			if (frame[length - 1] == RDSS_END_MARK)
				return STATUS::VALID_FRAME;
			else
				return STATUS::START_READING;
		} else
			return STATUS::NOT_VALID_FRAME;
	} else

		return STATUS::WAITING;
}

STATUS CommandMessage::validate_module(uint8_t *frame, uint8_t length) {

	if (frame[static_cast<int>(INDEX::MODULE_TYPE)] != module_function)
		return STATUS::RETRANSMIT_FRAME;

	return STATUS::CONFIG_FRAME;
}

STATUS CommandMessage::validate_crc(uint8_t *frame, uint8_t len) {
	uint16_t calculatedCrc;
	uint16_t savedCrc;
	savedCrc = ((uint16_t) frame[len - CRC_HIGH_BYTE_OFFSET] << 8);
	savedCrc |= (uint16_t) frame[len - CRC_LOW_BYTE_OFFSET];
	calculatedCrc = crc_get(&frame[1], len - FRAME_HEADER_SIZE);
	return ((calculatedCrc == savedCrc) ?
			(STATUS::RDSS_DATA_OK) : (STATUS::CRC_ERROR));
}

uint16_t CommandMessage::crc_get(uint8_t *buffer, uint8_t buff_len) {
	uint8_t byte_idx;
	uint8_t bit_idx;
	uint16_t generator = 0x1021; // 16-bit divisor
	uint16_t crc = 0;            // 16-bit CRC value

	for (byte_idx = 0; byte_idx < buff_len; byte_idx++) {
		crc ^= ((uint16_t) (buffer[byte_idx] << 8)); // Move byte into MSB of 16-bit CRC

		for (bit_idx = 0; bit_idx < 8; bit_idx++) {
			if ((crc & 0x8000) != 0) { // Test for MSB = bit 15
				crc = ((uint16_t) ((crc << 1) ^ generator));
			} else {
				crc <<= 1;
			}
		}
	}

	return crc;
}

void CommandMessage::save_frame(uint8_t *buffer, uint8_t length) {

	command_id = buffer[static_cast<int>(INDEX::CMD)];
	module_id = buffer[static_cast<int>(INDEX::MODULE_ID)];
	module_function = buffer[static_cast<int>(INDEX::MODULE_TYPE)];

	num_byte_data = (buffer[static_cast<int>(INDEX::DATA_LENGHT1)]
			+ buffer[static_cast<int>(INDEX::DATA_LENGHT2)]);

	storeData(num_byte_data,
			reinterpret_cast<const uint8_t*>(buffer)
					+ static_cast<int>(INDEX::DATA_START));
}
/*
 template <typename T>
 T data_frame = frame[data];
 */

void CommandMessage::storeData(size_t dataLength, const void *dataPtr) {
	if (dataLength <= 0) {
		return; // Manejo de errores: No guardar datos inválidos
	}

	dataSize = dataLength;
	message.resize(dataLength); // Redimensiona el vector para el nuevo tamaño

	// Copia la data al vector interno
	std::copy(static_cast<const uint8_t*>(dataPtr),
			static_cast<const uint8_t*>(dataPtr) + dataLength, message.begin());
}

// Devuelve el tamaño de la data almacenada
size_t CommandMessage::getDataSize() const {
	return dataSize;
}

// Devuelve el valor almacenado como uint8_t, si el tamaño es 1
uint8_t CommandMessage::getDataAsUint8() const {
	if (dataSize != 1) {
		// Manejo de errores: El tamaño no es 1
		return 0;
	}
	return message[0];
}

// Devuelve el valor almacenado como uint16_t, si el tamaño es 2
uint16_t CommandMessage::getDataAsUint16() const {
	if (dataSize != 2) {
		// Manejo de errores: El tamaño no es 4
		return 0;
	}
	union {
		uint32_t value;
		uint8_t bytes[2];
	} converter;

	std::copy(message.begin(), message.begin() + 2, converter.bytes);

	return converter.value;
}

// Devuelve el valor almacenado como uint32_t, si el tamaño es 4
uint32_t CommandMessage::getDataAsUint32() const {
	if (dataSize != 4) {
		// Manejo de errores: El tamaño no es 4
		return 0;
	}
	union {
		uint32_t value;
		uint8_t bytes[4];
	} converter;

	std::copy(message.begin(), message.begin() + 4, converter.bytes);

	return converter.value;
}

// Devuelve el valor almacenado como float, si el tamaño es 4
float CommandMessage::getDataAsFloat() const {
	if (dataSize != 4) {
		// Manejo de errores: El tamaño no es 4
		return 0;
	}
	union {
		float value;
		uint8_t bytes[4];
	} converter;

	std::copy(message.begin(), message.begin() + 4, converter.bytes);

	return converter.value;
}




