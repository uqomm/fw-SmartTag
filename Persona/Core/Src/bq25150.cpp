/*
 * bq.c
 *
 *  Created on: Apr 24, 2024
 *      Author: claud
 */
#include "bq25150.hpp"

//#define BQ25155_ADDR	(0x6B)
//
//#define BQ25155_STAT0	(0x00)
//#define BQ25155_STAT1	(0x01)
//#define BQ25155_STAT2	(0x02)
//#define BQ25155_FLAG0	(0x03)
//#define BQ25155_FLAG1	(0x04)
//#define BQ25155_FLAG2	(0x05)
//#define BQ25155_FLAG3	(0x06)
//#define BQ25155_MASK0	(0x07)
//#define BQ25155_MASK1	(0x08)
//#define BQ25155_MASK2	(0x09)
//#define BQ25155_MASK3	(0x0A)
//#define BQ25155_VBAT_CTRL	(0x12)
//#define BQ25155_ICHG_CTRL	(0x13)
//#define BQ25155_PCHRGCTRL	(0x14)
//#define BQ25155_TERMCTRL	(0x15)
//#define BQ25155_BUVLO	(0x16)
//#define BQ25155_CHARGERCTRL0	(0x17)
//#define BQ25155_CHARGERCTRL1	(0x18)
//#define BQ25155_ILIMCTRL	(0x19)
//#define BQ25155_LDOCTRL	(0x1D)
//#define BQ25155_MRCTRL	(0x30)
//#define BQ25155_ICCTRL0	(0x35)
//#define BQ25155_ICCTRL1	(0x36)
//#define BQ25155_ICCTRL2	(0x37)
//#define BQ25155_ADCCTRL0	(0x40)
//#define BQ25155_ADCCTRL1	(0x41)
//#define BQ25155_ADCDATA_VBAT_M	(0x42)
//#define BQ25155_ADCDATA_VBAT_L	(0x43)
//#define BQ25155_ADCDATA_TS_M	(0x44)
//#define BQ25155_ADCDATA_TS_L	(0x45)
//#define BQ25155_ADCDATA_ICHG_M	(0x46)
//#define BQ25155_ADCDATA_ICHG_L	(0x47)
//#define BQ25155_ADCDATA_ADCIN_M	(0x48)
//#define BQ25155_ADCDATA_ADCIN_L	(0x49)
//#define BQ25155_ADCDATA_VIN_M	(0x4A)
//#define BQ25155_ADCDATA_VIN_L	(0x4B)
//#define BQ25155_ADCDATA_PMID_M	(0x4C)
//#define BQ25155_ADCDATA_PMID_L	(0x4D)
//#define BQ25155_ADCDATA_IIN_M	(0x4E)
//#define BQ25155_ADCDATA_IIN_L	(0x4F)
//#define BQ25155_ADCDATA_TJ_M	(0x50)
//#define BQ25155_ADCDATA_TJ_L	(0x51)
//#define BQ25155_ADCALARM_COMP1_M	(0x52)
//#define BQ25155_ADCALARM_COMP1_L	(0x53)
//#define BQ25155_ADCALARM_COMP2_M	(0x54)
//#define BQ25155_ADCALARM_COMP2_L	(0x55)
//#define BQ25155_ADCALARM_COMP3_M	(0x56)
//#define BQ25155_ADCALARM_COMP3_L	(0x57)
//#define BQ25155_ADC_READ_EN	(0x58)
//#define BQ25155_TS_FASTCHGCTRL	(0x61)
//#define BQ25155_TS_COLD	(0x62)
//#define BQ25155_TS_COOL	(0x63)
//#define BQ25155_TS_WARM	(0x64)
//#define BQ25155_TS_HOT	(0x65)
//#define BQ25155_DEVICE_ID	(0x6F)
//#define BQ25155_STAT	BIT7 | BIT6






Bq25155::Bq25155(Gpio rst, Gpio ce,I2C_HandleTypeDef* _i2c){
	reset = rst;
	charge_en = ce;
	i2c = _i2c;
}
void Bq25155::mr_set(){
	_gpio.on(reset);
}
void Bq25155::mr_reset(){
	_gpio.off(reset);
}
void Bq25155::manual_read_adc_bat(){
	this->mr_reset();

	I2C_write(BQ25155_ADDR, BQ25155_ADCCTRL0, 0x00, i2c); //manual read
	I2C_write(BQ25155_ADDR, BQ25155_ADC_READ_EN, 0x08, i2c); // enable adc_bat
	I2C_write(BQ25155_ADDR, BQ25155_ADCCTRL0, 0x20, i2c); //read adc

	this->mr_set();
}

uint8_t Bq25155::adc_flags(){
	this->mr_reset();
	HAL_StatusTypeDef wr = HAL_I2C_Mem_Read(i2c, (BQ25155_ADDR<<1), BQ25155_FLAG2, 1, &data[2], 1, 1000);
	this->mr_set();
	if(wr == HAL_OK){
		return data[2];
	}
	else{
		return 1;
	}
}

uint8_t Bq25155::read_register(uint8_t mem_dir){
	this->mr_reset();
	HAL_StatusTypeDef wr = HAL_I2C_Mem_Read(i2c, (BQ25155_ADDR<<1), mem_dir, 1, &data[0], 1, 1000);
	this->mr_set();

	return data[0];
}


uint8_t Bq25155::register_STAT0(){
	HAL_StatusTypeDef rw;
	this->mr_reset();
		rw = HAL_I2C_Mem_Read(i2c, (BQ25155_ADDR<<1), BQ25155_STAT0, 1, &data[0], 1, 1000);
	this->mr_set();
	if(rw == HAL_OK){
		return (data[0]);
	}
	else{
		  return 1;
	  }
}

uint16_t Bq25155::register_adc_bat(){
	HAL_StatusTypeDef rw;
	this->mr_reset();
	for(uint8_t i=BQ25155_ADCDATA_VBAT_M; i<=BQ25155_ADCDATA_VBAT_L; i++){
		rw = HAL_I2C_Mem_Read(i2c, (BQ25155_ADDR<<1), i, 1, &data[i-BQ25155_ADCDATA_VBAT_M], 1, 1000);
	}
	this->mr_set();
	if(rw == HAL_OK){
		return (data[0]<<8) + data[1];
	}
	else{
		  return 1;
	  }
}



HAL_StatusTypeDef Bq25155::I2C_write(uint8_t deviceAddress, uint8_t registerAddress, uint8_t registerData, I2C_HandleTypeDef* i2c){
	  uint8_t buffer[2] = {registerAddress, registerData};
	  return HAL_I2C_Master_Transmit(i2c, (uint16_t)(deviceAddress<<1), buffer , 2, 1000);
}

void Bq25155::enter_ship_mode(){
	this->mr_reset();
	this->I2C_write(BQ25155_ADDR, BQ25155_ICCTRL0, 0x90, i2c);
}

void Bq25155::register_init_all_2(){
	this->I2C_write(BQ25155_ADDR, BQ25155_MASK0, 0x00, i2c);	//Modificado 0x20
	this->I2C_write(BQ25155_ADDR, BQ25155_MASK1, 0x00, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_MASK2, 0x71, i2c);	//Modificado 0x80
	this->I2C_write(BQ25155_ADDR, BQ25155_MASK3, 0x00, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_VBAT_CTRL, 0x3C, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ICHG_CTRL, 0xFF, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_PCHRGCTRL, 0x9E, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_TERMCTRL, 0x14, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_BUVLO, 0x06, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_CHARGERCTRL0, 0x42, i2c); //se dehabilita TS function y se deshabilita charging on HOT/COLD Only
	this->I2C_write(BQ25155_ADDR, BQ25155_CHARGERCTRL1, 0xC8, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ILIMCTRL, 0x06, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_LDOCTRL, 0xB0, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_MRCTRL, 0x2A, i2c);	//Revisar
	this->I2C_write(BQ25155_ADDR, BQ25155_ICCTRL0, 0x10, i2c);	//No se habilita mask all interrupts
	this->I2C_write(BQ25155_ADDR, BQ25155_ICCTRL1, 0x00, i2c);	//Revisar PIN NTC 0x40
	this->I2C_write(BQ25155_ADDR, BQ25155_ICCTRL2, 0x40, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ADCCTRL0, 0x00, i2c); //Se deja en lectura manual de ADC y el comparador 1 se dehabilita
	this->I2C_write(BQ25155_ADDR, BQ25155_ADCCTRL1, 0x00, i2c);	//Se deshabilita comparador 3 (compara con TS)
	this->I2C_write(BQ25155_ADDR, BQ25155_ADCALARM_COMP1_M, 0x23, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ADCALARM_COMP1_L, 0x20, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ADCALARM_COMP2_M, 0x38, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ADCALARM_COMP2_L, 0x90, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ADCALARM_COMP3_M, 0x00, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ADCALARM_COMP3_L, 0x00, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ADC_READ_EN, 0x08, i2c);	//Con todo deshabilitado se puede medir la bateria
	this->I2C_write(BQ25155_ADDR, BQ25155_TS_FASTCHGCTRL, 0x34, i2c);	//Los parametros de TS, son para cuando se usa la funcionalidad de TS
	this->I2C_write(BQ25155_ADDR, BQ25155_TS_COLD, 0x7C, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_TS_COOL, 0x6D, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_TS_WARM, 0x38, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_TS_HOT, 0x27, i2c);
}

void Bq25155::register_Factory_Reset(){
	this->I2C_write(BQ25155_ADDR, BQ25155_MASK0, 0x00, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_MASK1, 0x00, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_MASK2, 0x71, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_MASK3, 0x00, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_VBAT_CTRL, 0x3C, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ICHG_CTRL, 0x08, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_PCHRGCTRL, 0x02, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_TERMCTRL, 0x14, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_BUVLO, 0x00, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_CHARGERCTRL0, 0x82, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_CHARGERCTRL1, 0xC2, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ILIMCTRL, 0x06, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_LDOCTRL, 0xB0, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_MRCTRL, 0x2A, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ICCTRL0, 0x10, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ICCTRL1, 0x00, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ICCTRL2, 0x40, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ADCCTRL0, 0x02, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ADCCTRL1, 0x40, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ADCALARM_COMP1_M, 0x23, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ADCALARM_COMP1_L, 0x20, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ADCALARM_COMP2_M, 0x38, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ADCALARM_COMP2_L, 0x90, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ADCALARM_COMP3_M, 0x00, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ADCALARM_COMP3_L, 0x00, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_ADC_READ_EN, 0x00, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_TS_FASTCHGCTRL, 0x34, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_TS_COLD, 0x7C, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_TS_COOL, 0x6D, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_TS_WARM, 0x38, i2c);
	this->I2C_write(BQ25155_ADDR, BQ25155_TS_HOT, 0x27, i2c);
}


