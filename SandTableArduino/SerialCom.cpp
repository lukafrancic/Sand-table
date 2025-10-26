#include "SerialCom.h"
#include <Arduino.h>

SerialCOM::SerialCOM(){
    writeBuff[0] = static_cast<uint8_t>(MessageType::headerA);
    writeBuff[1] = static_cast<uint8_t>(MessageType::headerB);
    curState = SerialState::readHeader;
    lastMsg = MessageType::headerA;
}

void SerialCOM::readSerialInput() {
    this->available = Serial.available();

    if (this->available == 0) {
        return;
    }
    uint8_t incoming = 0;

    switch (this->curState) {
        case SerialState::readHeader:
            this->readHeaderLogic();
            break;
        case SerialState::readMsg:
            this->readMsgLogic();
            break;
        case SerialState::readPosition:
            this->readPosition();
            break;
        case SerialState::readSpeed:
            this->readSpeed();
            break;
        default:
            break;
    }

}


void SerialCOM::readHeaderLogic() {
    while (Serial.available() > 0){
        this->header[1] = this->header[0];
        this->header[0] = Serial.read();

        if (this->header[0] == static_cast<uint8_t>(MessageType::headerB) &&
            this->header[1] == static_cast<uint8_t>(MessageType::headerA)) {
            
            this->curState = SerialState::readMsg;
            break;
        }
    }
}


void SerialCOM::readMsgLogic() {
    if (Serial.available() == 0) {
        return;
    }

    uint8_t received = Serial.read();
    switch (static_cast<MessageType>(received)){
        case MessageType::position:
            this->lastMsg = static_cast<MessageType>(received);
            this->curState = SerialState::readPosition;
            break;
        case MessageType::speed:
            this->lastMsg = static_cast<MessageType>(received);
            this->curState = SerialState::readSpeed;
            break;
        case MessageType::getRBuffSize:
            this->lastMsg = static_cast<MessageType>(received);
            this->curState = SerialState::readHeader;
            this->writeBuff[3] = this->rBuff.getSize();
            this->sendMsgResponse(MessageType::sendRBuffSize, 4);
            break;
        case MessageType::start:
        case MessageType::stop:
        case MessageType::clear:
        case MessageType::home:
            this->lastMsg = static_cast<MessageType>(received);
            this->curState = SerialState::readHeader;
            this->sendMsgResponse(MessageType::confirmRec);
            break;
        default:
            this->lastMsg = MessageType::failedRec;
            this->sendMsgResponse(MessageType::failedRec);
            this->curState = SerialState::readHeader;
            break;
    }
}


void SerialCOM::readSpeed() {
    if (Serial.available() < 2){
        return;
    }
    uint8_t msb = Serial.read();
    uint8_t lsb = Serial.read();
    uint16_t readSpeed = (uint16_t)(msb << 8) | lsb;
    this->speed = static_cast<float>(readSpeed); 
    this->sendMsgResponse(MessageType::confirmRec);
    this->curState = SerialState::readHeader;
    this->speedUpdate = true;
}


void SerialCOM::readPosition() {

    if (Serial.available() < 8) {
        return;
    }
    
    uint8_t rec[4];
    if (this->rBuff.isFull()) {
        Serial.readBytes(rec, 4);
        Serial.readBytes(rec, 4);
        this->sendMsgResponse(MessageType::bufferFull);
        this->curState = SerialState::readHeader;
        return;
    }

    int numBytes = Serial.readBytes(rec, 4);
    this->posR = ((long)rec[0] << 24) | ((long)rec[1] << 16) 
                | ((long)rec[2] <<8) | (long)rec[3];

    numBytes = Serial.readBytes(rec, 4);
    this->posPhi = ((long)rec[0] << 24) | ((long)rec[1] << 16) 
                | ((long)rec[2] <<8) | (long)rec[3];

    this->rBuff.addItem(this->posR, this->posPhi);

    this->sendMsgResponse(MessageType::confirmRec);
    this->curState = SerialState::readHeader;
}


bool SerialCOM::getPosition(long& r, long& phi) {
    if (this->rBuff.isEmpty()) {
        return false;
    }

    this->rBuff.getItem(r, phi);
    return true;
}


// const v tem primeru pomeni:
// This member function will not modify any member variables 
// (unless they’re marked mutable)
float SerialCOM::getSpeed() const {
    this->speedUpdate = false;
    return this->speed;
}


//TODO adjust for dynamic length to maybe send data back
void SerialCOM::sendMsgResponse(MessageType msg, uint8_t buffSize) {
    
    this->writeBuff[2] = static_cast<uint8_t>(msg);

    Serial.write(this->writeBuff, buffSize);
}

/*Get the last msg. Msg gets cleared after being read.*/
MessageType SerialCOM::peekLastMsg() const{
    return this->lastMsg;
}


/*Get the last msg. Msg gets cleared after being read.*/
MessageType SerialCOM::getLastMsg(){
    MessageType retMsg = this->lastMsg;
    this->lastMsg = MessageType::none;
    return retMsg;
}


void SerialCOM::clearBuffer() {
    this-> rBuff.clear();
}
