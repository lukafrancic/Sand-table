#include "MotorDrive.h"

MotorDrive::MotorDrive()
    : rMotor(MOTOR_INTERFACE_TYPE, R_STEP_PIN, R_DIR_PIN),
      phiMotor(MOTOR_INTERFACE_TYPE, PHI_STEP_PIN, PHI_DIR_PIN)    
{
    // MultiStepper steppers();
    this->steppers.addStepper(rMotor);
    this->steppers.addStepper(phiMotor);

    digitalWrite(R_ENABLE_PIN, LOW);
    digitalWrite(PHI_ENABLE_PIN, LOW);

    this->rMotor.setMaxSpeed(INIT_SPEED);
    this->phiMotor.setMaxSpeed(INIT_SPEED);
}


void MotorDrive::runLoop() {
    switch (this->mState)
    {
    case MotorState::home:
        this->homeMotor();
        break;
    case MotorState::moveActive:
        this->runMotors();
        break;
    case MotorState::moveFinished:
        //do stuff to update speed and pos
        break;
    case MotorState::idle:
        break;
    default:
        break;
    }
}


void MotorDrive::home() {
    this->mState = MotorState::home;
}

void MotorDrive::start() {
    this->mState = MotorState::moveActive;
    this->moveActive = true;
}

void MotorDrive::stop() {
    this->mState = MotorState::idle;
    this->moveActive = false;
}

void MotorDrive::clear() {
    this->mState = MotorState::idle;
    this->moveFinished = true;
}


void MotorDrive::homeMotor() {
    uint8_t endstopVal = digitalRead(R_SWITCH_PIN);

    if (!this->homeSpeedSet) {
        this->rMotor.setSpeed(HOMING_SPEED);
        this->homeSpeedSet = true;
    }

    if (endstopVal) {
        this->rMotor.runSpeed();
    }
    else {
        this->rMotor.setCurrentPosition(R_OFFSET);
        this->phiMotor.setCurrentPosition(0);
        this->rMotor.setSpeed(0);
        this->rMotor.runSpeed();

        this->isHomed = true;
        this->homeSpeedSet = false;
        this->mState = MotorState::moveActive;
        // position already set to 0
        this->rMotor.setMaxSpeed(INIT_SPEED);
        this->phiMotor.setMaxSpeed(INIT_SPEED);
        this->steppers.moveTo(position);
    }
}


void MotorDrive::runMotors() {
    if (this->steppers.run()) {
        return;
    } else{
        this->mState = MotorState::moveFinished;
        this->moveFinished = true;
    }
}


void MotorDrive::setPosition(const long& r, const long& phi) {
    if (r >= R_MAX_STEP_LIMIT) {
        this->position[0] = R_MAX_STEP_LIMIT;
    } else if (r <= R_MIN_STEP_LIMIT) {
        this->position[0] = R_MIN_STEP_LIMIT;
    } else {
        this->position[0] = r;
    }
    this->position[1] += phi;
    this->mState = MotorState::moveActive;
    this->moveFinished = false;
    this->steppers.moveTo(position);
}


void MotorDrive::updateSpeed(float speed) {
    this->rMotor.setMaxSpeed(speed);
    this->phiMotor.setMaxSpeed(speed);
}