#include "globals.h"
#include <RCSwitch.h>

RCSwitch rf = RCSwitch();

MyMessage msgRfReceive(CID_RF, V_IR_RECEIVE); 

void pollRFCode() {
    if(rf.available()) {
        uint8_t protocol = rf.getReceivedProtocol();
        uint32_t code = rf.getReceivedValue();
        uint8_t numBits = rf.getReceivedBitlength();
        uint16_t pulseLength = rf.getReceivedDelay();

        // publish received RF Code to MySensors
        char rfStr[20];
        snprintf(rfStr, sizeof(rfStr), "%02X,%08lX,%02X,%04X", protocol, code, numBits, pulseLength);
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

    pinMode(RF_TX_ENABLE_PIN, OUTPUT);
    digitalWrite(RF_TX_ENABLE_PIN, LOW);
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
        uint16_t pulseLength = 0;

        const char* input = message.getString();

        // decode message
        sscanf(input, "%hhX,%lX,%hhX,%X", &protocol, &code, &numBits, &pulseLength);

        char buf[24];
        snprintf(buf, 24, "%02X,%08lX,%02X,%04X", protocol, code, numBits, pulseLength);
        Serial.println("Sending code " + String(buf));

        rf.setProtocol(protocol);
        if(pulseLength != 0)
            rf.setPulseLength(pulseLength);

        // disable receiver, so interrupts don't mess with the timing
        rf.disableReceive();
        // enable 12V boost converter
        digitalWrite(RF_TX_ENABLE_PIN, HIGH);
        // actually send the code
        rf.send(code, numBits);
        // shut down boost converter again
        digitalWrite(RF_TX_ENABLE_PIN, LOW);
        // re-enable receiver
        rf.enableReceive(digitalPinToInterrupt(RF_RX_PIN));
    }
}