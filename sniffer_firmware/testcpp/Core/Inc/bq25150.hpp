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
HAL_StatusTypeDef I2C_write(uint8_t, uint8_t, uint8_t, I2C_HandleTypeDef*);

class Bq25155{
public:
	uint8_t data[0x05];
	Gpio reset;
	Gpio low_power;
	Gpio charge_en;
	GpioHandler _gpio;
	I2C_HandleTypeDef* i2c;
	Bq25155(Gpio rst, Gpio lp, Gpio ce,I2C_HandleTypeDef* _i2c);
	void mr_set();
	void mr_reset();
	void manual_read_adc_bat();
	uint8_t adc_flags();
	uint16_t register_adc_bat();
	void register_init_all();
	void register_Factory_Reset();
	void register_init_all_2();
	HAL_StatusTypeDef I2C_write(uint8_t deviceAddress, uint8_t registerAddress, uint8_t registerData, I2C_HandleTypeDef* i2c);

};
