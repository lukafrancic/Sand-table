#pragma once

// #include <stdint.h>
#include <Arduino.h>

enum class MessageType: uint8_t {
    none = 0x60,
    headerA = 0x61, // a
    headerB = 0x62, // b
    position = 0x63, // c
    speed = 0x64, // d
    start = 0x65, // e
    stop = 0x66, // f
    clear = 0x67, // g
    home = 0x68, // h
    confirmRec = 0x69, // i
    failedRec = 0x70, //j
    getRBuffSize = 0x71,
    sendRBuffSize = 0x72,
    bufferFull = 0x73
};

enum class SerialState: uint8_t {
    readHeader,
    readMsg,
    readPosition,
    readSpeed,
    returnMsg
};