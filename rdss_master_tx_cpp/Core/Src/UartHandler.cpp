#include <UartHandler.hpp>

UartHandler::UartHandler(UART_HandleTypeDef *_huart) {
	huart = _huart;

}

UartHandler::~UartHandler() {

}

uint8_t UartHandler::transmitMessage(uint8_t *data_sen, uint16_t data_len) {
	if (HAL_UART_Transmit(huart, data_sen, data_len, 1000) == HAL_OK) {
		return (uint8_t)HAL_OK;
	}
	return (uint8_t)HAL_ERROR;
}

bool UartHandler::get_and_send_command(CommandMessage command) {
	uint8_t *data = command.get_composed_message().data();
	uint8_t size = command.get_composed_message().size();
	if (HAL_UART_Transmit(huart, data, size, 1000)) {
		return HAL_OK;
	}
}

uint8_t UartHandler::read(uint8_t *data_received) {
	read_timeout(data_received, 2000);
}

uint8_t UartHandler::read_timeout(uint8_t *data_received, uint16_t timeout_ms) {
	int i = 0;
	uint8_t size = sizeof(buffer);
	HAL_StatusTypeDef resp;
	resp = HAL_UART_Receive(huart, buffer, size, timeout_ms);

	for (i = 0; buffer[0] == 0x7e && buffer[i] != 0x7f; i++) {
		if (i == 255) {
			i = 0;
			break;
		}
	}

	if (i > 0) {
		i++;
		memcpy(data_received, buffer, i);
		memset(buffer,0,sizeof(buffer));
		return i;
	} else{
		memset(buffer,0,sizeof(buffer));
		return 0;
	}
}


void UartHandler::enable_receive_interrupt(uint8_t _bytes_it){
	HAL_UART_Receive_IT(huart, buffer , _bytes_it);
}


uint8_t UartHandler::read_timeout_new(uint8_t *data_received) {
	int i = 0;


	for (i = 0; buffer[0] == 0x7e && buffer[i] != 0x7f; i++) {
		if (i == MAX_BUFFER_UART -1) {
			i = 0;
			break;
		}
	}

	if (i > 0) {
		i++;
		memcpy(data_received, buffer, i);
		memset(buffer, 0, sizeof(buffer));
		return i;
	} else {
		memset(buffer, 0, sizeof(buffer));
		return 0;
	}
}

