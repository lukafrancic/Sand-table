#pragma once

#include <Arduino.h>
#include <AccelStepper.h>
#include <MultiStepper.h>

constexpr uint8_t MOTOR_INTERFACE_TYPE = 1;

constexpr uint8_t R_STEP_PIN = 54;
constexpr uint8_t R_DIR_PIN = 55;
constexpr uint8_t R_ENABLE_PIN = 38;
constexpr uint8_t R_SWITCH_PIN = 3;
constexpr int32_t R_MIN_STEP_LIMIT = 0;
constexpr int32_t R_MAX_STEP_LIMIT = 20700;
constexpr float HOMING_SPEED = -600;
constexpr float INIT_SPEED = 800;
constexpr long R_OFFSET = -300;

constexpr uint8_t PHI_STEP_PIN = 60;
constexpr uint8_t PHI_DIR_PIN = 61;
constexpr uint8_t PHI_ENABLE_PIN = 56;

enum class MotorState {
    home,
    moveActive,
    idle,
    moveFinished
};

class MotorDrive {
    public:
        MotorDrive();
        void runLoop();
        void home();
        void start();
        void stop();
        void clear();
        void setPosition(const long& r, const long& phi);
        void updateSpeed(float speed);
        bool isHomed = false;
        bool moveFinished;
        bool moveActive;

    private:
        AccelStepper rMotor;
        AccelStepper phiMotor;
        MultiStepper steppers;
        MotorState mState = MotorState::home;
        long position[2] = {0,0};
        bool homeSpeedSet = false;

        void homeMotor();
        void runMotors();
};