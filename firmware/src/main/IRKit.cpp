#include <SoftwareSerial.h>
#include "pins.h"
#include "MemoryFree.h"
#include "pgmStrToRAM.h"
#include "IrCtrl.h"
#include "FullColorLed.h"
#include "version.h"

#define LED_BLINK_INTERVAL 200

// down -1- up -2- down -3- up -4- down -5- up
#define VALID_IR_LEN_MIN   5

SoftwareSerial wifiUART( WIFI_RX, WIFI_TX );
// BLE112 ble112( (HardwareSerial *)&ble112uart, BLE112_RESET );
FullColorLed color( FULLCOLOR_LED_R, FULLCOLOR_LED_G, FULLCOLOR_LED_B );

// bool isAuthenticated(uint8 bond_handle) {
//     if (bond_handle == INVALID_BOND_HANDLE) {
//         return 0;
//     }
//     return authenticatedBondHandles.IsMember(bond_handle);
// }

// void didAuthenticate() {
//     Serial.print(P("didAuthenticate bond: ")); Serial.println(ble112.current_bond_handle);

//     authenticatedBondHandles.Add( ble112.current_bond_handle );
//     authenticatedBondHandles.Save();

//     color.SetLedColor( 0, 0, 1 );
// }

// void didTimeout() {
//     Serial.println(P("!!!\tTimeout occurred!"));
//     color.SetLedColor( 1, 0, 0 );
// }

// void didConnect() {
//     Serial.println(P("!!!\tConnected!"));

//     if (isAuthenticated(ble112.current_bond_handle)) {
//         color.SetLedColor( 0, 0, 1 );
//     }
//     else {
//         color.SetLedColor( 1, 1, 0, LED_BLINK_INTERVAL );
//     }
// }

// void didDisconnect() {
//     Serial.println(P("!!!\tDisonnected!"));
//     color.SetLedColor( 0, 1, 0 );
// }

// void beforeIR() {
//     Serial.println(P("!!!\twill xmit IR"));
//     color.SetLedColor( 0, 0, 1, LED_BLINK_INTERVAL );
// }

// void afterIR() {
//     Serial.println(P("!!!\tto IR finished"));
//     color.SetLedColor( 0, 0, 1 );
// }

// void beforeBT() {
//     Serial.println(P("!!!\twill xmit BT"));
//     color.SetLedColor( 0, 1, 1, LED_BLINK_INTERVAL );
// }

// void afterBT() {
//     Serial.println(P("!!!\tto BT finished"));
//     color.SetLedColor( 0, 0, 1 );
// }

// void cleared() {
//     Serial.println(P("authenticated bond cleared"));
// }

void ir_recv_loop(void) {
    if ( IRDidRecvTimeout() ) {
        Serial.println(P("!!!\tIR recv timeout"));
        IR_state(IR_IDLE);
        return;
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

    Serial.print(P("overflowed: "));  Serial.println( IrCtrl.overflowed );
    Serial.print(P("free:"));         Serial.println( freeMemory() );
    Serial.print(P("received len:")); Serial.println(IrCtrl.len,HEX);

    // start receiving again while leaving received data readable from central
    IR_state( IR_RECVED_IDLE );

    // if (ble112.current_bond_handle != INVALID_BOND_HANDLE) {
    //     // notify only when connected & authenticated
    //     ble112.writeAttributeUnreadStatus( 1 );
    // }
}

void printGuide(void) {
    Serial.println(P("Operations Menu:"));
    Serial.println(P("h) Print this guide"));

    // Serial.println(P("1) Hello"));
    // Serial.println(P("2) Start Advertising"));
    // Serial.println(P("3) Get rssi"));
    // Serial.println(P("5) Read attribute"));
    // Serial.println(P("6) Disconnect"));
    // Serial.println(P("7) Encrypt Start"));

    // Serial.println(P("a) Dump auth data"));
    // Serial.println(P("b) Get Bonds"));
    Serial.println(P("i) Dump IrCtrl.buff"));
    Serial.println(P("v) Dump version"));

    // Serial.println(P("c) Clear auth, bonding data"));
    Serial.println(P("d) IrCtrl to IR_IDLE state"));
    // Serial.println(P("s) Software reset BLE112 module"));
    // Serial.println(P("w) Hardware reset BLE112 module"));

    Serial.println(P("Command?"));
}

void IRKit_setup() {
    color.SetLedColor( 0, 1, 0 );

    pinMode(IR_OUT,           OUTPUT);

    // pull-up
    pinMode(IR_IN,            INPUT);
    digitalWrite(IR_IN,       HIGH);

    // authenticatedBondHandles.Setup();
    // if (! authenticatedBondHandles.IsValid()) {
    //     Serial.println(P("!!! EEPROM INVALID, CLEARING !!!"));
    //     authenticatedBondHandles.Clear();
    // }

    // ble112.setup();
    // ble112.isAuthenticatedCallback  = isAuthenticated;
    // ble112.didAuthenticateCallback  = didAuthenticate;
    // ble112.didTimeoutCallback    = didTimeout;
    // ble112.didConnectCallback    = didConnect;
    // ble112.didDisconnectCallback = didDisconnect;
    // ble112.beforeIRCallback      = beforeIR;
    // ble112.afterIRCallback       = afterIR;
    // ble112.beforeBTCallback      = beforeBT;
    // ble112.afterBTCallback       = afterBT;

    // // set the data rate for the SoftwareSerial port
    // ble112uart.begin(38400);

    // ble112.hardwareReset();

    IR_initialize();

    // ble112.startAdvertising();

    printGuide();
}

void IRKit_loop() {
    // keep polling for new data from BLE
    // ble112.loop();

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
        if (lastCharacter == 'h') {
            printGuide();
        }
        // else if (lastCharacter == '1') {
        //     // Say hello to the BLE112 and wait for response
        //     ble112.hello();
        // }
        // else if (lastCharacter == '2') {
        //     ble112.startAdvertising();
        // }
        // else if (lastCharacter == '3') {
        //     ble112.getRSSI();
        // }
        // else if (lastCharacter == '5') {
        //     ble112.readAttribute();
        // }
        // else if (lastCharacter == '6') {
        //     ble112.disconnect();
        // }
        // else if (lastCharacter == '7') {
        //     ble112.encryptStart();
        // }

        // else if (lastCharacter == 'a') {
        //     authenticatedBondHandles.Dump();
        // }
        // else if (lastCharacter == 'b') {
        //     ble112.getBonds();
        // }
        else if (lastCharacter == 'i') {
            IR_dump();
        }
        else if (lastCharacter == 'v') {
            Serial.print(P("version: "));
            Serial.println(version);
        }

        // else if (lastCharacter == 'c') {
        //     ble112.deleteBonding(0);

        //     authenticatedBondHandles.Clear();
        //     Serial.println(P("cleared authenticated bonding"));
        // }
        else if (lastCharacter == 'd') {
            IR_state(IR_IDLE);
        }
        // else if (lastCharacter == 's') {
        //     ble112.softwareReset();
        // }
        // else if (lastCharacter == 'w') {
        //     ble112.hardwareReset();
        // }
    }
}
