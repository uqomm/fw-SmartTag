/*
 * bq25150.h
 *
 *  Created on: Jul 29, 2024
 *      Author: claud
 */
#include "GpioHandler.hpp"
#ifndef INC_BQ25150_H_
#define INC_BQ25150_H_





#endif /* INC_BQ25150_H_ */

#define BQ25155_ADDR	(0x6B)

#define BQ25155_STAT0	(0x00)
#define BQ25155_STAT1	(0x01)
#define BQ25155_STAT2	(0x02)
#define BQ25155_FLAG0	(0x03)
#define BQ25155_FLAG1	(0x04)
#define BQ25155_FLAG2	(0x05)
#define BQ25155_FLAG3	(0x06)
#define BQ25155_MASK0	(0x07)
#define BQ25155_MASK1	(0x08)
#define BQ25155_MASK2	(0x09)
#define BQ25155_MASK3	(0x0A)
#define BQ25155_VBAT_CTRL	(0x12)
#define BQ25155_ICHG_CTRL	(0x13)
#define BQ25155_PCHRGCTRL	(0x14)
#define BQ25155_TERMCTRL	(0x15)
#define BQ25155_BUVLO	(0x16)
#define BQ25155_CHARGERCTRL0	(0x17)
#define BQ25155_CHARGERCTRL1	(0x18)
#define BQ25155_ILIMCTRL	(0x19)
#define BQ25155_LDOCTRL	(0x1D)
#define BQ25155_MRCTRL	(0x30)
#define BQ25155_ICCTRL0	(0x35)
#define BQ25155_ICCTRL1	(0x36)
#define BQ25155_ICCTRL2	(0x37)
#define BQ25155_ADCCTRL0	(0x40)
#define BQ25155_ADCCTRL1	(0x41)
#define BQ25155_ADCDATA_VBAT_M	(0x42)
#define BQ25155_ADCDATA_VBAT_L	(0x43)
#define BQ25155_ADCDATA_TS_M	(0x44)
#define BQ25155_ADCDATA_TS_L	(0x45)
#define BQ25155_ADCDATA_ICHG_M	(0x46)
#define BQ25155_ADCDATA_ICHG_L	(0x47)
#define BQ25155_ADCDATA_ADCIN_M	(0x48)
#define BQ25155_ADCDATA_ADCIN_L	(0x49)
#define BQ25155_ADCDATA_VIN_M	(0x4A)
#define BQ25155_ADCDATA_VIN_L	(0x4B)
#define BQ25155_ADCDATA_PMID_M	(0x4C)
#define BQ25155_ADCDATA_PMID_L	(0x4D)
#define BQ25155_ADCDATA_IIN_M	(0x4E)
#define BQ25155_ADCDATA_IIN_L	(0x4F)
#define BQ25155_ADCDATA_TJ_M	(0x50)
#define BQ25155_ADCDATA_TJ_L	(0x51)
#define BQ25155_ADCALARM_COMP1_M	(0x52)
#define BQ25155_ADCALARM_COMP1_L	(0x53)
#define BQ25155_ADCALARM_COMP2_M	(0x54)
#define BQ25155_ADCALARM_COMP2_L	(0x55)
#define BQ25155_ADCALARM_COMP3_M	(0x56)
#define BQ25155_ADCALARM_COMP3_L	(0x57)
#define BQ25155_ADC_READ_EN	(0x58)
#define BQ25155_TS_FASTCHGCTRL	(0x61)
#define BQ25155_TS_COLD	(0x62)
#define BQ25155_TS_COOL	(0x63)
#define BQ25155_TS_WARM	(0x64)
#define BQ25155_TS_HOT	(0x65)
#define BQ25155_DEVICE_ID	(0x6F)
#define BQ25155_STAT	BIT7 | BIT6




HAL_StatusTypeDef I2C_write(uint8_t, uint8_t, uint8_t, I2C_HandleTypeDef*);

class Bq25155{
public:
	uint8_t data[0x05];
	Gpio reset;
	Gpio low_power;
	Gpio charge_en;
	GpioHandler _gpio;
	I2C_HandleTypeDef* i2c;
	Bq25155(Gpio rst, Gpio ce,I2C_HandleTypeDef* _i2c);
	void mr_set();
	void mr_reset();
	void manual_read_adc_bat();
	uint8_t read_register(uint8_t dir_mem);
	uint8_t adc_flags();
	uint16_t register_adc_bat();
	uint8_t register_STAT0();
	void register_init_all();
	void register_Factory_Reset();
	void register_init_all_2();
	HAL_StatusTypeDef I2C_write(uint8_t deviceAddress, uint8_t registerAddress, uint8_t registerData, I2C_HandleTypeDef* i2c);

};
