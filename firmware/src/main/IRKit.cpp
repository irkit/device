#include <SoftwareSerial.h>
#include "pins.h"
#include "MemoryFree.h"
#include "pgmStrToRAM.h"
#include "BLE112.h"
#include "IrCtrl.h"
#include "EEPROMSet.h"
#include "FullColorLed.h"
#include "DebugHelper.h"
#include "version.h"

#define LED_BLINK_INTERVAL 200

// down -1- up -2- down -3- up -4- down -5- up
#define VALID_IR_LEN_MIN   5

SoftwareSerial ble112uart( BLE112_RX, BLE112_TX );
BLE112 ble112( (HardwareSerial *)&ble112uart, BLE112_RESET );
EEPROMSet authorizedBondHandles;
FullColorLed color( FULLCOLOR_LED_R, FULLCOLOR_LED_G, FULLCOLOR_LED_B );

bool isAuthorized(uint8 bond_handle) {
    return authorizedBondHandles.isMember(bond_handle);
}

void didAuthorized() {
    Serial.print(P("didAuthorized bond: ")); Serial.println(ble112.current_bond_handle);
    // ble112 will indicate iOS central device
    ble112.writeAttributeAuthorizationStatus(1);
}

void didTimeout() {
    Serial.println(P("!!!\tTimeout occurred!"));
    color.SetLedColor( 1, 0, 0 );
}

void didConnect() {
    Serial.println(P("!!!\tConnected!"));

    if (isAuthorized(ble112.current_bond_handle)) {
        color.SetLedColor( 0, 0, 1 );
    }
    else {
        color.SetLedColor( 1, 1, 0, LED_BLINK_INTERVAL );
    }
}

void didDisconnect() {
    Serial.println(P("!!!\tDisonnected!"));
    color.SetLedColor( 0, 1, 0 );
}

void beforeIR() {
    Serial.println(P("!!!\twill xmit IR"));
    color.SetLedColor( 0, 0, 1, LED_BLINK_INTERVAL );
}

void afterIR() {
    Serial.println(P("!!!\tto IR finished"));
    color.SetLedColor( 0, 0, 1 );
}

void beforeBT() {
    Serial.println(P("!!!\twill xmit BT"));
    color.SetLedColor( 0, 1, 1, LED_BLINK_INTERVAL );
}

void afterBT() {
    Serial.println(P("!!!\tto BT finished"));
    color.SetLedColor( 0, 0, 1 );
}

void cleared() {
    Serial.println(P("authorized bond cleared"));
}

void ir_recv_loop(void) {
    if ( IRDidRecvTimeout() ) {
        Serial.println(P("!!!\tIR recv timeout"));
        IR_state(IR_IDLE);
    }
    if (IrCtrl.state != IR_RECVED) {
        return;
    }
    if (IrCtrl.len < VALID_IR_LEN_MIN) {
        // data is too short = should be noise
        IR_state(IR_IDLE);
        return;
    }

    // can't receive here

    Serial.print(P("overflowed: ")); Serial.println( IrCtrl.overflowed );
    Serial.print(P("free:")); Serial.println( freeMemory() );
    Serial.print(P("received len:")); Serial.println(IrCtrl.len,HEX);

    // start receiving again while leaving received data readable from central
    IR_state( IR_RECVED_IDLE );

    if (ble112.current_bond_handle != INVALID_BOND_HANDLE) {
        // notify only when connected & authorized
        ble112.writeAttributeUnreadStatus( 1 );
    }
}

void IRKit_setup() {
    color.SetLedColor( 0, 1, 0 );

    pinMode(IR_OUT,           OUTPUT);

    // pull-up
    pinMode(IR_IN,            INPUT);
    digitalWrite(IR_IN,       HIGH);

    authorizedBondHandles.setup();

    ble112.setup();
    ble112.isAuthorizedCallback  = isAuthorized;
    ble112.didTimeoutCallback    = didTimeout;
    ble112.didConnectCallback    = didConnect;
    ble112.didDisconnectCallback = didDisconnect;
    ble112.beforeIRCallback      = beforeIR;
    ble112.afterIRCallback       = afterIR;
    ble112.beforeBTCallback      = beforeBT;
    ble112.afterBTCallback       = afterBT;

    // set the data rate for the SoftwareSerial port
    ble112uart.begin(38400);

    ble112.hardwareReset();

    IR_initialize();

    ble112.startAdvertising();

    Serial.println(P("Operations Menu:"));
    Serial.println(P("1) Hello"));
    Serial.println(P("2) Start Advertising"));
    Serial.println(P("3) Get rssi"));
    Serial.println(P("5) Read attribute"));
    Serial.println(P("6) Disconnect"));
    Serial.println(P("a) Encrypt Start"));
    Serial.println(P("b) Get Bonds"));
    Serial.println(P("s) IrCtrl to IR_IDLE state"));
    Serial.println(P("t) Software reset BLE112 module"));
    Serial.println(P("u) Hardware reset BLE112 module"));

    Serial.println(P("v) Dump version"));
    Serial.println(P("w) Dump bonding"));
    Serial.println(P("x) Dump IrCtrl.buff"));

    Serial.println(P("y) Delete bonding"));
    Serial.println(P("z) Clear Switch Auth saved data"));
    Serial.println(P("Command?"));
}

void IRKit_loop() {
    // keep polling for new data from BLE
    ble112.loop();

    // check if received
    ir_recv_loop();

    // blink
    color.Loop();

    // check for input from the user
    if (Serial.available()) {
        static uint8_t lastCharacter = '0';

        Serial.print(P("free:"));
        Serial.println( freeMemory() );

        lastCharacter = Serial.read();
        Serial.print(P("0x"));
        Serial.println( lastCharacter, HEX );

        uint8_t status;
        if (lastCharacter == '1') {
            // Say hello to the BLE112 and wait for response
            ble112.hello();
        }
        else if (lastCharacter == '2') {
            ble112.startAdvertising();
        }
        else if (lastCharacter == '3') {
            ble112.getRSSI();
        }
        else if (lastCharacter == '5') {
            ble112.readAttribute();
        }
        else if (lastCharacter == '6') {
            ble112.disconnect();
        }
        else if (lastCharacter == 'a') {
            ble112.encryptStart();
        }
        else if (lastCharacter == 'b') {
            ble112.getBonds();
        }
        else if (lastCharacter == 'g') {
            ble112.writeAttributeUnreadStatus( 1 );
        }
        else if (lastCharacter == 's') {
            IR_state(IR_IDLE);
        }
        else if (lastCharacter == 't') {
            ble112.softwareReset();
        }
        else if (lastCharacter == 'u') {
            ble112.hardwareReset();
        }
        else if (lastCharacter == 'v') {
            Serial.print(P("version: "));
            Serial.println(version);
        }
        else if (lastCharacter == 'w') {
            Serial.print(P("authorized bond: count: "));
            Serial.println(authorizedBondHandles.count(), HEX);
            Serial.print(P("{ "));
            for (uint8_t i=0; i<authorizedBondHandles.count(); i++) {
                Serial.print(authorizedBondHandles.data(i));
                Serial.print(P(" "));
            }
            Serial.println(P("}"));
        }
        else if (lastCharacter == 'x') {
            DumpIR(&IrCtrl);
        }
        else if (lastCharacter == 'y') {
            ble112.deleteBonding(0);
        }
        else if (lastCharacter == 'z') {
            Serial.println(P("cleared switch auth data"));
            authorizedBondHandles.clear();
        }
    }
}
