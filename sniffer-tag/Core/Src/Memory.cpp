/*
 * Memory.cpp
 *
 *  Created on: Jul 1, 2024
 *      Author: artur
 */

#include <Memory.hpp>

// Implementations of the member functions (would be placed outside the class)

Memory::Memory(I2C_HandleTypeDef *_hi2c) {
	hi2c = _hi2c;
	addres_scanner();
	EEPROM_CHIP_ADDR = 0xA0;
//	EEPROM_CHIP_ADDR = 0x50;
//	EEPROM_PAGE_SIZE = 32;
//	EEPrOM_PAGE_NUM = 256;

}

Memory::~Memory() {
}

uint8_t Memory::createKey(uint8_t address, uint8_t size) {
	// Add the name-address pair to the vector
	MemoryLocation mem_location;
	mem_location.address = address;
	mem_location.size = size;
	value_addr.push_back(mem_location);
  return (static_cast<uint8_t>(value_addr.size() - 1));
}

uint8_t Memory::EEPROM_Read(uint8_t address) {
	HAL_StatusTypeDef status;
	uint8_t data = 0;
	status = HAL_I2C_Mem_Read(hi2c, EEPROM_CHIP_ADDR, address, I2C_MEMADD_SIZE_16BIT, &data, 1, 100);
	if(status==HAL_OK){
		return data;
	}else{
		return status;
	}

}

void Memory::EEPROM_Write(uint8_t address, uint8_t data) {
	uint8_t buff[2];
	uint8_t stored_data;
	buff[0] = address;
	buff[1] = data;
	HAL_StatusTypeDef status;

	stored_data = EEPROM_Read(address);
	if (stored_data != data)
		status = HAL_I2C_Mem_Write(hi2c, EEPROM_CHIP_ADDR, address,
				I2C_MEMADD_SIZE_16BIT, &buff[1], 1, 100);
}

void Memory::addres_scanner() {
	    /*-[ I2C Bus Scanning ]-*/
    int i= 0;
    HAL_StatusTypeDef ret;
	int j= 0;
	for(i=1; i<128; i++)
    {
        ret = HAL_I2C_IsDeviceReady(hi2c, (uint16_t)(i<<1), 3, 5);
        if(ret == HAL_OK)
        {
        	i2c_addres[j++] = i;
        }
    }
}
