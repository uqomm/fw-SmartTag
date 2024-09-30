/*
 * SW1276HW.h
 *
 *  Created on: Jun 13, 2024
 *      Author: artur
 */

#ifndef INC_SW1276HW_HPP_
#define INC_SW1276HW_HPP_
#include "main.h"


class SX1276_HW {
public:
	SX1276_HW();
    // Constructor (to initialize members)
    SX1276_HW(SPI_HandleTypeDef* _hspi , GPIO_TypeDef* _nssPort, uint16_t _nssPin,
               GPIO_TypeDef* _nrstPort, uint16_t _nrstPin,
               GPIO_TypeDef* _dio0Port, uint16_t _dio0Pin);
    // Default Copy Constructor (performs shallow copy)
    SX1276_HW(const SX1276_HW& other) = default;

    // Default Assignment Operator (performs shallow copy)
    SX1276_HW& operator=(const SX1276_HW& other) = default;

    // You likely DO NOT need a destructor because you're not
    // doing any dynamic memory allocation in this class.

    // Accessor methods (getters)
    SPI_HandleTypeDef* getSPI() const { return (hspi); }
    GPIO_TypeDef* getNSSPort() const { return (nssPort); }
    uint16_t getNSSPin() const { return (nssPin); }
    // ... [Add getters for other members] ...

    // Mutator methods (setters - if you need to modify the members)
    void setSPI(SPI_HandleTypeDef* newSPI) { hspi = newSPI; }
    // ... [Add setters for other members if needed] ...

    uint8_t readReg(uint8_t address)const;
    void writeReg(uint8_t address, const uint8_t *cmd, uint8_t length) const;

private:
    SPI_HandleTypeDef* hspi;
    GPIO_TypeDef* nssPort;
    uint16_t nssPin;
    GPIO_TypeDef* nrstPort;
    uint16_t nrstPin;
    GPIO_TypeDef* dio0Port;
    uint16_t dio0Pin;
};

#endif /* INC_SW1276HW_HPP_ */
