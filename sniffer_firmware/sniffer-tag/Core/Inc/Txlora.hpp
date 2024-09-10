/*
 * Lora.hh
 *
 *  Created on: Aug 27, 2024
 *      Author: ALAN
 */

#ifndef INC_TXLORA_HPP_
#define INC_TXLORA_HPP_

#include <Txsx1278.hpp>
#include "main.h"
#include "Memory.hpp"
#include <cstring>
#include "Rxlora.hpp"

class Txlora : public Txsx1278{
public:

	Txlora(Gpio _nss, Gpio _reset, SPI_HandleTypeDef *_spi, Memory* _eeprom);
	virtual ~Txlora();

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

#endif /* INC_TXLORA_HPP_ */
