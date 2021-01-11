
#include "Brain.hpp"
#include "Logger.hpp"


Brain::Brain(Configuration& config, StateMachine& state, RocketData& rocket, SensorValues& sensors) : config(config), state(state), rocket(rocket), sensors(sensors) {
	
}
void Brain::check() {
	bool stateChange = state.getCurrentState() == lastState;
	hangleStateChange();
	bool pyroFired;
	switch(state.getCurrentState()) {
	case UNARMED:
		//TODO impliment arm checker(either sensor values read or vertical)
		break;
	case READY:
		if(motorIgnition()) {
			//IGNITION
			state.changeState(STAGE1POWERED);
		}
		else {
			//TODO Watch for disarming 
		}
		break;
	case STAGE1POWERED:
		if(motorCutoff()) {
			state.changeState(STAGE1COAST);
		}
		break;
	case STAGE1COAST:
		pyroFired = checkPyros();
		checkApogee();
		if(config.getTwoStageRocket() ) {
			if(motorIgnition()) {
				state.changeState(STAGE1COAST);	
			}
		}
		else {
			if(pyroFired) {
				state.changeState(DROGUEPAR);
			}
		}
		break;
	case STAGE2POWERED:
		if(motorCutoff()) {
			state.changeState(STAGE2COAST);
		}
		checkPyros();
		break;
	case STAGE2COAST:
		pyroFired = checkPyros();
		if(pyroFired) {
			state.changeState(DROGUEPAR);
		}
		checkApogee();
		break;
	case DROGUEPAR:
		pyroFired = checkPyros();
		checkApogee();
		if(pyroFired) {
			state.changeState(MAINPAR);
		}
	case MAINPAR:
		checkApogee();
		if(checkLanded()) {
			state.changeState(LANDED);
		}
		break;
	case LANDED:
		//write data then switch to reset mode
		break;
	case RESET:
		break;
	}
	lastState = state.getCurrentState();
}
//handles all state changes
void Brain::hangleStateChange() {
	State cState = state.getCurrentState();
	State pState = state.getPreviousState();
	//not all states are here only the states where action is required
	switch(cState) {
	case UNARMED:
		if(pState == READY) {
			//TODO disarm
		}
		break;
	case READY:
		if(pState == UNARMED) {
			//TODO arm
		}
		break;
	case LANDED:
		//TODO copy flash to SD
		break;
	case RESET:
		//TODO reset
		break;
	default:
		break;
	}
}
void Brain::arm() {
	if(state.getCurrentState() == UNARMED) {
		state.changeState(READY);
		//TODO arm
	}
}
void Brain::disarm() {
	if(state.getCurrentState() == READY) {
		state.changeState(UNARMED);
	}
	//TODO disarm
}

bool Brain::motorIgnition() {
	float acceleration = rocket.getAcceleration().magnitude();
	if (acceleration > ignitionThreshold) {
		ignitionCountdown--;
		if(ignitionCountdown) {
			return true;
		}
	}
	else {
		ignitionCountdown = ignitionCountdownStart;
	}
	return false;

}
bool Brain::motorCutoff() {
	float acceleration = rocket.getAcceleration().magnitude();
	if (acceleration < cutoffThreshold) {
		cutoffCountdown--;
		if(cutoffCountdown) {
			return true;
		}
	}
	else {
		cutoffCountdown = cutoffCountdownStart;
	}
	return false;
}


void Brain::checkApogee() {
	float currentAltitude = rocket.getDisplacement().z;
	if( currentAltitude <= pastAltitude ) {
		descentTimeSteps++;
		if(descentTimeSteps >= requiredTimeSteps) {
			postAgogee = true;
		}
	}
	else {
		descentTimeSteps = 0;
	}
}
		
void Brain::updateConfigValues() {
	float stepHz = 1000.0/( (float)(config.getUpperTimeStepms()) );
	requiredTimeSteps = stepHz * APOGEE_DESCENT_DETECT_TIME;
	
	cutoffThreshold = config.getCutoffThreshold();
	ignitionThreshold = config.getIgnitionThreshold();
	
	cutoffCountdownStart = (int)( stepHz * CUTOFF_DETECT_TIME );
	cutoffCountdown = cutoffCountdownStart;
	ignitionCountdownStart = (int)( stepHz * IGNITION_DETECT_TIME );
	ignitionCountdown = ignitionCountdownStart;
	
	landedCountdownStart = (int)( stepHz * LANDED_DETECTION_TIME );
	landedCountdown = landedCountdownStart;
	
	for(int i = 0; i < NUMBER_OF_PYROS; i++) {	
		PyroConfig configOne = config.getPyro(i)->configOne;
		PyroConfig configTwo = config.getPyro(i)->configTwo;
		if( configOne == TIME_DELAY ) {
			delayPyroCharge[i] = (int)((stepHz) * config.getPyro(i)->valueOne);
			counting[i] = false;
		}
		else if( configTwo == TIME_DELAY ) {
			delayPyroCharge[i] = (int)(( stepHz ) * config.getPyro(i)->valueTwo);
			counting[i] = false;
		}
		else {
			delayPyroCharge[i] = 0;
			counting[i] = false;
		}
	}
}

//checks and if neccessary fires pyros
bool Brain::checkPyros() {
	bool fired = false;
	for(int i = 0; i < NUMBER_OF_PYROS; i++) {
		Pyro pyro = *config.getPyro(i);
		bool caseOneMet = checkPyroCase(pyro, 1);
		bool caseTwoMet = checkPyroCase(pyro, 2);
		

		if(caseOneMet && caseTwoMet) {
			//Cases are met, begin the countdown
			counting[i] = true;
		}
		
		if(counting[i] == true) {
			if(delayPyroCharge[i] == 0) {
				char * msg = (char*)malloc(12);
				sprintf(msg, "FIRE PYRO %d", i);
				Logger::Event(msg, 12);
				sensors.firePyro(i); //FIRE THE PYRO
				fired = true;
				free(msg);
			}
			else {
				delayPyroCharge[i]--; //waaaaaaaait
			}
		}
	}
	return fired;	
}

		
bool Brain::checkPyroCase(Pyro pyro, int caseN ) {
	PyroConfig config;
	float value;
	if( caseN == 1) {
		config = pyro.configOne;
		value = pyro.valueOne;
	}
	else {
		config = pyro.configTwo;
		value = pyro.valueTwo;
	}
	float acceleration, altitude;
	switch(config) {
	case VELOCITY_ABOVE:
		//no velocity variable TODO
		break;	
	case VELOCITY_BELOW:
		//no velocity variable TODO
		break;	
	case ACCELERATION_BELOW:
		acceleration = rocket.getAcceleration().magnitude();
		if (acceleration < value ) {
			return true;
		}
		else {
			return false;
		}
	case ACCELERATION_ABOVE:
		altitude = rocket.getDisplacement().magnitude();
		if( altitude > value ) {
			return true;
		} 
		else {
			return false;
		}
	case ALTITUDE_ABOVE:
		altitude = rocket.getDisplacement().z;
		if( altitude > value ) {
			return true;
		} 
		else {
			return false;
		}
	case ALTITUDE_BELOW:
		altitude = rocket.getDisplacement().z;
		if( altitude < value ) {
			return true;
		} 
		else {
			return false;
		}	
	case TIME_DELAY://treat time_delay like none, it is implimented elsewhere
	case NONE:
		return true;
	default:
		return false;
	}
	return false;
}

bool Brain::checkLanded() {
	if(rocket.getDisplacement().z < LANDED_ALTITUDE) {
		landedCountdown--;
		if(landedCountdown < 0) {
			return true;
		}
	}
	else {
		landedCountdown = landedCountdownStart;
	}
	return false;
}
