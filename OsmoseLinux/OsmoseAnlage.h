/*
 * OsmoseAnlage.h
 *
 *  Created on: Feb 17, 2018
 *      Author: marc
 */

#ifndef OSMOSEANLAGE_H_
#define OSMOSEANLAGE_H_
#include "IJsonSeri.hpp"
#include "SensorIR.h"
#include "DigiIO.h"
#include "Statistik.h"
#include "spdlog/spdlog.h"
#include "MessageBus.h"
#include "mail.h"

extern mail  *mailer;

namespace std {
extern std::shared_ptr<spdlog::logger> logger;

//enum OsmoseAutoState { aidle ,amanual, aclean , aosmose , aerror };

enum OsmoseModus { modus_manuel ,modus_automatik};

enum OsmoseProductionState { sidle ,smanual, spclean, sclean ,sposmose, sosmose , serror,srclean,soff};
enum OsmoseRefillState {refill_off, refill_on,refill_error,refill_manual};

//enum OsmoseManualState { midle ,moff, moclean , mosmose , mrefill , merror };

class OsmoseAnlage : public IJsonSeri, std::MarelabBusNode{
private:
	//SensorIR bottom("BOTTOM",2,2000);
	//SensorIR top("TOP", 7, 2000);
	//SensorIR aqua("AQUA", 0, 2000);

	SensorIR *bottom;
	SensorIR *top;
	SensorIR *aqua;

	DigiIO *IOsmoseManuel;
	DigiIO *IOrefillManuel;
	DigiIO *IOpumpRefill;
	DigiIO *IOventFreshWater;
	DigiIO *IOventClean;


	///Statistik statistik;

	bool 			refillPump;
	bool 			osmosePump;
	bool			cleanVent;
	bool			frischVent;
	bool 			sensorTop;
	bool 			sensorBottom;
	bool 			sensorAqua;
	//unsigned int 	sensorBottomDbounce; 	// sec
	//unsigned int 	sensorTopDbounce; 		// sec
	//unsigned int 	sensorAquaDbounce;		// sec

	unsigned int 	refillFaillTime;		// sec
	unsigned int    osmoseFailTime;			// sec
	unsigned int    runtimeForCleaning;			// Zeitdauer für das Omose Cleaning


	OsmoseProductionState	osmoseProductionState;
	OsmoseProductionState	osmoseProductionLastState;

	OsmoseModus				osmoseModus;
	OsmoseRefillState		osmoseRefillState;
	bool					sendErrorMailRefill;	// Flag das zeigt ob schon error Mail versendet wurde
	//OsmoseManualState       osmoseManualState;
	bool					osmoseRefillActiv;

	unsigned long	refillRuntime;
	unsigned long	refillStartTime;

	unsigned long   osmoseRuntime;				// Laufzeit der Osmose Runde
	unsigned long  	osmoseRuntimeStart;			// Startzeitpunkt der Osmose Runde
	unsigned int    osmoseRuntimeToClean;		// Zeit die Osmose laufen soll bis zur reinigung SETUP


	unsigned long   cleanRuntime;
	unsigned long   cleanRuntimeStart;

	unsigned long   cycleTime;					// Zykluszeit bis neuer Clean lauf notwendig wird
	unsigned long   cycleTimeStart;				// Zykluszeit bis neuer Clean lauf notwendig wird

private:
    void onNotify(Message message);

public:
	OsmoseAnlage(MessageBus *messageBus);
	virtual ~OsmoseAnlage();

	int getSensorState();
	void CheckSensoren();
	//OsmoseAutoState calcOsmoseState();
	OsmoseProductionState stateMachineOsmose();
	void resetOsmoseProduction();
	OsmoseRefillState stateMachineRefill();
	void resetRefillProduction();

	virtual void Schreiben (Json::Value& root);
	virtual void Lesen( Json::Value& root);
	string getConfigAsJsonString();

	string osmoseStateToString(OsmoseProductionState state);
	string refillStateToString(OsmoseRefillState state);




	unsigned int getOsmoseFailTime() const {
		return osmoseFailTime;
	}

	void setOsmoseFailTime(unsigned int osmoseFailTime) {
		this->osmoseFailTime = osmoseFailTime;
	}

	bool isOsmosePump() const {
		return osmosePump;
	}

	void setOsmosePump(bool osmosePump) {
		this->osmosePump = osmosePump;
	}

	unsigned int getRefillFaillTime() const {
		return refillFaillTime;
	}

	void setRefillFaillTime(unsigned int refillFaillTime) {
		this->refillFaillTime = refillFaillTime;
	}

	bool isRefillPump() const {
		return refillPump;
	}

	void setRefillPump(bool refillPump) {
		this->refillPump = refillPump;

		// Schalten IO
		if(refillPump == true){
			IOpumpRefill->write		(true);
			IOventFreshWater->write	(false);
			IOventClean->write		(false);
		}else
		{
			IOpumpRefill->write		(false);
		}

	}

	bool isSensorAqua() const {
		return sensorAqua;
	}

	void setSensorAqua(bool sensorAqua) {
		this->sensorAqua = sensorAqua;
	}

	unsigned int getSensorAquaDbounce() const {
		return aqua->DBOUNCE_TIME_MS ;
	}

	void setSensorAquaDbounce(unsigned int sensorAquaDbounce) {
		aqua->DBOUNCE_TIME_MS = sensorAquaDbounce;
	}

	bool isSensorBottom() const {
		return sensorBottom;
	}

	void setSensorBottom(bool sensorBottom) {
		this->sensorBottom = sensorBottom;
	}

	unsigned int getSensorBottomDbounce() const {
		return bottom->DBOUNCE_TIME_MS;
	}

	void setSensorBottomDbounce(unsigned int sensorBottomDbounce) {
		bottom->DBOUNCE_TIME_MS = sensorBottomDbounce;
	}

	bool isSensorTop() const {
		return sensorTop;
	}

	void setSensorTop(bool sensorTop) {
		this->sensorTop = sensorTop;
	}

	unsigned int getSensorTopDbounce() const {
		return top->DBOUNCE_TIME_MS;
	}

	void setSensorTopDbounce(unsigned int sensorTopDbounce) {
		top->DBOUNCE_TIME_MS = sensorTopDbounce;
	}




	bool isCleanVent() const {
		return cleanVent;
	}

	void setCleanVent(bool cleanVent) {
		this->cleanVent = cleanVent;
	}

	bool isFrischVent() const {
		return frischVent;
	}

	void setFrischVent(bool frischVent) {
		this->frischVent = frischVent;
	}

	/*

	OsmoseAutoState getOsmoseAutoState() const {
		return osmoseAutoState;
	}

	void setOsmoseAutoState(OsmoseAutoState osmoseAutoState) {
		this->osmoseAutoState = osmoseAutoState;
	}

	OsmoseManualState getOsmoseManualState() const {
		return osmoseManualState;
	}

	void setOsmoseManualState(OsmoseManualState osmoseManualState) {
		this->osmoseManualState = osmoseManualState;
	}
	*/

	unsigned long getCleanRuntime() const {
		return cleanRuntime;
	}

	void setCleanRuntime(unsigned long cleanRuntime) {
		this->cleanRuntime = cleanRuntime;
	}

	unsigned long getCleanRuntimeStart() const {
		return cleanRuntimeStart;
	}

	void setCleanRuntimeStart(unsigned long cleanRuntimeStart) {
		this->cleanRuntimeStart = cleanRuntimeStart;
	}

	unsigned long getOsmoseRuntime() const {
		return osmoseRuntime;
	}

	void setOsmoseRuntime(unsigned long osmoseRuntime) {
		this->osmoseRuntime = osmoseRuntime;
	}

	unsigned long getOsmoseRuntimeStart() const {
		return osmoseRuntimeStart;
	}

	void setOsmoseRuntimeStart(unsigned long osmoseRuntimeStart) {
		this->osmoseRuntimeStart = osmoseRuntimeStart;
	}

	unsigned long getRuntimeForCleaning() const {
		return runtimeForCleaning;
	}

	void setRuntimeForCleaning(unsigned long runtimeForCleaning) {
		this->runtimeForCleaning = runtimeForCleaning;
	}

	unsigned long getOsmoseRuntimeToClean() const {
		return osmoseRuntimeToClean;
	}

	void setOsmoseRuntimeToClean(unsigned long osmoseRuntimeToClean) {
		this->osmoseRuntimeToClean = osmoseRuntimeToClean;
	}

	unsigned long getCycleTimeStart() const {
		return cycleTimeStart;
	}

	void setCycleTimeStart(unsigned long cycleTimeStart) {
		this->cycleTimeStart = cycleTimeStart;
	}

	unsigned long getCycleTime() const {
		return cycleTime;
	}

	void setCycleTime(unsigned long cycleTime) {
		this->cycleTime = cycleTime;
	}

	OsmoseModus getOsmoseModus() const {
		return osmoseModus;
	}

	void setOsmoseModus(OsmoseModus osmoseModus) {
		this->osmoseModus = osmoseModus;
	}

	unsigned long getRefillRuntime() const {
		return refillRuntime;
	}

	void setRefillRuntime(unsigned long refillRuntime) {
		this->refillRuntime = refillRuntime;
	}

	OsmoseProductionState getOsmoseLastState() const {
		return osmoseProductionLastState;
	}

	void setOsmoseLastState(OsmoseProductionState osmoseLastState) {
		this->osmoseProductionLastState = osmoseLastState;
	}

	OsmoseProductionState getOsmoseState() const {
		return osmoseProductionState;
	}

	void setOsmoseState(OsmoseProductionState osmoseState) {
		this->osmoseProductionState = osmoseState;
	}

	OsmoseRefillState getRefillState() const {
		return osmoseRefillState;
	}

	void setRefillState(OsmoseRefillState refillState) {
		this->osmoseRefillState = refillState;
	}

	OsmoseProductionState getOsmoseProductionLastState() const {
		return osmoseProductionLastState;
	}

	void setOsmoseProductionLastState(
			OsmoseProductionState osmoseProductionLastState) {
		this->osmoseProductionLastState = osmoseProductionLastState;
	}

	OsmoseRefillState getOsmoseRefillState() const {
		return osmoseRefillState;
	}

	void setOsmoseRefillState(OsmoseRefillState osmoseRefillState) {
		this->osmoseRefillState = osmoseRefillState;
	}

	DigiIO* getIOpumpRefill() {
		return IOpumpRefill;
	}

	DigiIO* getIOrefillManuel()  {
		return IOrefillManuel;
	}

	DigiIO* getIOsmoseManuel() {
		return IOsmoseManuel;
	}

	DigiIO* getIOventClean() {
		return IOventClean;
	}

	DigiIO* getIOventFreshWater() {
		return IOventFreshWater;
	}

	SensorIR* getAqua() {
		return aqua;
	}

	void setAqua(SensorIR* aqua) {
		this->aqua = aqua;
	}

	SensorIR* getBottom() {
		return bottom;
	}

	void setBottom(SensorIR* bottom) {
		this->bottom = bottom;
	}

	SensorIR* getTop() const {
		return top;
	}

	void setTop(SensorIR* top) {
		this->top = top;
	}

	//const Statistik& getStatistik() const {
	//	return statistik;
	//}

	bool isOsmoseRefillActiv() const {
		return osmoseRefillActiv;
	}

	void setOsmoseRefillActiv(bool osmoseRefillActiv) {
		this->osmoseRefillActiv = osmoseRefillActiv;
	}
};

} /* namespace std */

#endif /* OSMOSEANLAGE_H_ */
