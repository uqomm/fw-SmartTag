/*
 * Sx1278.hpp
 *
 *  Created on: Aug 29, 2024
 *      Author: ALAN
 */

#ifndef INC_TXSX1278_HPP_
#define INC_TXSX1278_HPP_

#include <stdint.h>
#include <string.h>
#include "main.h"
#include  "Gpio.hpp"
#include "Rxsx1278.hpp"

class Txsx1278 {
public:
	Txsx1278(Gpio _nss, Gpio _reset, SPI_HandleTypeDef *_spi);
	virtual ~Txsx1278();

	void write_reg_addr(uint8_t address, uint8_t *cmd, uint8_t lenght);
	void setRegModemConfig(uint8_t modem_cfg1,uint8_t modem_cfg2);
	void setDetectionParametersReg();
	void hw_reset();
	uint8_t write_tx_fifo_data(uint8_t *data, uint8_t data_len);

protected:

	static constexpr uint16_t SPI_TIMEOUT = 1000;
//RFM98 Internal registers Address
// Additional settings
	static constexpr uint8_t REG_LR_PLLHOP = 0x44;
	static constexpr uint8_t REG_LR_TCXO = 0x4B;
	static constexpr uint8_t REG_LR_PADAC = 0x4D;
	static constexpr uint8_t REG_LR_FORMERTEMP = 0x5B;
	static constexpr uint8_t REG_LR_AGCREF = 0x61;
	static constexpr uint8_t REG_LR_AGCTHRESH1 = 0x62;
	static constexpr uint8_t REG_LR_AGCTHRESH2 = 0x63;
	static constexpr uint8_t REG_LR_AGCTHRESH3 = 0x64;

///// Direcciones de prueba
	static constexpr uint8_t DIRECCION_0X80 = 0x80;
	static constexpr uint8_t DIRECCION_0X81 = 0x81;
	static constexpr uint8_t DIRECCION_0X82 = 0x82;
	static constexpr uint8_t DIRECCION_0X83 = 0x83;
	static constexpr uint8_t DIRECCION_0X84 = 0x84;
	static constexpr uint8_t DIRECCION_0X85 = 0x85;
	static constexpr uint8_t DIRECCION_0X86 = 0x86;
	static constexpr uint8_t DIRECCION_0X87 = 0x87;


	/**********************************************************
	 **Parameter table define
	 **********************************************************/

	static constexpr uint8_t RX_TIMEOUT_MASK = (0x1 << 7);       /*!< 0x00000020 */
	static constexpr uint8_t RX_DONE_MASK = (0x1 << 6);
	static constexpr uint8_t PAYLOAD_CRC_ERROR_MASK = (0x1 << 5);
	static constexpr uint8_t VALID_HEADER_MASK = (0x1 << 4);
	static constexpr uint8_t TX_DONE_MASK = (0x1 << 3);
	static constexpr uint8_t CAD_DONE_MASK = (0x1 << 2);
	static constexpr uint8_t FHSS_CHANGE_CHANNEL_MASK = (0x1 << 1);
	static constexpr uint8_t CAD_DETECTED_MASK = (0x1 << 0);

	static constexpr uint8_t LORA_MODE_ACTIVATION = (0x00 | 8 << 4);
	static constexpr uint8_t HIGH_FREQUENCY_MODE = (0x00 | 0 << 3);
	static constexpr uint8_t LOW_FREQUENCY_MODE = (0x00 | 1 << 3);

	static constexpr uint8_t DIO0_RX_DONE = (0x00 | 0 << 6);
	static constexpr uint8_t DIO0_TX_DONE = (0x00 | 1 << 6);
	static constexpr uint8_t DIO0_CAD_DONE = (0x00 | 2 << 6);
	static constexpr uint8_t DIO1_RX_TIMEOUT = (0x00 | 0 << 4);
	static constexpr uint8_t DIO1_FHSS_CHANGE_CHANNEL = (0x00 | 1 << 4);
	static constexpr uint8_t DIO1_CAD_DETECTED = (0x00 | 2 << 4);
	static constexpr uint8_t DIO2_FHSS_CHANGE_CHANNEL = (0x00 | 0 << 2);
	static constexpr uint8_t DIO3_CAD_DONE = (0x00 | 0 << 0);
	static constexpr uint8_t DIO3_VALID_HEADER = (0x00 | 1 << 0);
	static constexpr uint8_t DIO3_PAYLOAD_CRC_ERROR = (0x00 | 2 << 0);

	static constexpr uint8_t MASK_ENABLE = 0;
	static constexpr uint8_t MASK_DISABLE = 1;

	DeviceOperatingMode operatingMode;
	DeviceOperatingMode operating_mode;

	uint32_t lastTxTime;
	uint32_t lastRxTime;
	uint8_t fifo[255];
	uint8_t *rxData;
	uint8_t rxSize;
	uint8_t *txData;
	uint8_t txSize;
	uint8_t readBytes;
	Gpio nss;
	Gpio reset;
	SPI_HandleTypeDef *spi;
	bool saveParameters;

	//METODOS PROTECTED
	void set_carrier_frquency(uint32_t frequency);
	void write_8bit_reg(LoraRegisters reg, uint8_t _value);
	uint8_t read_reg_addr(LoraRegisters reg, uint8_t reg_len);
	uint8_t read_8bit_reg(LoraRegisters reg);
	int8_t wait_irq(uint8_t mask, uint16_t timeout);

};

#endif /* INC_TXSX1278_HPP_ */
