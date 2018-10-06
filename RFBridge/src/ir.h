#include "globals.h"
#include <IRremote.h>

IRrecv irrecv(IR_RX_PIN);
IRsend irsend;

IRCode codeToSend;

MyMessage msgIrReceive(CID_IR, V_IR_RECEIVE);

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
            char irStr[25];
            snprintf(irStr, 25, "%02X,%08lX,%02X,%04X,%02X", (uint8_t)ircode.decode_type, ircode.value, ircode.bits, ircode.address, 1);
            send(msgIrReceive.set(irStr));
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