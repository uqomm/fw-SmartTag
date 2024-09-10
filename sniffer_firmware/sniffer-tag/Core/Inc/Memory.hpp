/*
 * Memory.hpp
 *
 *  Created on: Jul 1, 2024
 *      Author: artur
 */

#ifndef INC_MEMORY_HPP_
#define INC_MEMORY_HPP_

#include <main.h>
#include <vector>
#include <string>

class Memory {
public:

	typedef struct {
		uint8_t address;
		uint8_t size;
	}MemoryLocation;

	Memory(I2C_HandleTypeDef* _hi2c);
	virtual ~Memory();
	uint8_t createKey(uint8_t address, uint8_t size);

	template <typename T>
	T getValue(uint8_t key);

	template <typename T>
	void setValue(uint8_t key, T value);

private:
	I2C_HandleTypeDef* hi2c;
	uint8_t EEPROM_CHIP_ADDR;
	uint8_t EEPROM_PAGE_SIZE;
	uint8_t EEPrOM_PAGE_NUM = 32;
	std::vector<MemoryLocation> value_addr;
	uint8_t EEPROM_Read(uint8_t address);
	void EEPROM_Write(uint8_t address, uint8_t data);

	template <typename T>
	void EEPROM_byte_Write(uint8_t addr, T data);

	template <typename T>
	T EEPROM_byte_Read(uint8_t address);
};

#include "Memory.tpp"  // Incluir el archivo de implementación de plantillas (.tpp)


#endif /* INC_MEMORY_HPP_ */
