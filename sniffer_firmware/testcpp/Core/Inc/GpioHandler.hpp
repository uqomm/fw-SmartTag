/*
 * GpioController.hpp
 *
 *  Created on: Jul 1, 2024
 *      Author: artur
 */

#ifndef SRC_GPIOCONTROLLER_HPP_
#define SRC_GPIOCONTROLLER_HPP_

#include "main.h"
#include "Gpio.hpp"

class GpioHandler {
public:

	GpioHandler();
	virtual ~GpioHandler();
	void on(Gpio gpio);
	void off(Gpio gpio);
	void turnOnWaitOff(Gpio gpio, int wait_ms);
    bool state(Gpio gpio);
    void blink(Gpio gpio,int ka_on_timeout, int ka_blink_timeout);

private:
	void switch_state(Gpio gpio, GPIO_PinState state);
	uint32_t ka_counter;
};

#endif /* SRC_GPIOCONTROLLER_HPP_ */
