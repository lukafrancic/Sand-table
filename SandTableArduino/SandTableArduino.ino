/*
RAMPS 1.4 shield
Geekcreit MEGA -> ATMEGA 2560 R3
*/

#include "SerialCom.h"
#include "MotorDrive.h"
#include <Arduino.h>

#define ANGLE_POS_PIN 5
#define ANGLE_UPDATE_TIME 100 // update every 500 ms

SerialCOM comObj;
MotorDrive motObj;

MessageType curMsg;
long r, phi;
long t1 = 0;
long t2 = 0;
int tempPhi;


void signalPosition();


void setup(){
  // Configure timer 3 to reduce PWM frequency to 120 Hz
  // Timer 3 is used on pin 2,3 and 5
  TCCR3B = (TCCR3B & ~0x07) | 0x04; // prescaler = 256

  Serial.begin(115200);
  
  delay(1000);

  pinMode(R_ENABLE_PIN, OUTPUT);
  pinMode(PHI_ENABLE_PIN, OUTPUT);
  pinMode(R_SWITCH_PIN, INPUT);
  pinMode(5, OUTPUT);
}

void loop(){
  t1 = millis();

  comObj.readSerialInput();
  motObj.runLoop();

  if (comObj.peekLastMsg() != MessageType::none){
    curMsg = comObj.getLastMsg();

    switch (curMsg) {
      case MessageType::home: 
        motObj.home();
        comObj.clearBuffer(); 
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

  if (motObj.mState == MotorState::moveFinished) {
    bool ret = comObj.getPosition(r, phi);
    
    if (comObj.speedUpdate) {
      float speed = comObj.getSpeed();
      motObj.updateSpeed(speed);
    }

    if (ret) {
      motObj.setPosition(r, phi);
    }
  }

  if (t1 - t2 > ANGLE_UPDATE_TIME) {
    signalPosition();
    t2 = t1;
  }
}


void signalPosition() {
  tempPhi = motObj.getCurrentAngle();
  tempPhi = map(tempPhi, 0, 359, 5, 250);

  analogWrite(ANGLE_POS_PIN, tempPhi);
}