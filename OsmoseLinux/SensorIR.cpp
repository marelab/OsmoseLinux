/*
 * SensorIR.cpp
 *
 *  Created on: Jan 20, 2018
 *      Author: marc
 */



#include "SensorIR.h"

namespace std {


SensorIR::SensorIR(std::string name,int port,int dbounceTime) {
	struct timeval now;
	this->NAME				= name;
	this->PORT				= port;
	this->DBOUNCE_TIME_MS 	= dbounceTime;
	pinMode(this->PORT, INPUT);
	gettimeofday(&now, NULL);
	this->STATE_NOW = digitalRead(this->PORT);
	this->STABLE_STATE = this->STATE_NOW;
	this->STATE_BEFORE = this->STATE_NOW;
	this->TIME_START = now.tv_sec;
	this->STABLE_TIME_STAMP = TIME_START;
}


SensorIR::~SensorIR() {

}

bool SensorIR::UpdateState(){
	struct timeval now;
	long diff,bounce;

	gettimeofday(&now, NULL);
/*	this->STATE_NOW = digitalRead(this->PORT);
	bounce = this->DBOUNCE_TIME_MS*1000;
	diff = (((now.tv_sec * 1000000) + now.tv_usec)-(this->TIME_START + bounce) );

	if((this->STABLE_STATE !=  this->STATE_NOW)){
			if (diff > 0){
				this->STABLE_STATE = this->STATE_NOW;
				this->TIME_START = (now.tv_sec * 1000000) + now.tv_u#include <wiringPi.h>sec;
				this->STABLE_TIME_STAMP = this->TIME_START;
			}
	}else{
		this->TIME_START = (now.tv_sec * 1000000) + now.tv_usec;
	}
	return this->STABLE_STATE;

	////////////////////////////////////////////////////////////////////
*/
	this->STATE_NOW = digitalRead(this->PORT);
	if ((STATE_NOW != STATE_BEFORE)){
		this->TIME_START = now.tv_sec;
	}
	else	// STATE BEFORE = STATE NOW
	{
		diff  = (now.tv_sec - (this->TIME_START + this->DBOUNCE_TIME_MS) );
		//cout << this->NAME <<" diff : " << diff << endl;
		if (diff >= 0){
			this->STABLE_STATE = this->STATE_NOW;
			//this->TIME_START = now.tv_sec;
			//this->STABLE_TIME_STAMP = this->TIME_START;
		}
	}
	STATE_BEFORE = this->STATE_NOW;
	return this->STABLE_STATE;
}

}

