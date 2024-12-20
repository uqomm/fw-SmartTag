/*
 * DistanceHandler.cpp
 *
 *  Created on: Oct 7, 2024
 *      Author: manue
 */

#include "DistanceHandler.hpp"

DistanceHandler::DistanceHandler(uint8_t _total_readings) {
	total_readings =_total_readings/2;
	readings = new double[total_readings];
}

DistanceHandler::~DistanceHandler() {
	// TODO Auto-generated destructor stub
}

void DistanceHandler::save(
		uint64_t rtd_init, uint64_t rtd_resp, float clockOffsetRatio) {
	double distance = calc_distance(rtd_init, rtd_resp,
			clockOffsetRatio);
	readings[counter] = distance;
	if (counter <= (total_readings)-1)
		counter++;
	else
		error_track++;
}

void DistanceHandler::error_crc_increment(){
	error_crc_times++;
}

double DistanceHandler::calc_distance(
		uint64_t rtd_init, uint64_t rtd_resp, float clockOffsetRatio) {
	float _DWT_TIME_UNITS = (1.0 / 499.2e6 / 128.0); //!< = 15.65e-12 s
	double _SPEED_OF_LIGHT = 299702547;
	// Hold copies of computed time of flight and distance here for reference
	double tof = ((rtd_init - rtd_resp * (1 - clockOffsetRatio)) / 2.0)
			* _DWT_TIME_UNITS;
	return tof * _SPEED_OF_LIGHT;
}

double DistanceHandler::get_media() {
	double sum = 0;
	for (uint8_t i = 0; i < counter; i++)
		sum += readings[i];

	return (double) ((sum) / counter);
}

uint8_t DistanceHandler::get_counter(){
	return counter;
}

uint8_t DistanceHandler::get_error_track(){
	return error_track;
}

uint8_t DistanceHandler::get_error_crc_times(){
	return error_crc_times;
}

uint8_t DistanceHandler::get_total_readings_for_two_transcievers(){
	return total_readings*2;
}

uint16_t DistanceHandler::get_media_multiplier(uint8_t multiplier) {
	return get_media() * multiplier;
}

void DistanceHandler::clear() {
	counter = 0;
	for(uint8_t i= 0; i<(total_readings); i++)
		readings[i] = 0;
	value = 0;
	error_crc_times = 0;
	error_track = 0;
}

double DistanceHandler::get_last_distance(){
	return readings[counter-1];
}

void DistanceHandler::set_total_readings(uint8_t _total_readings){
	total_readings = _total_readings/2;
}

void DistanceHandler::change_total_readings(uint8_t _total_readings) {

  delete[] readings;
  total_readings = _total_readings / 2;
  readings = new double[total_readings];

  for (uint8_t i = 0; i < total_readings; ++i) {
    readings[i] = 0.0;
  }
}


