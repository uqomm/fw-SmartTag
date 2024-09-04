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


void GpioHandler::switch_state(Gpio gpio, GPIO_PinState state){
	HAL_GPIO_WritePin(gpio.get_port(), gpio.get_pin(), state);
}

void GpioHandler::turnOn(Gpio gpio){
	this->switch_state(gpio, GPIO_PIN_SET);
}

void GpioHandler::turnOff(Gpio gpio){
	this->switch_state(gpio, GPIO_PIN_RESET);
}

void GpioHandler::turnOnWaitOff(Gpio gpio, int wait_ms){
	this->turnOn(gpio);
	HAL_Delay(wait_ms);
	this->turnOff(gpio);
}
