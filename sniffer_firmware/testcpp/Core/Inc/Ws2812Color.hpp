/*
 * Ws2812Color.hpp
 *
 *  Created on: Aug 7, 2024
 *      Author: ALAN
 */

#include "main.h"

#ifndef INC_WS2812COLOR_HPP_
#define INC_WS2812COLOR_HPP_

class Ws2812Color {
public:
	Ws2812Color();
	virtual ~Ws2812Color();
	void off();
	void red_on();
	void green_on();
	void blue_on();
	void orange_on();
	void set_red_bright(uint8_t _bright);
	void set_blue_bright(uint8_t _bright);
	void set_green_bright(uint8_t _bright);
	void set_bright(uint8_t _bright);
	uint32_t get_rgb_color();



private:
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t bright;
};




#endif /* INC_WS2812COLOR_HPP_ */
