/*
 * Sx1278.hpp
 *
 *  Created on: Aug 29, 2024
 *      Author: ALAN
 */

#ifndef INC_SX1278_HPP_
#define INC_SX1278_HPP_

#include <stdint.h>
#include <string.h>
#include "main.h"
#include  "Gpio.hpp"

constexpr uint32_t FXOSC = 32000000;
constexpr uint8_t DATA_BUFFER_BASE_ADDR = 0x00;

enum class LoraRegisters {
	RegFifo = 0x00,
	RegOpMode = 0x01,
	RegFrMsb = 0x06,
	RegFrMid = 0x07,
	RegFrLsb = 0x08,
	RegPaConfig = 0x09,
	RegPaRamp = 0x0A,
	RegOcp = 0x0B,
	RegLna = 0x0C,
	RegFifoAddrPtr = 0x0D,
	RegFifoTxBaseAddr = 0x0E,
	RegFifoRxBaseAddr = 0x0F,
	RegFifoRxCurrentaddr = 0x10,
	RegIrqFlagsMask = 0x11,
	RegIrqFlags = 0x12,
	RegRxNbBytes = 0x13,
	RegRxHeaderCntValueMsb = 0x14,
	RegRxHeaderCntValueLsb = 0x15,
	RegRxPacketCntValueMsb = 0x16,
	RegRxPacketCntValueLsb = 0x17,
	RegModemStat = 0x18,
	RegPktSnrValue = 0x19,
	RegPktRssiValue = 0x1A,
	RegRssiValue = 0x1B,
	RegHopChannel = 0x1C,
	RegModemConfig1 = 0x1D,
	RegModemConfig2 = 0x1E,
	RegSymbTimeoutLsb = 0x1F,
	RegPreambleMsb = 0x20,
	RegPreambleLsb = 0x21,
	RegPayloadLength = 0x22,
	RegMaxPayloadLength = 0x23,
	RegHopPeriod = 0x24,
	RegFifoRxByteAddr = 0x25,
	RegModemConfig3 = 0x26,
	RegDetectOptimize = 0x31,
	RegDetectionThreshold = 0x37,
	RegSyncWord = 0x39,
	RegDioMapping1 = 0x40,
	RegDioMapping2 = 0x41,
	RegVersion = 0x42,
	RegPllHop = 0x44,
	RegTCXO = 0x4B,
	RegPaDac = 0x4D,
	RegFormerTemp = 0x5B,
	RegAgcRef = 0x61,
	RegAgcThresh1 = 0x62,
	RegAgcThresh2 = 0x63,
	RegAgcThresh3 = 0x64
};

enum class DeviceOperatingMode {
	SLEEP,
	STANDBY,
	FSTX, //Frequency synthesis TX
	TX,
	FSRX, //Frequency synthesis RX
	RX_CONTINUOUS,
	RX_SINGLE,
	CAD //Channel activity detection
};

enum class IrgFlagBit {
	CadDetected,    // Bit 0
	FhssChangeChannel, // Bit 1
	CadDone,        // Bit 2
	TxDone,          // Bit 3
	ValidHeader,     // Bit 4
	PayloadCrcError,  // Bit 5
	RxDone,          // Bit 6
	RxTimeout        // Bit 7
};

	constexpr uint8_t CAD_DETECTED_MASK = (1 << static_cast<int>(IrgFlagBit::CadDetected));
	constexpr uint8_t FHSS_CHANGE_CHANNEL_MASK = (1 << static_cast<int>(IrgFlagBit::FhssChangeChannel));
	constexpr uint8_t CAD_DONE_MASK = (1 << static_cast<int>(IrgFlagBit::CadDone));
	constexpr uint8_t TX_DONE_MASK = (1 << static_cast<int>(IrgFlagBit::TxDone));
	constexpr uint8_t VALID_HEADER_MASK = (1 << static_cast<int>(IrgFlagBit::ValidHeader));
	constexpr uint8_t PAYLOAD_CRC_ERROR_MASK = (1 << static_cast<int>(IrgFlagBit::PayloadCrcError));
	constexpr uint8_t RX_DONE_MASK = (1 << static_cast<int>(IrgFlagBit::RxDone));
	constexpr uint8_t RX_TIMEOUT_MASK = (1 << static_cast<int>(IrgFlagBit::RxTimeout));

class Sx1278 {
public:
	Sx1278(Gpio _nss, Gpio _reset, SPI_HandleTypeDef *_spi);
	virtual ~Sx1278();

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
	SPI_HandleTypeDef *spi;
	Gpio nss;
	Gpio reset;
	bool saveParameters;

	//METODOS PROTECTED
	void set_carrier_frquency(uint32_t frequency);
	void write_8bit_reg(LoraRegisters reg, uint8_t _value);
	uint8_t read_reg_addr(LoraRegisters reg, uint8_t reg_len);
	uint8_t read_8bit_reg(LoraRegisters reg);
	int8_t wait_irq(uint8_t mask, uint16_t timeout);

};

#endif /* INC_SX1278_HPP_ */
