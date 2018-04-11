/*
 * OsmoseAnlage.cpp
 *
 *  Created on: Feb 17, 2018
 *      Author: marc
 */
#include <iostream>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include "../json/json.h"
#include "wiringPi/wiringPi.h"
#include <inttypes.h>
#include "OsmoseAnlage.h"

namespace std {



OsmoseAnlage::OsmoseAnlage(MessageBus *messageBus ) : MarelabBusNode(messageBus) {
	refillPump				=false;
	osmosePump				=false;
	cleanVent				=false;
	frischVent				=false;

	sensorTop				=false;
	sensorBottom			=false;
	sensorAqua				=false;
	//sensorBottomDbounce 	= 5; 	// sec
	//sensorTopDbounce 		= 5; 	// sec
	//sensorAquaDbounce 		= 5;	// sec
	refillFaillTime   		= 20;	// sec
	osmoseFailTime   		= 9000;	// sec

	//osmoseAutoState 		= aidle ;
	//osmoseManualState 		= moff;
	//osmoseLastAutoState     = aidle;
	osmoseProductionLastState     	= sidle;

	osmoseRuntime			= 0l;
	cleanRuntime            = 0l;
	osmoseRuntimeStart		= 0l;
	cleanRuntimeStart		= 0l;

	runtimeForCleaning      = 10l;

	osmoseRuntimeToClean    = 30l;  // Zyklus Zeit bis erneutes Cleaning notwendig wird
	cycleTime				= 0;
	cycleTimeStart			= 0;


	refillRuntime 			= 0;
	refillStartTime 		= 0;

	osmoseModus 			= modus_manuel;

	osmoseRefillState       = refill_off;

	sendErrorMailRefill 	= false;

	osmoseRefillActiv 		= true;

	//Statistik statistik(messageBus);


}

OsmoseAnlage::~OsmoseAnlage() {

}

void OsmoseAnlage::onNotify(Message message)
{
	if ((message.getMsgFor()=="*")||(message.getMsgFor()=="OSMOSEANLAGE"))
		std::cout << "OSMOSEANLAGE received: " << message.getEvent() << std::endl;
}


string OsmoseAnlage::osmoseStateToString(OsmoseProductionState state){
	string stateString;
	switch(state) {
	    case sidle : 		stateString = "sidle";
	    			 	 	break;
	    case smanual : 		stateString = "smanual";
	    	    	 	 	break;
	    case spclean : 		stateString = "spclean";
	    	    	    	break;
	    case sclean : 		stateString = "sclean";
	    	    	    	break;
	    case sposmose : 	stateString = "sposmose";
	    	    	    	break;
	    case sosmose : 		stateString = "sosmose";
	    	    	    	break;
	    case serror : 		stateString = "serror";
	    	    	    	break;
	    case srclean : 		stateString = "srclean";
	    	    	    	break;
	    case soff : 		stateString = "soff";
	    	    	    	break;
	}
	return stateString;
}

string OsmoseAnlage::refillStateToString(OsmoseRefillState state){
	string stateString;
	switch(state) {
	    case refill_off : 		stateString = "refill_off";
	    			 	 	break;
	    case refill_on : 		stateString = "refill_on";
	    	    	 	 	break;
	    case refill_error : 		stateString = "refill_error";
	    	    	    	break;
	    case refill_manual : 		stateString = "refill_manual";
	    	    	    	break;
	}
	return stateString;
}

int OsmoseAnlage::getSensorState(){
 // State der sesoren
 int sensorState =0;
 if(!sensorAqua)
	 sensorState = sensorState +1;
 if (!sensorTop)
	 sensorState = sensorState +2;
 if (!sensorBottom)
	 sensorState = sensorState +4;
 return sensorState;
}

void OsmoseAnlage::CheckSensoren() {
	unsigned long stable;
	//OsmoseAutoState osmoseState;
	struct timeval now;
	gettimeofday(&now, NULL);
	if (top->UpdateState() == LOW) {
		setSensorTop(true);

	} else {
		setSensorTop(false);

	}
	stable = (now.tv_sec  - top->STABLE_TIME_STAMP);


	if (aqua->UpdateState() == LOW) {
		setSensorAqua(true);

	} else {
		setSensorAqua(false);

	}
	stable = (now.tv_sec  - aqua->STABLE_TIME_STAMP);


	if (bottom->UpdateState() == LOW) {
		setSensorBottom(true);
	} else {
		setSensorBottom(false);
	}
	stable = (now.tv_sec  - bottom->STABLE_TIME_STAMP);
}

OsmoseRefillState OsmoseAnlage::stateMachineRefill() {
	struct timeval now;
	bool reachedRefillTime = true;



	auto logger = spdlog::get("logger");

	Json::Value json;

	//json["command"].Value("setEndRefill");
	//json["time"].Value()


	//Message startRefill("STATISTIK","MESSAGE->stateMachineRefill",NULL);
	//send(startRefill);

	// Schalter REFILL am Gehäuse
	if (this->getIOrefillManuel()->read()== true)
	{
		osmoseRefillState = refill_manual;
		this->IOpumpRefill->write(true);
		return osmoseRefillState;
	}
	else if (osmoseRefillState==refill_manual) {
		osmoseRefillState = refill_off;
		this->IOpumpRefill->write(false);
		return osmoseRefillState;
	}

	if (osmoseRefillActiv){

	if (refillRuntime < refillFaillTime) {
		reachedRefillTime = false;
	}

	if ((osmoseModus == modus_automatik)&&((osmoseProductionLastState == sidle))) {
		gettimeofday(&now, NULL);
		// Sensor im Wasser & Refill Time nicht überschritten
		if ( sensorAqua && !reachedRefillTime) {
			refillRuntime = 0;
			refillStartTime = 0;

			this->setRefillPump(false);

			// STATISTIK ENDE
			 if (osmoseRefillState==refill_on) {
				json["command"] = "setEndRefill";
				uint64_t timeint;
				timeint = (uint64_t)now.tv_sec;
				json["time"]=timeint;

				Message startRefill("STATISTIK","MESSAGE->setEndRefill",json);
				send(startRefill);
				///  this->statistik.setEndRefill(now.tv_sec);
				logger->info("Statistik ende Refill");
				///  this->statistik.UpdateRefill();
			}

			osmoseRefillState = refill_off;

			return osmoseRefillState;
		}
		// Sensor trocken & Refill Time nicht überschritten & der erste Durchlauf
		else if ( !sensorAqua  && !reachedRefillTime && (osmoseRefillState == refill_off)) {
			// Refill
			refillRuntime = 1;
			refillStartTime = now.tv_sec;
			osmoseRefillState = refill_on;
			// STATISTIK START
			/// this->statistik.setStartRefill(now.tv_sec);
			json["command"] = "setStartRefill";
			uint64_t timeint;
			timeint = (uint64_t)now.tv_sec;
			json["time"]=timeint;
			Message startRefill("STATISTIK","MESSAGE->setStartRefill",json);
			send(startRefill);
		//	Message startRefill("statistik",to_string(now.tv_sec));
		//	send(startRefill);

			logger->info("Statistik starte Refill");
			this->setRefillPump(true);
			return osmoseRefillState;
		}
		// Sensor trocken & Refill Time nicht überschritten & wiederholter Durchlauf
		else if ( !sensorAqua  && !reachedRefillTime && (osmoseRefillState == refill_on)) {
			// Refill
			//cout << "REFILL REPEAT ON" << endl;
			refillRuntime = now.tv_sec - refillStartTime;
			osmoseRefillState = refill_on;
			this->setRefillPump(true);
			return osmoseRefillState;
		}
		// REFILL HAT LAUFZEIT ÜBERSCHRITTEN
		else if (!sensorAqua  && reachedRefillTime) {
			// Laufzeit zu groß

			osmoseRefillState = refill_error;
			this->setRefillPump(false);

			// STATISTIK STOP RUNTIME ERROR NUR EINMAL AUSFÜHREN
			if (!sendErrorMailRefill){
				/// this->statistik.setEndProductionOsmose(now.tv_sec);
				json["command"] = "setEndProductionOsmose";
				uint64_t timeint;
				timeint = (uint64_t)now.tv_sec;
				json["time"]=timeint;
				Message startRefill("STATISTIK","MESSAGE->setEndProductionOsmose",json);
				send(startRefill);
				logger->error("Statistik ende Refill !!! Laufzeit überschritten !!!");
				/// this->statistik.UpdateRefill();
				sendErrorMailRefill= true; // Wird bei reset auf false gesetzt
			}

			return osmoseRefillState;
		}
		else if ( !sensorAqua && (osmoseRefillState != refill_error)) {

			logger->error("REFILL UNKNOWN ERROR");
			osmoseRefillState = refill_off;
			this->setRefillPump(false);
			return osmoseRefillState;
		}

	}
	else
	{
		refillRuntime = 0;
		refillStartTime = 0;
		osmoseRefillState = refill_off;
		this->setRefillPump(false);
		return osmoseRefillState;
	}
	osmoseRefillState = refill_error;
	}
	return osmoseRefillState;
}

OsmoseProductionState OsmoseAnlage::stateMachineOsmose()
{
  //calc timer status
  bool reachedOsmoseRuntime=false;
  bool reachedCycleTimeClean=false;
  bool reachedCleanRuntime=false;
  OsmoseProductionState calcState;
  struct timeval now;
  Json::Value json;
  auto logger = spdlog::get("logger");
  //logger->debug("stateMachineOsmose check");

	// Schalter OSMOSE
	if (this->getIOsmoseManuel()->read()== true)
	{
		cout << "Schalter Osmose Manual AN" << endl;
		osmoseProductionLastState = smanual;
		this->IOventClean->write(false);
		this->IOventFreshWater->write(true);
		return osmoseProductionLastState;
	}
	else if (osmoseProductionLastState==smanual) {
		cout << "Schalter Osmose Manual AUS" << endl;
		osmoseProductionLastState = sidle;
		this->IOventClean->write(false);
		this->IOventFreshWater->write(false);
		return osmoseProductionLastState;
	}


  if (osmoseModus==modus_automatik)
  {
	  gettimeofday(&now, NULL);

	  if (osmoseRuntime >= osmoseFailTime)
		  reachedOsmoseRuntime = true;
	  if (cycleTime >= osmoseRuntimeToClean)
		  reachedCycleTimeClean = true;
	  if (cleanRuntime >= runtimeForCleaning)
		  reachedCleanRuntime = true;


  // Aktuellen State bestimmen
  //idle->pclean
  if      ((osmoseProductionLastState == sidle)    && !sensorTop  && !sensorBottom &&!reachedOsmoseRuntime&&!reachedCleanRuntime)
	  calcState = spclean;

  //pclean->clean
  else if ((osmoseProductionLastState == spclean)  && !sensorTop  && !sensorBottom &&!reachedOsmoseRuntime&&!reachedCleanRuntime)
  {
	  //Trigger Start eines Osmose Laufes
	  calcState=sclean;
	  /// this->statistik.setStartProductionOsmose(now.tv_sec);
	  json["command"] = "setStartProductionOsmose";
	  uint64_t timeint;
	  timeint = (uint64_t)now.tv_sec;
	  json["time"]=timeint;
	  Message startRefill("STATISTIK","MESSAGE->setStartProductionOsmose",json);
	  send(startRefill);
	  logger->info("Statistik setStartProductionOsmose");
  }


  //clean->posmose
  else if ((osmoseProductionLastState == sclean)   && !sensorTop  && !reachedOsmoseRuntime && reachedCleanRuntime)
  {
	  calcState=sposmose;
  }

  //posmose->osmose
  else if ((osmoseProductionLastState == sposmose) && !sensorTop  && !reachedOsmoseRuntime && !reachedCycleTimeClean)
	  calcState=sosmose;

  //osmose->pclean
  else if ((osmoseProductionLastState == sosmose)  && !sensorTop  && !reachedOsmoseRuntime && reachedCycleTimeClean)
   	  calcState=srclean;

  //xxx->idle
  else if ((osmoseProductionLastState!=serror) && sensorTop   && sensorBottom){
	  // Trigger für Ende eines Osmose Laufes
	  if (osmoseProductionLastState==sosmose) {
		  /// this->statistik.setEndProductionOsmose(now.tv_sec);
		  json["command"] = "setEndProductionOsmose";
		  uint64_t timeint;
		  timeint = (uint64_t)now.tv_sec;
		  json["time"] = timeint;
		  Message startRefill("STATISTIK","MESSAGE->setEndProductionOsmose",json);
		  send(startRefill);
		  logger->info("Statistik setEndProductionOsmose");
		  /// this->statistik.UpdateOsmose();

	  }
	  calcState=sidle;
  }
  // ERROR
  else if ((sensorTop && !sensorBottom)||reachedOsmoseRuntime)
	  calcState= serror;
  else
	  calcState = osmoseProductionLastState;



  if (calcState == sidle){
	  cleanRuntime 		= 0;
	  osmoseRuntime 	= 0;
	  cycleTime	 		= 0;
	  this->setCleanVent(false);
	  this->setFrischVent(false);
	  osmoseProductionLastState   = sidle;
  }
  else if (calcState == spclean){
	  osmoseRuntimeStart = now.tv_sec;
	  osmoseRuntime = 1;
	  cleanRuntime = 1;
	  cleanRuntimeStart = now.tv_sec;
	  this->setCleanVent(true);
	  this->setFrischVent(true);
	  osmoseProductionLastState = spclean;
  }
  else if (calcState == srclean){
  	  cleanRuntime = 1;
  	  cleanRuntimeStart = now.tv_sec;
  	this->setCleanVent(true);
  	this->setFrischVent(true);
  	  osmoseProductionLastState = sclean;
    }
  else if (calcState == sclean){
	  cleanRuntime = now.tv_sec - cleanRuntimeStart;
	  osmoseProductionLastState = sclean;
	  this->setCleanVent(true);
	  this->setFrischVent(true);
  }
  else if (calcState == sposmose){
	  cycleTime = 1;
	  cycleTimeStart = now.tv_sec;
	  osmoseProductionLastState = sposmose;
	  this->setFrischVent(true);
	  this->setCleanVent(false);
  }
  else if (calcState == sosmose){
	  osmoseRuntime = now.tv_sec - osmoseRuntimeStart;
	  cycleTime		= now.tv_sec - cycleTimeStart;
	  osmoseProductionLastState = sosmose;
	  this->setFrischVent(true);
	  this->setCleanVent(false);
  }
  else if (osmoseModus == modus_manuel){
	  osmoseProductionLastState = smanual;
	  return osmoseProductionLastState;
  }
  else{
  	  osmoseProductionLastState = serror;
  	  this->setFrischVent(false);
  	  this->setCleanVent(false);
  }
  }

  // Schalten IO
  if((osmoseProductionLastState == sidle) || (osmoseProductionLastState == serror )){
	  IOpumpRefill->write		(false);
	  IOventFreshWater->write	(false);
	  IOventClean->write		(false);
  }
  else if((osmoseProductionLastState == sclean)||(osmoseProductionLastState == spclean)){
  	  IOpumpRefill->write		(false);
  	  IOventFreshWater->write	(true);
  	  IOventClean->write		(true);
  }
  else if((osmoseProductionLastState == sosmose)||(osmoseProductionLastState == sposmose)){
  	  IOpumpRefill->write		(false);
  	  IOventFreshWater->write	(true);
  	  IOventClean->write		(false);
  }

  return osmoseProductionLastState;
}

void OsmoseAnlage::resetOsmoseProduction(){
	osmoseRuntimeStart = 0;
	osmoseRuntime = 0;
	sendErrorMailRefill = false;
	setOsmoseProductionLastState(sidle);
}

void OsmoseAnlage::resetRefillProduction(){
	refillRuntime = 0;
	refillStartTime = 0;
	setOsmoseRefillState(refill_off);
}

/* JSON Config File -> JavaObject*/
void OsmoseAnlage::Schreiben( Json::Value& root )
{
	Json::Value vOsmoseAnlage;

	vOsmoseAnlage["refillFaillTime"]		= this->refillFaillTime;
	vOsmoseAnlage["osmoseFailTime"]			= this->osmoseFailTime;
	vOsmoseAnlage["runtimeForCleaning"]     = this->runtimeForCleaning;
	vOsmoseAnlage["osmoseModus"]			= this->osmoseModus;
	vOsmoseAnlage["osmoseRuntimeToClean"]   = this->osmoseRuntimeToClean;
	vOsmoseAnlage["emailAktiv"]   			= mailer->isActivmailer();
	vOsmoseAnlage["email"]   				= mailer->getMailadr() ;

	vOsmoseAnlage["SENSOR_IR"]["sensorTop"]["SENSOR_NAME"] 		= this->top->NAME;
	vOsmoseAnlage["SENSOR_IR"]["sensorTop"]["SENSOR_PORT"] 		= this->top->PORT;
	vOsmoseAnlage["SENSOR_IR"]["sensorTop"]["DBOUNCE"] 			= this->top->DBOUNCE_TIME_MS;
	vOsmoseAnlage["SENSOR_IR"]["sensorBottom"]["SENSOR_NAME"] 	= this->bottom->NAME;
	vOsmoseAnlage["SENSOR_IR"]["sensorBottom"]["SENSOR_PORT"] 	= this->bottom->PORT;
	vOsmoseAnlage["SENSOR_IR"]["sensorBottom"]["DBOUNCE"] 		= this->bottom->DBOUNCE_TIME_MS;
	vOsmoseAnlage["SENSOR_IR"]["sensorAqua"]["SENSOR_NAME"] 	= this->aqua->NAME;
	vOsmoseAnlage["SENSOR_IR"]["sensorAqua"]["SENSOR_PORT"] 	= this->aqua->PORT;
	vOsmoseAnlage["SENSOR_IR"]["sensorAqua"]["DBOUNCE"] 		= this->aqua->DBOUNCE_TIME_MS;

	vOsmoseAnlage["DIGI_IO"]["IOsmoseManuel"]["IO_NAME"] 		= this->IOsmoseManuel->NAME;
	vOsmoseAnlage["DIGI_IO"]["IOsmoseManuel"]["IO_PORT"]		= this->IOsmoseManuel->PORT;
	vOsmoseAnlage["DIGI_IO"]["IOsmoseManuel"]["IO_DBOUNCE"]		= this->IOsmoseManuel->DBOUNCE_TIME_MS;
	vOsmoseAnlage["DIGI_IO"]["IOsmoseManuel"]["IO_INPUT"]		= this->IOsmoseManuel->INPUT_IO;

	vOsmoseAnlage["DIGI_IO"]["IOpumpRefill"]["IO_NAME"] 		= this->IOpumpRefill->NAME;
	vOsmoseAnlage["DIGI_IO"]["IOpumpRefill"]["IO_PORT"]			= this->IOpumpRefill->PORT;
	vOsmoseAnlage["DIGI_IO"]["IOpumpRefill"]["IO_DBOUNCE"]		= this->IOpumpRefill->DBOUNCE_TIME_MS;
	vOsmoseAnlage["DIGI_IO"]["IOpumpRefill"]["IO_INPUT"]		= this->IOpumpRefill->INPUT_IO;

	vOsmoseAnlage["DIGI_IO"]["IOrefillManuel"]["IO_NAME"] 		= this->IOrefillManuel->NAME;
	vOsmoseAnlage["DIGI_IO"]["IOrefillManuel"]["IO_PORT"]		= this->IOrefillManuel->PORT;
	vOsmoseAnlage["DIGI_IO"]["IOrefillManuel"]["IO_DBOUNCE"]	= this->IOrefillManuel->DBOUNCE_TIME_MS;
	vOsmoseAnlage["DIGI_IO"]["IOrefillManuel"]["IO_INPUT"]		= this->IOrefillManuel->INPUT_IO;

	vOsmoseAnlage["DIGI_IO"]["IOventClean"]["IO_NAME"] 			= this->IOventClean->NAME;
	vOsmoseAnlage["DIGI_IO"]["IOventClean"]["IO_PORT"]			= this->IOventClean->PORT;
	vOsmoseAnlage["DIGI_IO"]["IOventClean"]["IO_DBOUNCE"]		= this->IOventClean->DBOUNCE_TIME_MS;
	vOsmoseAnlage["DIGI_IO"]["IOventClean"]["IO_INPUT"]			= this->IOventClean->INPUT_IO;

	vOsmoseAnlage["DIGI_IO"]["IOventFreshWater"]["IO_NAME"] 	= this->IOventFreshWater->NAME;
	vOsmoseAnlage["DIGI_IO"]["IOventFreshWater"]["IO_PORT"]		= this->IOventFreshWater->PORT;
	vOsmoseAnlage["DIGI_IO"]["IOventFreshWater"]["IO_DBOUNCE"]	= this->IOventFreshWater->DBOUNCE_TIME_MS;
	vOsmoseAnlage["DIGI_IO"]["IOventFreshWater"]["IO_INPUT"]	= this->IOventFreshWater->INPUT_IO;

	cout << "Schreibe config FreshWaterPort "<< this->IOventFreshWater->PORT <<endl;

	root["OsmoseAnlage"]=vOsmoseAnlage;
}

void OsmoseAnlage::Lesen( Json::Value& master )
{
	Json::Value root = master["OsmoseAnlage"];

	this->refillFaillTime		 = root["refillFaillTime"].asInt();
	this->osmoseFailTime		 = root["osmoseFailTime"].asInt();
	this->runtimeForCleaning     = root["runtimeForCleaning"].asInt();
	this->osmoseModus			 = (OsmoseModus)root["osmoseModus"].asInt();
	this->osmoseRuntimeToClean	 = root["osmoseRuntimeToClean"].asInt();
	mailer->setActivmailer		   (root["emailAktiv"].asBool());
	mailer->setMailadr             (root["email"].asCString());


	Json::Value sensorTop,sensorBottom,sensorAqua;

	delete top;
	delete bottom;
	delete aqua;

	sensorTop = root["SENSOR_IR"]["sensorTop"];
	top = new SensorIR(		sensorTop["SENSOR_NAME"].asString(),
							sensorTop["SENSOR_PORT"].asInt(),
							sensorTop["DBOUNCE"].asInt()
						);

	sensorBottom = root["SENSOR_IR"]["sensorBottom"];
	bottom = new SensorIR(	sensorBottom["SENSOR_NAME"].asString(),
							sensorBottom["SENSOR_PORT"].asInt(),
							sensorBottom["DBOUNCE"].asInt()
						);

	sensorAqua= root["SENSOR_IR"]["sensorAqua"];
	aqua = new SensorIR(	sensorAqua["SENSOR_NAME"].asString(),
							sensorAqua["SENSOR_PORT"].asInt(),
							sensorAqua["DBOUNCE"].asInt()
							);

	delete this->IOsmoseManuel;
	delete this->IOpumpRefill;
	delete this->IOrefillManuel;
	delete this->IOventClean;
	delete this->IOventFreshWater;
	root = master["OsmoseAnlage"]["DIGI_IO"];
	//DigiIO(std::string name,std::string port,int dbounceTime,bool input);
	this->IOsmoseManuel = new DigiIO( 	root["IOsmoseManuel"]["IO_NAME"].asString(),
										root["IOsmoseManuel"]["IO_PORT"].asInt(),
										root["IOsmoseManuel"]["IO_DBOUNCE"].asInt(),
										root["IOsmoseManuel"]["IO_INPUT"].asInt()
							  );
	this->IOpumpRefill = new DigiIO( 	root["IOpumpRefill"]["IO_NAME"].asString(),
										root["IOpumpRefill"]["IO_PORT"].asInt(),
										root["IOpumpRefill"]["IO_DBOUNCE"].asInt(),
										root["IOpumpRefill"]["IO_INPUT"].asInt()
								);
	this->IOrefillManuel = new DigiIO( 	root["IOrefillManuel"]["IO_NAME"].asString(),
										root["IOrefillManuel"]["IO_PORT"].asInt(),
										root["IOrefillManuel"]["IO_DBOUNCE"].asInt(),
										root["IOrefillManuel"]["IO_INPUT"].asInt()
								);
	this->IOventClean 	= new DigiIO( 	root["IOventClean"]["IO_NAME"].asString(),
										root["IOventClean"]["IO_PORT"].asInt(),
										root["IOventClean"]["IO_DBOUNCE"].asInt(),
										root["IOventClean"]["IO_INPUT"].asInt()
								);
	this->IOventFreshWater = new DigiIO(root["IOventFreshWater"]["IO_NAME"].asString(),
										root["IOventFreshWater"]["IO_PORT"].asInt(),
										root["IOventFreshWater"]["IO_DBOUNCE"].asInt(),
										root["IOventFreshWater"]["IO_INPUT"].asInt()
								);

	cout << "Lese config FreshWaterPort "<< this->IOventFreshWater->PORT <<endl;

}

string OsmoseAnlage::getConfigAsJsonString(  )
{
	Json::Value vOsmoseAnlage;
	Json::StyledWriter writer;
	string jsonstring;
	vOsmoseAnlage["cmd"]					= "setupdatatoclient";
	vOsmoseAnlage["sensorAquaDbounce"]		= this->aqua->DBOUNCE_TIME_MS;
	vOsmoseAnlage["sensorTopDbounce"]		= this->top->DBOUNCE_TIME_MS;
	vOsmoseAnlage["sensorBottomDbounce"]	= this->bottom->DBOUNCE_TIME_MS;
	vOsmoseAnlage["refillFaillTime"]		= this->refillFaillTime;
	vOsmoseAnlage["osmoseFailTime"]			= this->osmoseFailTime;
	vOsmoseAnlage["runtimeForCleaning"]     = this->runtimeForCleaning;
	vOsmoseAnlage["osmoseModus"]			= this->osmoseModus;
	vOsmoseAnlage["osmoseRuntimeToClean"]   = this->osmoseRuntimeToClean;
	vOsmoseAnlage["IoPinSensorAqua"]		= this->aqua->PORT;
	vOsmoseAnlage["IoPinSensorTop"]			= this->top->PORT;
	vOsmoseAnlage["IoPinSensorBottom"]		= this->bottom->PORT;

	vOsmoseAnlage["IOsmoseManuelPort"]		= this->IOsmoseManuel->PORT;
	vOsmoseAnlage["IOpumpRefillPort"]		= this->IOpumpRefill->PORT;
	vOsmoseAnlage["IOrefillManuelPort"]		= this->IOrefillManuel->PORT;
	vOsmoseAnlage["IOventCleanPort"]		= this->IOventClean->PORT;
	vOsmoseAnlage["IOventFreshWaterPort"]	= this->IOventFreshWater->PORT;

	vOsmoseAnlage["emailAktiv"]   			= mailer->isActivmailer();
	vOsmoseAnlage["email"]   				= mailer->getMailadr() ;

    jsonstring = writer.write(vOsmoseAnlage);
	return   jsonstring;
}

} /* namespace std */
