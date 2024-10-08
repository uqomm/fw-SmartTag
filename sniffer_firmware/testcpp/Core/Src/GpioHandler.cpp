/*
 * GpioController.cpp
 *
 *  Created on: Jul 1, 2024
 *      Author: artur
 */

#include "GpioHandler.hpp"


GpioHandler::GpioHandler(){
}



GpioHandler::~GpioHandler() {
	// TODO Auto-generated destructor stub
}

void GpioHandler::blink(Gpio gpio,int ka_on_timeout, int ka_timeout) {
    if (HAL_GetTick() - ka_counter > ka_timeout)
      ka_counter = HAL_GetTick();
    else {
       if (HAL_GetTick() - ka_counter > ka_on_timeout)
          off(gpio);
       else
          on(gpio);
    }

}

void GpioHandler::switch_state(Gpio gpio, GPIO_PinState state){
	HAL_GPIO_WritePin(gpio.get_port(), gpio.get_pin(), state);
}

void GpioHandler::on(Gpio gpio){
	this->switch_state(gpio, GPIO_PIN_SET);
}

void GpioHandler::off(Gpio gpio){
	this->switch_state(gpio, GPIO_PIN_RESET);
}

void GpioHandler::turnOnWaitOff(Gpio gpio, int wait_ms){
	this->on(gpio);
	HAL_Delay(wait_ms);
	this->off(gpio);
}

bool GpioHandler::state(Gpio gpio){
    return (HAL_GPIO_ReadPin(gpio.get_port(), gpio.get_pin()));
}
