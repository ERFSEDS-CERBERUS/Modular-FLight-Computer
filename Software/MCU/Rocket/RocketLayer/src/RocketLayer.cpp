#include "RocketLayer.hpp"

RocketLayer::RocketLayer(ARM_CALLBACK, void *armContext,
	      FIRE_CALLBACK, void *fireContext,
	      FLASH_WRITE_CALLBACK, void * writeContext,
	      TRANSMIT_CALLBACK, void *transmitContext,
			 Cartesian startGravity, float groundPressure) :
  sensors{},
  config{},
  data{config, sensors, startGravity},
  stateMachine{config},
brain{config, stateMachine, data, sensors, arm_callback, armContext, fire_callback, fireContext}
{
  sensors.setSPressure(groundPressure);
  Logger::setTransmitCallback(transmit_callback);
  Logger::setFlashWriteCallback(flash_write_callback);
}
RocketLayer::~RocketLayer() {
  
}
		
State RocketLayer::getState() {
  return stateMachine.getCurrentState();
}
void RocketLayer::handleMessage(char * msg, size_t size) {

}
void RocketLayer::updateAcceleration(Cartesian acceleration) {
  sensors.setAcceleration(acceleration);
}
void RocketLayer::updateRotation(Cartesian rotation) {
  sensors.setAngularVelocity(rotation);
}
void RocketLayer::updatePressure(float pressure) {
  sensors.setCPressure(pressure);;
}

void RocketLayer::updatePyroCorrect(bool * pyros) {
  
}

void RocketLayer::update() {
  data.update();
  brain.check();
}
