#include <Gpio.hpp>

Gpio::Gpio(GPIO_TypeDef* _port, uint16_t _pin) {
	port = _port;
	pin = _pin;
}

GPIO_TypeDef* Gpio::get_port(){
	return(port);
}

uint16_t Gpio::get_pin(){
	return(pin);
}
