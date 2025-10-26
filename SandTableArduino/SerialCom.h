#pragma once

#include "MessageTypes.h"

constexpr uint8_t RBUFF_SIZE = 10;
constexpr uint8_t WRITE_BUFF_SIZE = 11;

struct RollingBuffer {
    long bufferR[RBUFF_SIZE];
    long bufferPhi[RBUFF_SIZE];
    uint8_t rIdx = 0;
    uint8_t wIdx = 0;

    bool isEmpty() const {
        return rIdx == wIdx;
    }

    bool isFull() const {
        return ((wIdx + 1) % RBUFF_SIZE) == rIdx;
    }

    void addItem(long r, long phi) {
        bufferR[wIdx] = r;
        bufferPhi[wIdx] = phi;
        wIdx = (wIdx + 1) % RBUFF_SIZE;
    }

    /*Return the number of elements in the buffer*/
    uint8_t getSize() const {
        return (wIdx - rIdx + RBUFF_SIZE) % RBUFF_SIZE;
    }

    void getItem(long& r, long& phi) {
        r = bufferR[rIdx];
        phi = bufferPhi[rIdx];
        rIdx = (rIdx + 1) % RBUFF_SIZE;
    }

    void clear() {
        rIdx = 0;
        wIdx = 0;
    }

};


class SerialCOM {
    public:
        SerialCOM();
        void readSerialInput();
        bool getPosition(long& r, long& phi);
        float getSpeed() const;
        MessageType peekLastMsg() const;
        MessageType getLastMsg();
        void clearBuffer();

        mutable bool speedUpdate = false;


    private:
        RollingBuffer rBuff;
        uint8_t header[2];
        uint8_t writeBuff[WRITE_BUFF_SIZE];
        uint8_t available;
        float speed = 1000;
        long posR;
        long posPhi;
        MessageType lastMsg;
        SerialState curState;

        void readHeaderLogic();
        void readMsgLogic();
        void sendMsgResponse(MessageType msg, uint8_t buffSize= 3);
        void readSpeed();
        void readPosition();
};
