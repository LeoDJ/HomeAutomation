#include "globals.h"
#include <RCSwitch.h>

RCSwitch rf = RCSwitch();

MyMessage msgRfReceive(CID_RF, V_IR_RECEIVE); 

void pollRFCode() {
    if(rf.available()) {
        uint8_t protocol = rf.getReceivedProtocol();
        uint32_t code = rf.getReceivedValue();
        uint8_t numBits = rf.getReceivedBitlength();

        // publish received RF Code to MySensors
        char rfStr[20];
        snprintf(rfStr, sizeof(rfStr), "%02X,%08lX,%02X", protocol, code, numBits);
        send(msgRfReceive.set(rfStr));

        rf.resetAvailable();
    }
}

void rfSetup() {
    #ifdef RF_RX_PIN
        rf.enableReceive(digitalPinToInterrupt(RF_RX_PIN));
    #endif

    #ifdef RF_TX_PIN
        rf.enableTransmit(RF_TX_PIN);
    #endif
}

void rfLoop() {
    pollRFCode();
}

//received IR code from MySensors to send 
void rfReceive(const MyMessage &message) {
    // use MySensors IR_SEND datatype for RF codes too, because no better is available
    if (message.type == V_IR_SEND && message.sensor == CID_RF) {
        uint8_t protocol;
        uint32_t code;
        uint8_t numBits;

        const char* input = message.getString();

        // decode message
        sscanf(input, "%hhX,%lX,%hhX", &protocol, &code, &numBits);

        rf.setProtocol(protocol);

        // disable receiver, so interrupts don't mess with the timing
        rf.disableReceive();

        rf.send(code, numBits);

        // re-enable receiver
        rf.enableReceive(digitalPinToInterrupt(RF_RX_PIN));
    }
}