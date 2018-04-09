/*
 * SensorIR.h
 *
 *  Created on: Jan 20, 2018
 *      Author: marc
 */
#include <string>
#include <iostream>

#include <sys/time.h>
#include <unistd.h>

#include <string>
#include "wiringPi/wiringPi.h"
#include "../json/json.h"

#ifndef SENSORIR_H_
#define SENSORIR_H_
namespace std {


class SensorIR {
public:
	SensorIR(std::string name,int port,int dbounceTime);
	virtual ~SensorIR();
	bool UpdateState();
	std::string 		NAME;
	int 				PORT;
	int			  		DBOUNCE_TIME_MS; 	// Zeit die das Signal stabil sein muss

	bool 				STABLE_STATE;	 	// Letzter gelesener IO State (Low/High)
	bool 				STATE_NOW;		 	// Letzter State der per DBOUNCE bestimmt wird
	bool 				STATE_BEFORE;
	unsigned long       STABLE_TIME_STAMP; 	// Zeitpunkt als Wert als Stable angesehen wird
	unsigned long 		TIME_START;         	// ZEITPUNKT der letzten Messung

};
}
#endif /* SENSORIR_H_ */
