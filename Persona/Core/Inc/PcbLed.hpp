/*
 * PcbLed.hpp
 *
 *  Created on: Aug 7, 2024
 *      Author: ALAN
 */

#ifndef INC_PCBLED_HPP_
#define INC_PCBLED_HPP_

#include "main.h"
#include <stdlib.h>
#include "Ws2812Color.hpp"
#include "string.h"


enum class Color {
    RED,
	YELLOW,
    GREEN
};


class PcbLed {
public:
    PcbLed(TIM_HandleTypeDef *_timer_handler, uint32_t _timer_channel);
    ~PcbLed();

    void set_data_flag(bool data_sent_flag);
    void set_and_send_led(Ws2812Color *led, uint8_t led_number);
    void set_and_send_led_color(Ws2812Color *led, uint8_t led_number, uint32_t delay, Color color);
    void spiralAnimation(Ws2812Color *led,uint8_t led_number, uint8_t step_ms, uint8_t green, uint8_t red,
    		uint8_t blue);
    void off(Ws2812Color * led, uint8_t led_number);
    void wait_for_data_send();



protected:
	TIM_HandleTypeDef *timer_handler;
	uint32_t timer_channel;
	bool datasentflag;
	uint16_t pwmData[800];
	uint32_t createPwmSequence(Ws2812Color *led, uint8_t led_number,uint16_t *pwmData);

	typedef enum {
		left1,
		left2,
		left3,
		up1,
		up2,
		up3,
		up4,
		up5,
		right3,
		right2,
		right1,
		center2,
		center1,
		none
	} WS2812_Name_t;





	void set_downlink_power_led(uint16_t downlink_power_level);
	void set_input_voltage_led(uint16_t vin);
	void set_agc_downlink_led(uint16_t level, uint16_t agc, uint16_t ref);
	void set_agc_uplink_led(uint16_t level, uint16_t agc,uint16_t ref);
	void set_base_overcurrent_led(uint16_t current);
	void set_conventional_reverse_led(bool _data);
	void set_smart_tune_led(bool _data);



};

#endif /* INC_PCBLED_HPP_ */
