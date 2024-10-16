/*
 * Sx1278Lora.cpp
 *
 *  Created on: Aug 27, 2024
 *      Author: ALAN
 */

#include <Lora.hpp>


Lora::Lora(Gpio _nss, Gpio _reset, SPI_HandleTypeDef *_spi, Memory* _eeprom) :
	Sx1278(_nss, _reset, _spi) {

	spi = _spi;
	eeprom = _eeprom;

	//EPROM KEYS
	sf_key = eeprom->createKey(EP_SF_ADDR, sizeof(uint8_t));
	bw_key = eeprom->createKey(EP_BW_ADDR, sizeof(uint8_t));
	cr_key = eeprom->createKey(EP_CR_ADDR, sizeof(uint8_t));
	frq_key = eeprom->createKey(EP_FRQ_ADDR, sizeof(uint32_t));
	frq_dw_key = eeprom->createKey(EP_FRQ_UP_ADDR, sizeof(uint32_t));
	frq_up_key = eeprom->createKey(EP_FRQ_DW_ADDR, sizeof(uint32_t));

	set_default_configurations();
}

Lora::~Lora() {
}

int8_t Lora::receive(uint8_t *data_received, LINKMODE mode) {
	uint8_t op_mode = (read_8bit_reg(LoraRegisters::RegOpMode));
	if (!(op_mode & static_cast<uint8_t>(DeviceOperatingMode::RX_CONTINUOUS))) {

		set_low_frequency_mode(DeviceOperatingMode::SLEEP); //Change modem mode Must in Sleep mode
		HAL_Delay(1);

		set_link_frequency(mode);

		write_8bit_reg(LoraRegisters::RegFifoAddrPtr, DATA_BUFFER_BASE_ADDR);
		set_low_frequency_mode(DeviceOperatingMode::RX_CONTINUOUS);
	}
	if (!wait_irq(RX_DONE_MASK, 1)) {
		uint8_t rx_nb_bytes = read_8bit_reg(LoraRegisters::RegRxNbBytes); //Number for received bytes
		if (read_reg_addr(LoraRegisters::RegFifo, rx_nb_bytes) == 0) {
			memcpy(data_received, fifo, rx_nb_bytes);
			return rx_nb_bytes;
		} else {
			data_received = NULL;
			return 0;
		}
	} else
		return 0;
}

void Lora::set_link_frequency(LINKMODE mode) {
	if (mode == LINKMODE::DOWNLINK) {
		set_carrier_frquency(downlink_frequency);
	} else if (mode == LINKMODE::UPLINK) {
		set_carrier_frquency(uplink_frequency);
	}
}

uint8_t Lora::transmit(uint8_t *data, uint8_t data_len, LINKMODE mode) {
	uint32_t initime = HAL_GetTick();
	set_low_frequency_mode(DeviceOperatingMode::STANDBY);

	set_link_frequency(mode);

	write_tx_fifo_data(data, data_len);

	set_low_frequency_mode(DeviceOperatingMode::TX);

	if((wait_irq(TX_DONE_MASK, 2000))){
			return -1;
	}else {
		uint32_t endtime = HAL_GetTick();
		uint32_t duracion = endtime - initime;
		return 0;
	}
}

void Lora::set_lora_settings(LoraBandWidth bw, CodingRate cr, SpreadFactor sf,
		uint32_t dl_freq, uint32_t up_freq) {
	uint8_t symb_timeout_msb = 0;
	if (sf < SpreadFactor::SF_6 || sf > SpreadFactor::SF_12)
		spread_factor = SpreadFactor::SF_10;
	else {
		spread_factor = sf;
		if (spread_factor == SpreadFactor::SF_6) {
			header_mode = LoraHeaderMode::IMPLICIT;
			symb_timeout_msb = 0x03;
			setDetectionParametersReg();
		} else {
			header_mode = LoraHeaderMode::EXPLICIT;
			symb_timeout_msb = 0x00;
		}
	}

	if (bw < LoraBandWidth::BW_7_8KHZ || bw > LoraBandWidth::BW_500KHZ)
		bw = LoraBandWidth::BW_62_5KHZ;
	else
		bandwidth = bw;

	if (cr < CodingRate::CR_4_5 || cr > CodingRate::CR_4_8)
		cr = CodingRate::CR_4_6;
	else
		coding_rate = cr;

	if (up_freq < UPLINK_FREQ_MIN || up_freq > UPLINK_FREQ_MAX)
		uplink_frequency = UPLINK_FREQ;
	else
		uplink_frequency = up_freq;

	if (dl_freq < DOWNLINK_FREQ_MIN || dl_freq > DOWNLINK_FREQ_MAX)
		downlink_frequency = DOWNLINK_FREQ;
	else
		downlink_frequency = dl_freq;

	uint8_t modem_cfg1 = 0;
	uint8_t modem_cfg2 = 0;

	modem_cfg1 = static_cast<uint8_t>(bandwidth) << 4;
	modem_cfg1 |= static_cast<uint8_t>(coding_rate) << 1;
	modem_cfg1 |= static_cast<uint8_t>(header_mode);

	modem_cfg2 = static_cast<uint8_t>(spread_factor) << 4;
	modem_cfg2 |= static_cast<uint8_t>(CrcSum::CRC_DISABLE) << 2; //TODO revisar este valor
	modem_cfg2 |= static_cast<uint8_t>(symb_timeout_msb);
	set_low_frequency_mode(DeviceOperatingMode::SLEEP);
	setRegModemConfig(modem_cfg1, modem_cfg2);

	save_settings();
}

void Lora::set_low_frequency_mode(DeviceOperatingMode mode) {
	uint8_t cmd = LORA_MODE_ACTIVATION | LOW_FREQUENCY_MODE
			| static_cast<uint8_t>(mode);
	write_8bit_reg(LoraRegisters::RegOpMode, cmd);
	operatingMode = mode;
}

void Lora::set_default_configurations() {
	set_low_frequency_mode(DeviceOperatingMode::SLEEP);
	uint8_t flagsMode = 0x00;
	HAL_Delay(15);
	write_8bit_reg(LoraRegisters::RegFifoAddrPtr, DATA_BUFFER_BASE_ADDR);
	write_8bit_reg(LoraRegisters::RegFifoRxCurrentaddr, DATA_BUFFER_BASE_ADDR);
	write_8bit_reg(LoraRegisters::RegFifoRxBaseAddr, DATA_BUFFER_BASE_ADDR);
	write_8bit_reg(LoraRegisters::RegFifoTxBaseAddr, DATA_BUFFER_BASE_ADDR);
	write_8bit_reg(LoraRegisters::RegSyncWord, LTEL_COMPATIBLE_SYNC_WORD);
	write_8bit_reg(LoraRegisters::RegPaConfig, SX1278_POWER_17DBM);
	write_8bit_reg(LoraRegisters::RegOcp, DEFAULT_OVERCURRENTPROTECT);
	write_8bit_reg(LoraRegisters::RegLna, DEFAULT_LNAGAIN);
	write_8bit_reg(LoraRegisters::RegSymbTimeoutLsb, RX_TIMEOUT_LSB);
	write_8bit_reg(LoraRegisters::RegPreambleMsb, PREAMBLE_LENGTH_MSB);
	write_8bit_reg(LoraRegisters::RegPreambleLsb,
			LTEL_COMPATIBLE_PREAMBLE_LENGTH_LSB);
	write_8bit_reg(LoraRegisters::RegIrqFlags, CLEAR_IRQ_MASK);
	write_8bit_reg(LoraRegisters::RegHopPeriod, LTEL_COMPATIBLE_HOPS_PERIOD);
	write_8bit_reg(LoraRegisters::RegModemConfig3, LTEL_COMPATIBLE_AGC_AUTO_ON);
	//write_8bit_reg(LoraRegisters::RegDioMapping1, dioConfig);
	write_8bit_reg(LoraRegisters::RegIrqFlagsMask, flagsMode);
}

void Lora::save_settings(){
	eeprom->setValue(sf_key,  static_cast<uint8_t>(spread_factor));
	eeprom->setValue(bw_key,  static_cast<uint8_t>(bandwidth));
	eeprom->setValue(cr_key,  static_cast<uint8_t>(coding_rate));
	eeprom->setValue(frq_key, static_cast<uint32_t>(frequency));
	eeprom->setValue(frq_dw_key, static_cast<uint32_t>(downlink_frequency));
	eeprom->setValue(frq_up_key, static_cast<uint32_t>(uplink_frequency));
}

uint32_t Lora::read_settings(){
	uint8_t i = eeprom->getValue<uint8_t>(sf_key);
	uint8_t o = eeprom->getValue<uint8_t>(bw_key);
	uint8_t p = eeprom->getValue<uint8_t>(cr_key);
	uint32_t l = eeprom->getValue<uint32_t>(frq_key);
	uint32_t j = eeprom->getValue<uint32_t>(frq_dw_key);
	uint32_t h = eeprom->getValue<uint32_t>(frq_up_key);
	return i;
}
