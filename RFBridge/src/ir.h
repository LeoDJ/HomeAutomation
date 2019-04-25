#include "globals.h"
#include <IRremote.h>

IRrecv irrecv(IR_RX_PIN);
IRsend irsend;

IRCode codeToSend;

MyMessage msgIrReceive(CID_IR, V_IR_RECEIVE);
MyMessage msgIrReceiveDebug(CID_IR, V_VAR1);

//Mitsubishi and Pronto not implemented
void sendIRCode(decode_type_t protocol, uint32_t code, uint8_t numBits = 32, uint16_t address = 0) {
    bool repeat = false;
    uint8_t protocolId = (uint8_t)protocol;

    switch(protocolId) {
        case RC5: irsend.sendRC5(code, numBits); break;
        case RC6: irsend.sendRC6(code, numBits); break;
        case NEC: irsend.sendNEC(code, numBits); break;
        case SONY: irsend.sendSony(code, numBits); break;
        case PANASONIC: irsend.sendPanasonic(address, code); break;
        case JVC: irsend.sendJVC(code, numBits, repeat); break;
        case SAMSUNG: irsend.sendSAMSUNG(code, numBits); break;
        case WHYNTER: irsend.sendWhynter(code, numBits); break;
        case AIWA_RC_T501: irsend.sendAiwaRCT501(code); break;
        case LG: irsend.sendLG(code, numBits); break;
        case DISH: irsend.sendDISH(code, numBits); break;
        case SHARP: irsend.sendSharp(address, code); break;
        case DENON: irsend.sendDenon(code, numBits); break;
        case LEGO_PF: irsend.sendLegoPowerFunctions(code, repeat); break;
        default: break;
    }
}

void sendIRCode(IRCode code) {
    sendIRCode(code.protocol, code.code, code.numBits, code.address);
}

void printIRCode(IRCode c) {
    char strBuf[25];
    snprintf(strBuf, 25, "%02X,%08lX,%02X,%04X,%02X", (uint8_t)c.protocol, c.code, c.numBits, c.address, c.repetitions);
    Serial.println(strBuf);
}

void pollIRCode() {
    decode_results ircode;
    if (irrecv.decode(&ircode)) {
        if(ircode.decode_type != UNKNOWN && ircode.value != REPEAT) {
            uint16_t addr = ircode.address;
            decode_type_t type = ircode.decode_type;
            if(type != PANASONIC && type != SHARP) {
                addr = 0;   // prevent random values for address on other protocols
            }
            char irStr[25];
            snprintf(irStr, 25, "%02X,%08lX,%02X", (uint8_t)type, ircode.value, ircode.bits);
            send(msgIrReceive.set(irStr));
            snprintf(irStr + strlen(irStr), 25, ",%04X,%02X", addr, 1);
            send(msgIrReceiveDebug.set(irStr));
        }
        irrecv.resume();
    }
}

void irSetup() {
    irrecv.enableIRIn();
}

void irLoop() {
    pollIRCode();
}

// receive IR message from MySensors
void irReceive(const MyMessage &message) {
    if (message.type == V_IR_SEND && message.sensor == CID_IR) {
        IRCode c;
        const char* input = message.getString();
        //decode message
        sscanf(input, "%hhX,%lX,%hhX,%X,%hhX", (uint8_t*)&c.protocol, &c.code, &c.numBits, &c.address, &c.repetitions);
        
        printIRCode(c);

        //TODO: handle repeat commands

        sendIRCode(c);
    }
}