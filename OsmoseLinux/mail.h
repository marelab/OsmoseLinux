/*
 * mail.h
 *
 *  Created on: Feb 25, 2018
 *      Author: marc
 */

#ifndef MAIL_H_
#define MAIL_H_
#include <string>
#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <sys/wait.h>
#include "wiringPi/wiringPi.h"
#include "../json/json.h"
#include "ConfigRegister.h"
#include "MessageBus.h"



extern ConfigRegister  *config;

class mail : public MarelabBusNode{
private:
	// TODO add to Config mailadr
	string mailadr;
	// TODO add to Config sendtime
	unsigned long sendtime;
	static const int READEND = 0;
	static const int WRITEEND = 1;

private:
    void onNotify(Message message);
    int sendEmail(string to, string subject, string body);

public:
	mail(MessageBus* messageBus);
	virtual ~mail();
	void SendMail(string subject, string texttosend);

	const string& getMailadr() const {
		return mailadr;
	}

	void setMailadr(const string& mailadr) {
		this->mailadr = mailadr;
	}

	unsigned long getSendtime() const {
		return sendtime;
	}

	void setSendtime(unsigned long sendtime) {
		this->sendtime = sendtime;
	}
};



#endif /* MAIL_H_ */
