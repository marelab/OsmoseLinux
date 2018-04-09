/*
 * SensorIR.h
 *
 *  Created on: Jan 20, 2018
 *      Author: marc
 */
#include <string>
#include "spdlog/spdlog.h"

#ifndef DIGIIO_H_
#define DIGIIO_H_

class DigiIO {
public:

	std::string NAME;
	int PORT;
	int DBOUNCE_TIME_MS;
	int INPUT_IO;
	int STABLE_MS;
	bool LAST_STATE;
	bool STATE;

public:
	DigiIO(std::string name,int port,int dbounceTime,int input);
	virtual ~DigiIO();

	int getPort() const {
		return PORT;
	}

	void setPort(int port) {
		PORT = port;
	}

	bool read();
	void write(bool onoff);
};

#endif /* DIGIIO_H_ */
