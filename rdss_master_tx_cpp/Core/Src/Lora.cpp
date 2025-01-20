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

int8_t Lora::receive(uint8_t *data_received, LinkMode mode) {
	uint8_t op_mode = (read_8bit_reg(LoraRegisters::RegOpMode));
	if (!(op_mode & static_cast<uint8_t>(DeviceOperatingMode::RX_CONTINUOUS))) {

		set_low_frequency_mode(DeviceOperatingMode::SLEEP); //Change modem mode Must in Sleep mode
		HAL_Delay(1);

		set_link_frequency(mode);

		write_8bit_reg(LoraRegisters::RegFifoAddrPtr, DATA_BUFFER_BASE_ADDR);
		set_low_frequency_mode(DeviceOperatingMode::RX_CONTINUOUS);
	}
	if (!wait_irq(RX_DONE_MASK, 2000)) {
		uint8_t rx_nb_bytes = read_8bit_reg(LoraRegisters::RegRxNbBytes); //Number for received bytes
		if (read_reg_addr(LoraRegisters::RegFifo, rx_nb_bytes) == 0) {
			memcpy(data_received, fifo, rx_nb_bytes);
			return rx_nb_bytes;
		} else {
			data_received = NULL;
			return -1;
		}
	} else
		return -1;
}

void Lora::set_link_frequency(LinkMode mode) {
	if (mode == LinkMode::DOWNLINK) {
		set_carrier_frquency(downlink_frequency);
	} else if (mode == LinkMode::UPLINK) {
		set_carrier_frquency(uplink_frequency);
	}
}

uint8_t Lora::transmit(uint8_t *data, uint8_t data_len, LinkMode mode) {

	set_low_frequency_mode(DeviceOperatingMode::STANDBY);

	set_link_frequency(mode);

	write_tx_fifo_data(data, data_len);
	set_low_frequency_mode(DeviceOperatingMode::TX);
	if((wait_irq(TX_DONE_MASK, 1000)))
			return HAL_ERROR;
	return HAL_OK;

}


void Lora::check_already_store_data(){
	this->read_settings();
	this->set_lora_settings(bandwidth, coding_rate, spread_factor, downlink_frequency, uplink_frequency);
	this->save_settings();
}



uint32_t Lora::get_rx_frequency(){
	return uplink_frequency;
}

uint32_t Lora::get_tx_frequency(){
	return downlink_frequency;
}

uint8_t Lora::get_spread_factor(){
	return (uint8_t)spread_factor;
}

uint8_t Lora::get_coding_rate(){
	return (uint8_t)coding_rate;
}

uint8_t Lora::get_bandwidth(){
	return (uint8_t)bandwidth;
}

void Lora::set_tx_freq(uint32_t freq){
	downlink_frequency = freq;
}

void Lora::set_rx_freq(uint32_t freq){
	uplink_frequency = freq;
}

void Lora::set_bandwidth(uint8_t bw){
	bandwidth = (LoraBandWidth)bw;
}

void Lora::set_spread_factor(uint8_t spread){
	spread_factor = (SpreadFactor)spread;
}

void Lora::set_coding_rate(uint8_t cr){
	coding_rate = (CodingRate)cr;
}


void Lora::set_default_parameters(){
	downlink_frequency = (0xffff);
	uplink_frequency = (0xffff);
	bandwidth = (LoraBandWidth)(0xff);
	spread_factor = (SpreadFactor)(0xff);
	coding_rate = (CodingRate)(0xff);
	this->set_lora_settings(bandwidth, coding_rate, spread_factor, downlink_frequency, uplink_frequency);
	save_settings();
}


void Lora::set_lora_settings(LoraBandWidth bw, CodingRate cr, SpreadFactor sf,
		uint32_t dl_freq, uint32_t up_freq) {
	uint8_t symb_timeout_msb = 0;
	if (sf < SpreadFactor::SF_6 || sf > SpreadFactor::SF_12)
		spread_factor = SpreadFactor::SF_7;
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
		bandwidth = LoraBandWidth::BW_500KHZ;
	else
		bandwidth = bw;

	if (cr < CodingRate::CR_4_5 || cr > CodingRate::CR_4_8)
		coding_rate = CodingRate::CR_4_6;
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
}


void Lora::configure_modem(){

	uint8_t symb_timeout_msb = 0;
	if (spread_factor < SpreadFactor::SF_6 || spread_factor > SpreadFactor::SF_12)
		spread_factor = SpreadFactor::SF_10;
	else {
		if (spread_factor == SpreadFactor::SF_6) {
			header_mode = LoraHeaderMode::IMPLICIT;
			symb_timeout_msb = 0x03;
			setDetectionParametersReg();
		} else {
			header_mode = LoraHeaderMode::EXPLICIT;
			symb_timeout_msb = 0x00;
		}
	}

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
	spread_factor = (SpreadFactor)eeprom->getValue<uint8_t>(sf_key);
	bandwidth = (LoraBandWidth)eeprom->getValue<uint8_t>(bw_key);
	coding_rate = (CodingRate)eeprom->getValue<uint8_t>(cr_key);
	frequency = eeprom->getValue<uint32_t>(frq_key);
	downlink_frequency = eeprom->getValue<uint32_t>(frq_dw_key);
	uplink_frequency = eeprom->getValue<uint32_t>(frq_up_key);
	return frequency;
}
