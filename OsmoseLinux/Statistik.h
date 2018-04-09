/*
 * Statistik.h
 *
 *  Created on: Feb 25, 2018
 *      Author: marc
 */

#ifndef STATISTIK_H_
#define STATISTIK_H_
#include <sys/time.h>
#include <unistd.h>
#include <string>
#include <ctime>
#include <sstream>
#include <queue>
#include <iostream>
#include <iomanip>

#include "json/json.h"
#include "spdlog/spdlog.h"
#include "MessageBus.h"
#include "ConfigRegister.h"

extern ConfigRegister  *config;


namespace std {



class StatistikObject{
	string type;
	unsigned long startDate;
	unsigned long duration;
public:
	StatistikObject(string type,unsigned long startDate,unsigned long duration){
		this->type		= type;
		this->startDate = startDate;
		this->duration	= duration;
	}

	unsigned long getDuration() const {
		return duration;
	}

	unsigned long getStartDate() const {
		return startDate;
	}

	const string& getType() const {
		return type;
	}
};


class Statistik : public MarelabBusNode {
private:
	unsigned int refillAnzahl;
	unsigned int refillMaxTime;

	unsigned int  refillLastProductionDuration;
	unsigned long startRefill;
	unsigned long endRefill;
	unsigned long refillLastProductionDate;

	unsigned int  osmoseLastProductionDuration;
	unsigned long startProductionOsmose;
	unsigned long endProductionOsmose;
	unsigned long osmoseLastProductionDate;
	unsigned long dateOfStatistik;
	string 		  filename;


// Statistik Queue
	unsigned int maxSizeStatistikQueue;
	queue <StatistikObject> queueStatistik;
private:
    void onNotify(Message message);

public:
				Statistik(MessageBus *messageBus);

	string 		GetOsmoseStatistik();			// Ausgabe der Statistik als formatierter string
	string 		GetRefillStatistik();			// Ausgabe der Statistik als formatierter string
	void 		UpdateOsmose();
	void 		UpdateRefill();
	void 		writeFile();					// Schreibe Statistik in Datei
	bool 		readFile();

	//Statistik Queue Methoden
	void 		AddStatistikToQueue(StatistikObject &object);
	string 		PrintStatistikQueue();

	void SendMail(string text,string stati);

	unsigned long getDateOfStatistik() const {
		return dateOfStatistik;
	}

	void setDateOfStatistik(unsigned long dateOfStatistik) {
		this->dateOfStatistik = dateOfStatistik;
	}

	void setEndProductionOsmose(unsigned long endProductionOsmose) {
		this->endProductionOsmose = endProductionOsmose;
	}

	void setStartProductionOsmose(unsigned long startProductionOsmose) {
		this->startProductionOsmose = startProductionOsmose;
	}

	void setOsmoseLastProductionDate(unsigned long osmoseLastProductionDate) {
		this->osmoseLastProductionDate = osmoseLastProductionDate;
	}

	unsigned long getEndRefill() const {
		return endRefill;
	}

	void setEndRefill(unsigned long endRefill) {
		this->endRefill = endRefill;
	}



	unsigned long getStartRefill() const {
		return startRefill;
	}

	void setStartRefill(unsigned long startRefill) {
		this->startRefill = startRefill;
	}

	unsigned int getRefillLastProductionDuration() const {
		return refillLastProductionDuration;
	}

	void setRefillLastProductionDuration(
			unsigned int refillLastProductionDuration) {
		this->refillLastProductionDuration = refillLastProductionDuration;
	}


	unsigned int getMaxSizeStatistikQueue() const {
		return maxSizeStatistikQueue;
	}

	void setMaxSizeStatistikQueue(unsigned int maxSizeStatistikQueue) {
		this->maxSizeStatistikQueue = maxSizeStatistikQueue;
	}
};

} /* namespace std */

#endif /* STATISTIK_H_ */
