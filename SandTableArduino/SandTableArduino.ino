#include "SerialCom.h"
#include "MotorDrive.h"
#include <Arduino.h>

SerialCOM comObj;
MotorDrive motObj;

MessageType curMsg;
long r, phi;

void setup(){
  Serial.begin(115200);
  
  delay(1000);

  pinMode(R_ENABLE_PIN, OUTPUT);
  pinMode(PHI_ENABLE_PIN, OUTPUT);
  pinMode(R_SWITCH_PIN, INPUT);
}

void loop(){
  comObj.readSerialInput();
  motObj.runLoop();

  if (comObj.peekLastMsg() != MessageType::none){
    curMsg = comObj.getLastMsg();

    switch (curMsg) {
      case MessageType::home: 
        motObj.home(); 
        break;
      case MessageType::start:
        motObj.start();
        break;
      case MessageType::stop:
        motObj.stop();
        break;
      case MessageType::clear:
        motObj.clear();
        comObj.clearBuffer();
        break;
      default:
        break;
    }
  }

  if (motObj.moveFinished && motObj.moveActive) {
    bool ret = comObj.getPosition(r, phi);
    
    if (comObj.speedUpdate) {
      float speed = comObj.getSpeed();
      motObj.updateSpeed(speed);
    }

    if (ret) {
      motObj.setPosition(r, phi);
    }
  }
}

