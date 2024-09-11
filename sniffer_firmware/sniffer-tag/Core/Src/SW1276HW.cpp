/*
 * SW1276HW.cpp
 *
 *  Created on: Jun 13, 2024
 *      Author: artur
 */

#include <SW1276HW.hpp>

SX1276_HW::SX1276_HW(SPI_HandleTypeDef* _hspi , GPIO_TypeDef* _nssPort, uint16_t _nssPin,
        GPIO_TypeDef* _nrstPort, uint16_t _nrstPin,
        GPIO_TypeDef* _dio0Port, uint16_t _dio0Pin){
	hspi = _hspi;
	nssPort = _nssPort;
	nssPin = _nssPin;
	nrstPort = _nrstPort;
	nrstPin = _nrstPin;
	dio0Port = _dio0Port;
	dio0Pin = _dio0Pin;
}

uint8_t SX1276_HW::readReg(uint8_t address) const {
	uint8_t rec = 0;
	HAL_StatusTypeDef res;

	HAL_GPIO_WritePin(nssPort, nssPin, GPIO_PIN_RESET);
	HAL_Delay(1);

	res = HAL_SPI_Transmit(hspi, &address, 1, 100);
	res = HAL_SPI_Receive(hspi, &rec, 1, 100);

	HAL_Delay(1);
	HAL_GPIO_WritePin(nssPort, nssPin, GPIO_PIN_SET);

	if (res != HAL_OK) {
		Error_Handler();
	}
	return (rec);

}

void SX1276_HW::writeReg(uint8_t address, const uint8_t *cmd,
		uint8_t length) const { // Method of the class
	HAL_StatusTypeDef res;
	if (length > 4) {
		return;
	}
	uint8_t tx_data[5] = { 0 };
	tx_data[0] = address | 0x80;
	int j = 0;
	for (int i = 1; i <= length; i++) {
		tx_data[i] = cmd[j++];
	}

	HAL_GPIO_WritePin(nssPort, nssPin, GPIO_PIN_RESET);
	res = HAL_SPI_Transmit(hspi, tx_data, length + 1, 1000);
	HAL_GPIO_WritePin(nssPort, nssPin, GPIO_PIN_SET);

	if (res != HAL_OK) {
		Error_Handler();
	}
}
