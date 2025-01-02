/*
 * PcbLed.cpp
 *
 *  Created on: Aug 7, 2024
 *      Author: artur
 */
#include "PcbLed.hpp"

PcbLed::PcbLed(TIM_HandleTypeDef *_timer_handler, uint32_t _timer_channel) {

	timer_handler = _timer_handler;
	timer_channel = _timer_channel;
	datasentflag = false;

}

PcbLed::~PcbLed() {

}

void PcbLed::set_data_flag(bool data_sent_flat) {
	datasentflag = data_sent_flat;
}

uint32_t PcbLed::createPwmSequence(Ws2812Color *led, uint8_t led_number,
		uint16_t *pwmData) {
	uint32_t indx = 0;
	uint32_t color = 0;

	for (int i = 0; i < led_number; i++) {
		color = led[i].get_rgb_color();
		for (int j = 23; j >= 0; j--) {
			if (color & (1 << j)) {
				pwmData[indx] = 27;  // /10/13 of 37 19
			} else
				pwmData[indx] = 13;  // 10/13 of 37 11
			indx++;
		}
	}
	for (int i = 0; i < 50; i++) {
		pwmData[indx] = 0;
		indx++;
	}
	return (indx);
}

void PcbLed::set_and_send_led(Ws2812Color *led, uint8_t led_number) {
	uint32_t indx;
	// TODO : REvisar acá
	//uint16_t *pwmData = new uint16_t[(24 * led_number) + 280 * 2];

//	uint16_t *pwmData = reinterpret_cast<uint16_t*>(malloc(24 * led_number + 280 * 2));
	memset(pwmData, 0, sizeof(pwmData));
	indx = createPwmSequence(led, led_number, pwmData);
	HAL_TIM_PWM_Start_DMA(timer_handler, timer_channel, (uint32_t*) (pwmData),
			indx + 10);
}

void PcbLed::set_and_send_led_color(Ws2812Color *led, uint8_t led_number,
		uint32_t delay, Color color) {
	uint32_t indx;

	led->off();
	switch (color) {
	case Color::RED:
		led->set_red_bright(255);
		break;
	case Color::YELLOW:
		led->set_green_bright(255);
		led->set_red_bright(255);
		break;
	case Color::GREEN:
		led->set_green_bright(255);
		break;
	default:
		break;
	}

	memset(pwmData, 0, sizeof(pwmData));
	indx = createPwmSequence(led, led_number, pwmData);
	HAL_TIM_PWM_Start_DMA(timer_handler, timer_channel, (uint32_t*) (pwmData),
			indx + 10);

	HAL_Delay(delay);
}

void PcbLed::wait_for_data_send() {
	while (datasentflag == false) {
	}
	;
	datasentflag = false;
}

void PcbLed::spiralAnimation(Ws2812Color *led, uint8_t led_number,
		uint8_t step_ms, uint8_t green, uint8_t red, uint8_t blue) {

	for (int j = 0; j < led_number; j++) {
		led[j].set_green_bright(green);
		led[j].set_red_bright(red);
		led[j].set_blue_bright(blue);
		if (j == 0) {
			for (int i = j; i < led_number; i++) {
				if (j != i) {
					led[i].off();

				}
			}
		} else {
			led[j - 1].off();
		}
		set_and_send_led(led, led_number);
		HAL_Delay(step_ms);
	}
	led[led_number - 1].off();

	set_and_send_led(led, led_number);
}

void PcbLed::off(Ws2812Color *led, uint8_t led_number) {
	for (int i = 0; i < led_number; i++)
		led[i].off();
	set_and_send_led(led, led_number);
}

