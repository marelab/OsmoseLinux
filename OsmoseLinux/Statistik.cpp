/*
 * Statistik.cpp
 *
 *  Created on: Feb 25, 2018
 *      Author: marc
 */

#include "Statistik.h"
#include <fstream>
#include <iostream>
#include "ConfigRegister.h"

extern ConfigRegister  *config;


namespace std {




Statistik::Statistik(MessageBus* messageBus): MarelabBusNode(messageBus)  {
	 struct timeval now;
	 gettimeofday(&now, NULL);
	 this->osmoseLastProductionDate = 0;
	 this->osmoseLastProductionDuration = 0;
	 this->startProductionOsmose = 0;
	 this->endProductionOsmose=0;
	 this->refillAnzahl = 0;
	 this->refillMaxTime = 0;
	 this->dateOfStatistik = now.tv_sec;

	 this->refillLastProductionDuration = 0;
	 this->startRefill=0;
	 this->endRefill =0;

	 // Statistik Queue
	 maxSizeStatistikQueue = 100 ; // Max Anzahl der Statistik Elemente in der Cache Queue
	 filename = "statistik.json";
	 readFile();
}




void Statistik::AddStatistikToQueue(StatistikObject &object){
	    	if (queueStatistik.size() < maxSizeStatistikQueue){
	    		queueStatistik.push(object);
	    	}else{
	    		queueStatistik.pop();
	    		queueStatistik.push(object);
	    	}
	    	writeFile();
}


string Statistik::PrintStatistikQueue() {
	string ausgabe;
	std::stringstream ausstream;
	unsigned long dauer;
	time_t rawtime;
	//StatistikObject *statis_obj;
	Json::Value jStatistikArray,jentry;
	Json::StyledWriter writer;
	queue<StatistikObject> g = queueStatistik;

	while (!g.empty()) {
		jentry["startdatum"] 	= (uint64_t)(g.front().getStartDate());
		jentry["typ"] 			= g.front().getType();
		jentry["duration"] 		= (uint64_t) g.front().getDuration();
		jStatistikArray["statistik"].append	(jentry);
		cout << "JENTRY startdatum"<< jentry["startdatum"] << " UNIT64 " << (uint64_t)(g.front().getStartDate()) << endl;
		g.pop();
	}
	jStatistikArray["cmd"] = "sendStatistik";
	std::string out_string = writer.write(jStatistikArray);
	cout << out_string << '\n';

	return out_string;
}

void Statistik::SendMail(string text,string stati){

    Json::Value json;
    json["command"] = "SendMail";
    json["subject"] = text;
    json["mailtext"] = stati;
    Message mail("MAIL","MESSAGE FROM STATISTIK->SENDMAIL",json);
    send(mail);
}

void Statistik::UpdateOsmose() {
	string aktuelleStatistik;
	auto logger = spdlog::get("logger");
	osmoseLastProductionDate = startProductionOsmose;
	osmoseLastProductionDuration = endProductionOsmose - startProductionOsmose;
	startProductionOsmose = 0;
	endProductionOsmose = 0;
	aktuelleStatistik = GetOsmoseStatistik();
	logger->info(aktuelleStatistik);
	// Add to Statistik Queue
	StatistikObject *statis = new StatistikObject("OsmoseLauf",osmoseLastProductionDate,osmoseLastProductionDuration);
	AddStatistikToQueue(*statis);
	SendMail("Osmose Lauf Ende",aktuelleStatistik);
	PrintStatistikQueue();
}

void Statistik::UpdateRefill() {
	string aktuelleStatistik;
	auto logger = spdlog::get("logger");
	refillLastProductionDate = startRefill;
	refillLastProductionDuration = endRefill - startRefill;
	startRefill= 0;
	endRefill = 0;
	aktuelleStatistik = GetRefillStatistik();
	logger->info(aktuelleStatistik);
	// Add to Statistik Queue
	StatistikObject *statis = new StatistikObject("RefillLauf",refillLastProductionDate,refillLastProductionDuration);
	AddStatistikToQueue(*statis);
	SendMail("Osmose Refill Lauf Ende",aktuelleStatistik);
	PrintStatistikQueue();
}

string Statistik::GetOsmoseStatistik() {
	string statistikString;
	unsigned long durationInMin;
	time_t rawtime = osmoseLastProductionDate;

	std::string sprodDura,sdurationInMin;
	std::stringstream strstream,stream;
	strstream << osmoseLastProductionDuration;
	strstream >> sprodDura;

	durationInMin = (unsigned long)(osmoseLastProductionDuration / 60);
	stream << durationInMin;
	stream >> sdurationInMin;



	statistikString = statistikString + "Osmose Lauf start am:" + std::ctime(&rawtime) + "Dauer (sec/min):" + sprodDura +"/"+ sdurationInMin;
	return statistikString;
}

string Statistik::GetRefillStatistik() {
	string statistikString;
	time_t rawtime = refillLastProductionDate;

	std::string sprodDura;
	std::stringstream strstream,stream;
	strstream << refillLastProductionDuration;
	strstream >> sprodDura;

	statistikString = statistikString + "Refill Lauf start am:" + std::ctime(&rawtime) + "Dauer (sec):" + sprodDura ;
	return statistikString;

}

void Statistik::writeFile(){
		string fileStatistik;
		fileStatistik = config->getPathOsmoseLinux()+filename;
    	std::ofstream out(fileStatistik.c_str());
    	out << PrintStatistikQueue();
    	out.close();
    	cout << "Statistik written" << endl;
}

bool Statistik::readFile(){
	Json::Value rootin;
	Json::Reader reader;

	string fileStatistik;
	fileStatistik = config->getPathOsmoseLinux()+filename;
	ifstream in;
	in.open(fileStatistik.c_str(), std::ifstream::in);
	if (in.fail()) {
		cout << "Error open statistik.json file ...";
		in.close();
		return false;
	}

	std::string input, line;

	// Read File
	while (getline(in, line))
		input += line + "\n";

	if (!reader.parse(input, rootin)){
		cout << "Error parse statistik.json file ...";
		in.close();
		return false;
	}

	for (Json::Value::ArrayIndex i = 0; i != rootin["statistik"].size(); i++){
	 StatistikObject *statis = new StatistikObject(rootin["statistik"][i]["typ"].asString(),rootin["statistik"][i]["startdatum"].asULong(),rootin["statistik"][i]["duration"].asInt());
	 AddStatistikToQueue(*statis);
	}
}

void Statistik::onNotify(Message message)
{
	/*
	command: Befehl
	ParameterName :Wert
	ParemeterName n : Wert n
	*/
	Json::Value json;
	if ((message.getMsgFor()=="*")||(message.getMsgFor()=="STATISTIK")){

		try{
        json = message.getRootJson();
        string command = json["command"].asString();
        std::cout << "Msg       :" << message.getEvent() << std::endl;
        std::cout << "command   :" << command << std::endl;
        std::cout << "time      :" << json["time"].asULong() << std::endl;

        if (command == "setStartRefill"){
        	setStartRefill(json["time"].asULong());
        }
        else if (command == "setEndRefill"){
        	setEndRefill(json["time"].asULong());
        	UpdateRefill();
        }
        else if (command == "setStartProductionOsmose"){
        	setStartProductionOsmose(json["time"].asULong());
        }
        else if (command == "setEndProductionOsmose"){
        	setEndProductionOsmose(json["time"].asULong());
        	UpdateOsmose();
        }

		}catch(...){
			cout << "ERROR NOTIFY STATISTIK" << endl;

		}

        //if (command=="setStartRefill")


        //Message startRefill("OSMOSEANLAGE","MESSAGE FROM STATISTIK->Schau mal OsmoseAnlage",NULL);
        //send(startRefill);
	}

}

} /* namespace std */


