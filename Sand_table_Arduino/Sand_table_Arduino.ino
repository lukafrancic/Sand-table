#include <AccelStepper.h>
#include <MultiStepper.h>

#define HEADER_A 0x61
#define HEADER_B 0x62
#define POSITION_MSG 0x63
#define SPEED_MSG 0x64
#define BEGIN_MSG 0x65
#define STOP_MSG 0x66
#define REQUEST_MSG 0x67

#define LEN_POS 8
#define LEN_SPEED 2

#define MOTOR_INTERFACE_TYPE 1

#define R_STEP_PIN 54
#define R_DIR_PIN 55
#define R_ENABLE_PIN 38
#define R_SWITCH_PIN 3
#define R_STEP_LIMIT 20455

#define PHI_STEP_PIN 60
#define PHI_DIR_PIN 61
#define PHI_ENABLE_PIN 56

#define HEADER_SIZE 3
#define BUFFER_SIZE 5
#define HOMING_SPEED -600 // negative so it goes toward the endstop

#define R_START_POSITION 0
#define PHI_START_POSITION 0

#define REQUEST_DATA_TIME_DELAY 250 // request new data n ms


AccelStepper R_motor(MOTOR_INTERFACE_TYPE, R_STEP_PIN, R_DIR_PIN);
AccelStepper PHI_motor(MOTOR_INTERFACE_TYPE, PHI_STEP_PIN, PHI_DIR_PIN);
MultiStepper steppers;

enum class MotorState {HOME, MOVE, STOP};
MotorState m_state = MotorState::HOME;

enum class SerialState {READ_HEADER, BEGIN, STOP,
                        UPDATE_POSITION, UPDATE_SPEED, CLEAR};
SerialState s_state = SerialState::READ_HEADER;

// run the motor and data acq
bool is_active = false;
bool request_free = true;

// buffer index so we can implement a circular buffer
uint8_t buffer_index = 0; // current position to use
uint8_t buffer_fill_index = 0; // index at which to store a new value
uint8_t buffer_storage = 0; // keeps track of current buffer state
long R_buffer[BUFFER_SIZE];
long PHI_buffer[BUFFER_SIZE];

// speed/move control
bool update_speed = false;
float max_speed = 1000.;
long incoming_positon;
long position[2] = {R_START_POSITION, PHI_START_POSITION};
uint16_t incoming_speed;

// serial stuff
byte serial_head[HEADER_SIZE];
uint8_t serial_counter = 0;

// for requesting time limit
unsigned long t1 = 0;
unsigned long t2 = 0;


void home_R();
void read_serial_input();
void run_motor();

void run_steppers();
void serial_header_handling();
void serial_speed_handling();
void serial_position_handling();
void request_data();


void setup() {
  // pins to be defined
  pinMode(R_ENABLE_PIN, OUTPUT);
  pinMode(PHI_ENABLE_PIN, OUTPUT);
  pinMode(R_SWITCH_PIN, INPUT);

  // enable the steppers by setting the enable pins low
  // by setting high they should be disabled
  digitalWrite(R_ENABLE_PIN, LOW);
  digitalWrite(PHI_ENABLE_PIN, LOW);

  // set the stepper speeds
  R_motor.setMaxSpeed(1000); 
  PHI_motor.setMaxSpeed(1000); 

  steppers.addStepper(R_motor);
  steppers.addStepper(PHI_motor);

  // begin Serial
  Serial.begin(115200);

  // for (int i = 0; i < BUFFER_SIZE; i++){
  //   // request_data();
  //   Serial.write(FINISH_MOVE);
  // }
}


void loop() {
  // t1 = millis();

  read_serial_input();

  run_motor();

  if (buffer_storage < BUFFER_SIZE && is_active && request_free) {
    request_data();
  }
  
}


void run_motor(){
  switch (m_state)
  {
  case MotorState::HOME:
    home_R();
    break;

  case MotorState::MOVE:
    run_steppers();
    break;

  case MotorState::STOP:
    break;

  default:
    break;
  }
}


// home the R axis stepper
void home_R() {
  uint8_t R_endstop_value = digitalRead(R_SWITCH_PIN);
  
  if (R_endstop_value) {
    R_motor.setSpeed(HOMING_SPEED);
    R_motor.runSpeed();
  }
  else {
    R_motor.setCurrentPosition(0);
    R_motor.setSpeed(0);
    R_motor.runSpeed();

    m_state = MotorState::STOP;
    steppers.moveTo(position);
  }
}


// run the steppers and update the ring buffer params and signal an update
void run_steppers() {

  if (steppers.run() == 0 && buffer_storage > 0) {
    buffer_storage -= 1;

    position[0] = R_buffer[buffer_index];
    position[1] = PHI_motor.currentPosition() + PHI_buffer[buffer_index];

    // prevent collision to the end point
    if (position[0] > R_STEP_LIMIT){
      position[0] = R_STEP_LIMIT;
    }

    if (update_speed) {
      R_motor.setMaxSpeed(max_speed);
      PHI_motor.setMaxSpeed(max_speed);
      update_speed = false;
    }

    steppers.moveTo(position);

    //posodobimo na koncu, ker zacnemo z buffer_index = 0    
    buffer_index = (buffer_index + 1) % BUFFER_SIZE;    
  }
}


void read_serial_input() {
  if (Serial.available() > 0) {
    switch (s_state)
    {
    case SerialState::READ_HEADER:
      serial_header_handling();
      break;

    case SerialState::UPDATE_SPEED:
      serial_speed_handling();
      break;

    case SerialState::UPDATE_POSITION:
      serial_position_handling();
      break;

    case SerialState::BEGIN:
      is_active = true;
      m_state = MotorState::MOVE;
      s_state = SerialState::READ_HEADER;
      break;

    case SerialState::STOP:
      is_active = false;
      m_state = MotorState::STOP;
      s_state = SerialState::READ_HEADER;
      break;

    default:
      s_state = SerialState::READ_HEADER;
      break;
    }

  }
}


void serial_header_handling() {
  serial_head[0] = serial_head[1];
  serial_head[1] = serial_head[2];
  serial_head[2] = Serial.read();
  if (serial_head[0] == HEADER_A && serial_head[1] == HEADER_B){
    
    switch (serial_head[2])
    {
    case SPEED_MSG:
      s_state = SerialState::UPDATE_SPEED;
      break;
    case POSITION_MSG:
      s_state = SerialState::UPDATE_POSITION;
      break;
    case BEGIN_MSG:
      s_state = SerialState::BEGIN;
      break;
    case STOP_MSG:
      s_state = SerialState::STOP;
      break;
    default:
      s_state = SerialState::READ_HEADER;
      break;
    }
  }
  
  serial_counter = 0;
}


void serial_speed_handling() {
  byte rec = Serial.read();

  if (serial_counter == 0){
    incoming_speed = (uint16_t)(rec << 8);
    serial_counter++;
  }
  else {
    incoming_speed = incoming_speed | rec;
    max_speed = (float)incoming_speed;
    serial_counter = 0;
    update_speed = true;
    s_state = SerialState::READ_HEADER;
  }
}


void serial_position_handling() {
  byte rec = Serial.read();

  switch (serial_counter)
  {
  case 0:
    incoming_positon = (long)(rec << 24);
    break;
  case 1:
    incoming_positon = incoming_positon | (long)(rec << 16);
    break;
  case 2:
    incoming_positon = incoming_positon | (long)(rec << 8);
    break;
  case 3:
    incoming_positon = incoming_positon | rec;
    R_buffer[buffer_fill_index] = incoming_positon;
    break;
  case 4:
    incoming_positon = (long)(rec << 24);
    break;
  case 5:
    incoming_positon = incoming_positon | (long)(rec << 16);
    break;
  case 6:
    incoming_positon = incoming_positon | (long)(rec << 8);
    break;
  case 7:
    incoming_positon = incoming_positon | rec;
    PHI_buffer[buffer_fill_index] = incoming_positon;
    buffer_storage++;
    buffer_fill_index = (buffer_fill_index + 1) % BUFFER_SIZE;
    request_free = true;
    s_state = SerialState::READ_HEADER;
    break;
  default:
    s_state = SerialState::READ_HEADER;
    break;
  }

  serial_counter++;
}


void request_data(){
  request_free = false;  
  Serial.write(HEADER_A);
  Serial.write(HEADER_B);
  Serial.write(REQUEST_MSG);

    // Serial.write(buffer_storage);
    // t2 = t1;
  
}