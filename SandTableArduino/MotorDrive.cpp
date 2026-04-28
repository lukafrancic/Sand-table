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
        // wait for new command
        break;
    case MotorState::idle:
        // wait for resume
        break;
    default:
        break;
    }
}


void MotorDrive::home() {
    this->mState = MotorState::home;
    // set start values for homing
    this->homeSpeedSet = false;
    this->rHomed = false;
    this->phiHomed = false;
    this->isHomed = false;
    this->prevPhiStopVal = 1;
}

void MotorDrive::start() {
    if (this->mState == MotorState::idle) {
        this->mState = MotorState::moveActive;
    }
}

void MotorDrive::stop() {
    if (this->mState == MotorState::moveActive) {
        this->mState = MotorState::idle;
    }
}

void MotorDrive::clear() {
    if (this->mState == MotorState::moveActive || this->mState == MotorState::idle) {
        this->mState = MotorState::moveFinished;
    }
}


void MotorDrive::homeMotor() {
    uint8_t endstopVal = digitalRead(R_SWITCH_PIN);
    uint8_t phistopVal = digitalRead(PHI_SWITCH_PIN);

    if (!this->homeSpeedSet) {
        this->rMotor.setSpeed(HOMING_SPEED);
        this->phiMotor.setSpeed(HOMING_SPEED);
        this->homeSpeedSet = true;
    }

    if (endstopVal) {
        this->rMotor.runSpeed();
    }
    else {
        this->rMotor.setCurrentPosition(R_OFFSET);
        this->rMotor.setSpeed(0);
        this->rMotor.runSpeed();
        this->rHomed = true;
    }

    // detect rising edge and mark it as 0 position
    if (!this->phiHomed) {
        this->phiMotor.runSpeed();
    } 
    if (this->prevPhiStopVal == 0 && phistopVal == 1) {
        this->phiMotor.setSpeed(0);
        this->phiMotor.runSpeed();
        this->phiMotor.setCurrentPosition(0);
        this->phiHomed = true;
    }

    if (this->rHomed && this->phiHomed) {
        this->isHomed = true;
        this->rHomed = false;
        this->phiHomed = false;
        this->homeSpeedSet = false;
        // reset position if you call home during work
        this->position[0] = 0;
        this->position[1] = 0;
        this->rMotor.setMaxSpeed(INIT_SPEED);
        this->phiMotor.setMaxSpeed(INIT_SPEED);
        this->steppers.moveTo(position);

        // run to initial position
        this->mState = MotorState::moveActive;
    }

    prevPhiStopVal = phistopVal;
}


void MotorDrive::runMotors() {
    if (this->steppers.run()) {
        return;
    } else{
        this->mState = MotorState::moveFinished;
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
    this->steppers.moveTo(position);
}


void MotorDrive::updateSpeed(float speed) {
    this->rMotor.setMaxSpeed(speed);
    this->phiMotor.setMaxSpeed(speed);
}


/*Return angle in degrees -> 0...359 deg*/
int MotorDrive::getCurrentAngle() {
    long phi = this->phiMotor.currentPosition();
    phi = phi % PHI_STEPS;
    // we want to operate in angle from 0 to 359
    if (phi < 0) {phi += PHI_STEPS;}
    phi = map(phi, 0, PHI_STEPS-1, 0, 359);

    int angle = static_cast<int>(phi);
    
    return angle;
}
