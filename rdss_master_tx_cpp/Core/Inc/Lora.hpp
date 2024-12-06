/*
 * Lora.hh
 *
 *  Created on: Aug 27, 2024
 *      Author: ALAN
 */

#ifndef INC_LORA_HPP_
#define INC_LORA_HPP_

#include <Sx1278.hpp>
#include "main.h"
#include "Memory.hpp"
#include <cstring>

constexpr uint32_t DOWNLINK_FREQ_MAX = 157000000UL;
constexpr uint32_t DOWNLINK_FREQ_MIN = 148000000;
constexpr uint32_t DOWNLINK_FREQ = 155000000;
constexpr uint32_t UPLINK_FREQ_MAX = 180000000;
constexpr uint32_t UPLINK_FREQ_MIN = 164000000;
constexpr uint32_t UPLINK_FREQ = 180000000;

constexpr uint8_t SX1278_MAX_PACKET = 100;
constexpr uint16_t SX1278_DEFAULT_TIMEOUT = 3000;
constexpr uint16_t LORA_SEND_TIMEOUT = 2000;
constexpr uint8_t SX1278_POWER_20DBM = 0xFF;
constexpr uint8_t SX1278_POWER_17DBM = 0xFC;
constexpr uint8_t SX1278_POWER_14DBM = 0xF9;
constexpr uint8_t SX1278_POWER_11DBM = 0xF6;
constexpr uint8_t LORAWAN = 0x34;
constexpr uint8_t DEFAULT_OVERCURRENTPROTECT = 0x0B;
constexpr uint8_t DEFAULT_LNAGAIN = 0x23;
constexpr uint8_t LNA_SET_BY_AGC = 0x04;
constexpr uint8_t RX_TIMEOUT_LSB = 0x08;
constexpr uint8_t PREAMBLE_LENGTH_MSB = 0x00;
constexpr uint8_t LTEL_COMPATIBLE_HOPS_PERIOD = 0x07;
constexpr uint8_t DIO0_1_2_3_CONFIG = 0x41;
constexpr uint8_t FLAGS_VALUE = 0xF7;
constexpr uint8_t LTEL_COMPATIBLE_AGC_AUTO_ON = 12;
constexpr uint8_t LTEL_COMPATIBLE_SYNC_WORD = 0x12;
constexpr uint8_t LTEL_COMPATIBLE_PREAMBLE_LENGTH_LSB = 12;
constexpr uint8_t CLEAR_IRQ_MASK = 0xFF;


enum class LinkMode {
	DOWNLINK,UPLINK
};

enum class LoraBandWidth {
	BW_7_8KHZ,
	BW_10_4KHZ,
	BW_15_6KHZ,
	BW_20_8KHZ,
	BW_31_2KHZ,
	BW_41_7KHZ,
	BW_62_5KHZ,
	BW_125KHZ,
	BW_250KHZ,
	BW_500KHZ
};

enum class CodingRate {
	CR_4_5 = 1, CR_4_6, CR_4_7, CR_4_8
};

enum class SpreadFactor {
	SF_6 = 6, SF_7, SF_8, SF_9, SF_10, SF_11, SF_12
};

enum class LoraHeaderMode {
	EXPLICIT, IMPLICIT
};
enum class CrcSum {
	CRC_DISABLE, CRC_ENABLE
};


class Lora : public Sx1278{
public:

	Lora(Gpio nss, Gpio reset, SPI_HandleTypeDef *_spi, Memory* _eeprom);
	virtual ~Lora();

	void set_lora_settings(LoraBandWidth bw, CodingRate cr, SpreadFactor sf,
			uint32_t dl_freq, uint32_t up_freq);

	void setRxFifoAddr();
	void set_downlink_frequency(uint32_t freq);
	void set_uplink_frequency(uint32_t freq);
	void save_settings();
	int8_t receive(uint8_t *data_received, LinkMode mode);
	uint8_t transmit(uint8_t *data, uint8_t data_len, LinkMode mode);
	uint32_t read_settings();
private:
	Memory* eeprom;
	uint8_t len;
	uint8_t dioConfig;
	uint8_t flagsMode;
	I2C_HandleTypeDef* hi2c;

	SpreadFactor spread_factor;
	LoraBandWidth bandwidth;
	CodingRate coding_rate;
	uint32_t frequency;
	uint32_t downlink_frequency;
	uint32_t uplink_frequency;
	LoraHeaderMode header_mode;

	//EPROM MEMORY ADDRESS
	static constexpr uint8_t EP_SF_ADDR = 0x00;
	static constexpr uint8_t EP_BW_ADDR = (EP_SF_ADDR + sizeof(uint8_t));
	static constexpr uint8_t EP_CR_ADDR = (EP_BW_ADDR + sizeof(uint8_t));
	static constexpr uint8_t EP_FRQ_ADDR = (EP_CR_ADDR + sizeof(uint8_t));
	static constexpr uint8_t EP_FRQ_UP_ADDR = (EP_FRQ_ADDR + sizeof(uint32_t));
	static constexpr uint8_t EP_FRQ_DW_ADDR = (EP_FRQ_UP_ADDR + sizeof(uint32_t));

	//EPROM MEMORY KEYS
	uint8_t sf_key;
	uint8_t bw_key;
	uint8_t cr_key;
	uint8_t frq_key;
	uint8_t frq_dw_key;
	uint8_t frq_up_key;

	//METODOS PRIVADOS
	void set_default_configurations();
	void save_lora_settings();
	void set_low_frequency_mode(DeviceOperatingMode mode);
	void changeMode(LinkMode mode);
	void set_link_frequency(LinkMode mode);
};

#endif /* INC_LORA_HPP_ */
