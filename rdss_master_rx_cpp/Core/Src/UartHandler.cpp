#include <UartHandler.hpp>

UartHandler::UartHandler(UART_HandleTypeDef *_huart) {
	huart = _huart;

}

UartHandler::~UartHandler() {

}

uint8_t UartHandler::transmitMessage(uint8_t *data_sen, uint16_t data_len) {
	if (HAL_UART_Transmit(huart, data_sen, data_len, 1000)) {
		return HAL_OK;
	}
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

