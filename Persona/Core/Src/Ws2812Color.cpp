/*
 * Ws2812Color.cpp
 *
 *  Created on: Aug 7, 2024
 *      Author: ALAN
 */
#include "Ws2812Color.hpp"

Ws2812Color::Ws2812Color() {
	blue = 0;
	red = 0;
	green = 0;
	bright = 0;
}

Ws2812Color::~Ws2812Color() {
}

void Ws2812Color::off()  // 0-45
{
	blue = 0;
	red = 0;
	green = 0;
}

void Ws2812Color::red_on(){
	red = bright;
}
void Ws2812Color::green_on(){
	green = bright;
}

void Ws2812Color::blue_on(){
	blue = bright;

}

void Ws2812Color::set_bright(uint8_t _bright){
	bright = _bright;
}

void  Ws2812Color::orange_on(){
	set_red_bright(236);
	green_on();
}

void Ws2812Color::set_red_bright(uint8_t _bright){
	red = _bright;
}

void Ws2812Color::set_blue_bright(uint8_t _bright){
	blue = _bright;
}

void Ws2812Color::set_green_bright(uint8_t _bright){
	green = _bright;
}


uint32_t Ws2812Color::get_rgb_color(){
	return ((green << 16) | (red << 8) | (blue));
}
