/*
 * DistanceHandler.hpp
 *
 *  Created on: Oct 7, 2024
 *      Author: manue
 */

#ifndef SRC_DISTANCEHANDLER_HPP_
#define SRC_DISTANCEHANDLER_HPP_
#include "main.h"


class DistanceHandler {
public:
	DistanceHandler(uint8_t _total_readings);
	void save(uint64_t rtd_init, uint64_t rtd_resp, float clockOffsetRatio);

	uint16_t get_media_multipier(uint8_t multiplier);
	void clear();
	virtual ~DistanceHandler();
	double get_last_distance();

private:
	uint8_t total_readings;
	double *readings;
	uint16_t value;
	uint8_t error_times;
	uint8_t counter;
	uint8_t error_track=0;

	double calc_distance(uint64_t rtd_init, uint64_t rtd_resp,
			float clockOffsetRatio);
	double get_media();

};

#endif /* SRC_DISTANCEHANDLER_HPP_ */
