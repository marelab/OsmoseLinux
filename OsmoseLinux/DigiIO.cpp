/*
 * SensorIR.cpp
 *
 *  Created on: Jan 20, 2018
 * On Power GPIO
 * GPIO 0 - 8  				HIGH INPUT pull-ups to 3V3 enabled
 * 		9 - 27 LOW (21-29)  have pull-downs to 0V enabled
 */
#include "../json/json.h"
#include <sys/time.h>
#include <unistd.h>
#include "wiringPi/wiringPi.h"

#include "DigiIO.h"



DigiIO::DigiIO(std::string name,int port,int dbounceTime,int input) {
	auto logger = spdlog::get("logger");

	this->NAME				= name;
	this->PORT				= port;
	this->DBOUNCE_TIME_MS 	= dbounceTime;
	this->INPUT_IO 			= input;


	if (this->INPUT_IO==1)
	{
		pinMode(this->PORT, INPUT);
		logger->debug("Config Pin Name: "+this->NAME+" INPUT ");
	}else{
		pinMode(this->PORT, OUTPUT);
		digitalWrite (this->PORT ,0) ;
		logger->debug("Config Pin Name: "+this->NAME+" OUTPUT ");
	}
}

DigiIO::~DigiIO() {

}

bool DigiIO::read() {
	if (this->INPUT_IO ==1){
	 return	digitalRead (this->PORT);
	}
	return false;
}

void DigiIO::write(bool onoff) {
	if (onoff)
	    digitalWrite(this->PORT, 1);
	else
		digitalWrite(this->PORT, 0);
}
