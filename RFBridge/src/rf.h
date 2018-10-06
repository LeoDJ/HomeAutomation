#include "globals.h"
#include <RCSwitch.h>

RCSwitch rf = RCSwitch();

void rfSetup() {
    #ifdef RF_RX_PIN
        rf.enableReceive(digitalPinToInterrupt(RF_RX_PIN));
    #endif

    #ifdef RF_TX_PIN
        rf.enableTransmit(RF_TX_PIN);
    #endif
}

void rfLoop() {

}

void rfReceive(const MyMessage &message) {

}