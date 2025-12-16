/*
 * Gpio.hpp
 *
 *  Created on: Jul 1, 2024
 *      Author: artur
 */

#ifndef SRC_GPIO_HPP_
#define SRC_GPIO_HPP_

#include "main.h"

class Gpio {

public:
	Gpio(GPIO_TypeDef* _port = NULL , uint16_t _pin = 0);
	GPIO_TypeDef* get_port();
	uint16_t get_pin();

private:
	GPIO_TypeDef* port;
	uint16_t pin;
};

#endif /* SRC_GPIO_HPP_ */
