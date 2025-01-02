/**
  ******************************************************************************

  EEPROM.h Using the HAL I2C Functions
  Author:   ControllersTech
  Updated:  Feb 16, 2021

  ******************************************************************************
  Copyright (C) 2017 ControllersTech.com

  This is a free software under the GNU license, you can redistribute it and/or modify it under the terms
  of the GNU General Public License version 3 as published by the Free Software Foundation.
  This software library is shared with public for educational purposes, without WARRANTY and Author is not liable for any damages caused directly
  or indirectly by this software, read more about this on the GNU General Public License.

  ******************************************************************************
*/

#ifndef INC_EEPROM_H_
#define INC_EEPROM_H_
#ifdef __cplusplus // Check if compiling with a C++ compiler
extern "C" {
#endif
#include "stdint.h"

#define SF_OFFSET 0
#define BW_OFFSET 1
#define CR_OFFSET 2
#define FUNCTION_OFFSET 3
#define ID_OFFSET 4
#define UL_OFFSET 0
#define DL_OFFSET 5

#define K24C02_PAGES 128
#define K24C02_PAGE_SIZE 32

#define K24C02_PAGE_ADDR(page) ((page) * K24C02_PAGE_SIZE)

void EEPROM_Write (uint16_t page, uint16_t offset, uint8_t *data, uint16_t size);
void EEPROM_Read (uint16_t page, uint16_t offset, uint8_t *data, uint16_t size);
void EEPROM_PageErase (uint16_t page);

void EEPROM_Write_NUM (uint16_t page, uint16_t offset, float  fdata);
float EEPROM_Read_NUM (uint16_t page, uint16_t offset);
void escribir_uint32_en_eeprom(uint8_t *mem_ptr, uint32_t value);
#ifdef __cplusplus
} // End of extern "C" block
#endif
#endif /* INC_EEPROM_H_ */
