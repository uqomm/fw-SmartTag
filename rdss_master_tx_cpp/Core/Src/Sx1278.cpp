/*
 * Sx1278.cpp
 *
 *  Created on: Aug 29, 2024
 *      Author: ALAN
 */

#include <Sx1278.hpp>


Sx1278::Sx1278(Gpio _nss, Gpio _reset, SPI_HandleTypeDef *_spi) {

	nss = _nss;
	reset = _reset;
	spi = _spi;
	HAL_GPIO_WritePin(nss.get_port(), nss.get_pin(), GPIO_PIN_SET);
	HAL_GPIO_WritePin(reset.get_port(), reset.get_pin(), GPIO_PIN_SET);


}

Sx1278::~Sx1278() {
}

uint8_t Sx1278::read_reg_addr(LoraRegisters reg, uint8_t reg_len) {
	if(reg_len <= 0 )
		return -1;
	uint8_t address = static_cast<uint8_t>(reg);
	HAL_GPIO_WritePin(nss.get_port(), nss.get_pin(), GPIO_PIN_RESET);  // pull the pin low
	HAL_Delay(1);
	HAL_SPI_Transmit(spi, &address, 1, 100);  // send address
	HAL_SPI_Receive(spi, fifo, reg_len, 100);  // receive 6 bytes data
	HAL_Delay(1);
	HAL_GPIO_WritePin(nss.get_port(), nss.get_pin(), GPIO_PIN_SET);  // pull the pin high

	return 0;
}

void Sx1278::write_reg_addr(uint8_t address, uint8_t *cmd, uint8_t lenght) {
	if (lenght > 4)
		return;
	uint8_t tx_data[5] = { 0 };
	tx_data[0] = address | 0x80;
	int j = 0;
	for (int i = 1; i <= lenght; i++) {
		tx_data[i] = cmd[j++];
	}
	HAL_GPIO_WritePin(nss.get_port(), nss.get_pin(), GPIO_PIN_RESET);  // pull the pin low
	HAL_SPI_Transmit(spi, tx_data, lenght + 1, 1000);
	HAL_GPIO_WritePin(nss.get_port(), nss.get_pin(), GPIO_PIN_SET);  // pull the pin high
	HAL_Delay(10);
}

uint8_t Sx1278::read_8bit_reg(LoraRegisters reg) {
	uint8_t reg_len = 1;
	if(!read_reg_addr(reg, reg_len))
		return fifo[0];
	return -1;
}

void Sx1278::write_8bit_reg(LoraRegisters reg, uint8_t _value) {
	uint8_t value[1];
	value[0] = _value;
	write_reg_addr(static_cast<uint8_t>(reg), value, 1);
}

void Sx1278::set_carrier_frquency(uint32_t frequency) {
	uint64_t freq = ((uint64_t) frequency << 19) / FXOSC;
	uint8_t freq_reg[3];
	freq_reg[0] = (uint8_t) (freq >> 16);
	freq_reg[1] = (uint8_t) (freq >> 8);
	freq_reg[2] = (uint8_t) (freq >> 0);
	write_reg_addr(static_cast<uint8_t>(LoraRegisters::RegFrMsb), freq_reg, sizeof(freq_reg));
}

void Sx1278::setRegModemConfig(uint8_t modem_cfg1, uint8_t modem_cfg2) {
	write_reg_addr(static_cast<uint8_t>(LoraRegisters::RegModemConfig1), &modem_cfg1, 1); //Explicit Enable CRC Enable(0x02) & Error Coding rate 4/5(0x01), 4/6(0x02), 4/7(0x03), 4/8(0x04)
	write_reg_addr(static_cast<uint8_t>(LoraRegisters::RegModemConfig2), &modem_cfg2, 1);
}

void Sx1278::setDetectionParametersReg() {
	uint8_t tmp;
	tmp = read_8bit_reg(LoraRegisters::RegDetectOptimize);
	tmp &= 0xF8;
	tmp |= 0x05;
//	write_reg_addr(static_cast<uint8_t>(LoraRegisters::RegDetectOptimize), &tmp, 1);
	tmp = 0x0C;
	write_reg_addr(static_cast<uint8_t>(LoraRegisters::RegDetectionThreshold), &tmp, 1);
}

void Sx1278::hw_reset() {
	HAL_GPIO_WritePin(nss.get_port(), nss.get_pin(), GPIO_PIN_SET);
	HAL_GPIO_WritePin(nss.get_port(), nss.get_pin(), GPIO_PIN_RESET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(nss.get_port(), nss.get_pin(), GPIO_PIN_SET);
	HAL_Delay(100);
}

int8_t Sx1278::wait_irq(uint8_t mask, uint16_t timeout) {
	int timeStart = HAL_GetTick();
	uint8_t irqFlags = 0;
	while (1) {
		irqFlags = read_8bit_reg(LoraRegisters::RegIrqFlags);
		if ((irqFlags & mask)) {
			int timeEnd = HAL_GetTick();
			lastTxTime = timeEnd - timeStart;
			write_8bit_reg(LoraRegisters::RegIrqFlags, mask);
			read_8bit_reg(LoraRegisters::RegOcp);
			return 0;
		}
		if (HAL_GetTick() - timeStart > timeout) {
			//hw_reset();
			read_8bit_reg(LoraRegisters::RegOcp);
			return -1;
		}
	}

	return 0;
}

uint8_t Sx1278::write_tx_fifo_data(uint8_t *data, uint8_t data_len) {
	if (data_len > 0) {
		write_8bit_reg(LoraRegisters::RegPayloadLength, data_len);
		write_8bit_reg(LoraRegisters::RegFifoAddrPtr, DATA_BUFFER_BASE_ADDR); //DATA_BUFFER_BASE_ADDR
		for (int i = 0; i < data_len; i++)
			write_8bit_reg(LoraRegisters::RegFifo, data[i]);

		//write_reg_addr(LoraRegisters::RegFifo,data, data_len);
	}
	return data_len;
}

