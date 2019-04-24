#pragma once
#include <Arduino.h>
#include <IRremote.h>

#define CID_IR 1
#define IR_RX_PIN 5
#define IR_TX_PIN 3 // pin is predefined as 3, do not change

#define CID_RF 2
#define RF_RX_PIN 2 //has to be an interrupt pin
#define RF_TX_PIN 4
#define RF_TX_ENABLE_PIN 6

typedef struct {
    decode_type_t protocol;
    uint32_t code;
    uint8_t numBits = 32;
    uint16_t address = 0;
    uint8_t repetitions = 1;
} IRCode;