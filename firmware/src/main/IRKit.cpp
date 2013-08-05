#include <SoftwareSerial.h>
#include "pins.h"
#include "MemoryFree.h"
#include "pgmStrToRAM.h"
#include "BLE112.h"
#include "IrCtrl.h"
#include "SetSwitch.h"
#include "FullColorLed.h"
#include "DebugHelper.h"

SoftwareSerial ble112uart( BLE112_RX, BLE112_TX );
BLE112 ble112( (HardwareSerial *)&ble112uart, BLE112_RESET );
SetSwitch authorizedBondHandles( AUTH_SWITCH );
FullColorLed color( FULLCOLOR_LED_R, FULLCOLOR_LED_G, FULLCOLOR_LED_B );

void didAuthorized() {
    Serial.print(P("didAuthorized bond: ")); Serial.println(ble112.current_bond_handle);
    // ble112 will indicate iOS central device
    ble112.writeAttributeAuthorizationStatus(1);
}

bool isAuthorized(uint8 bond_handle) {
    return authorizedBondHandles.isMember(bond_handle);
}

void cleared() {
    Serial.println(P("authorized bond cleared"));
}

void ir_recv_loop(void) {
    if (IrCtrl.state != IR_RECVED) {
        return;
    }

    // can't receive here

    unsigned long now = millis();
    Serial.print(P("now: ")); Serial.println( now );
    Serial.print(P("overflowed: ")); Serial.println( IrCtrl.overflowed );
    Serial.print(P("free:")); Serial.println( freeMemory() );
    Serial.print(P("received len:")); Serial.println(IrCtrl.len,HEX);

    // start receiving again while leaving received data readable from central
    IR_state( IR_RECVED_IDLE );

    ble112.writeAttributeUnreadStatus( 1 );

    Serial.print(P("free:")); Serial.println( freeMemory() );
}

void IRKit_setup() {
    pinMode(BUSY_LED,         OUTPUT);
    digitalWrite(BUSY_LED,    LOW);

    color.SetLedColor( 0, 1, 0 );

    pinMode(IR_OUT,           OUTPUT);

    // pull-up
    pinMode(IR_IN,            INPUT);
    digitalWrite(IR_IN,       HIGH);

    // pull-up
    pinMode(AUTH_SWITCH,      INPUT);
    digitalWrite(AUTH_SWITCH, HIGH);

    ble112.setup();
    ble112.isAuthorizedCallback = isAuthorized;

    // set the data rate for the SoftwareSerial port
    ble112uart.begin(38400);

    ble112.hardwareReset();

    authorizedBondHandles.setup();
    authorizedBondHandles.callback      = didAuthorized;
    authorizedBondHandles.clearCallback = cleared;

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
    Serial.println(P("c) Passkey Entry"));
    Serial.println(P("u) Software reset BLE112 module"));
    Serial.println(P("v) Hardware reset BLE112 module"));
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

    // check for auth switch pressed
    authorizedBondHandles.loop(ble112.current_bond_handle);

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
        else if (lastCharacter == 'c') {
            ble112.passkeyEntry();
        }
        else if (lastCharacter == 'g') {
            ble112.writeAttributeUnreadStatus( 1 );
        }
        else if (lastCharacter == 'u') {
            ble112.softwareReset();
        }
        else if (lastCharacter == 'v') {
            ble112.hardwareReset();
        }
        else if (lastCharacter == 'w') {
            Serial.print("authorized bond: { ");
            for (uint8_t i=0; i<authorizedBondHandles.count(); i++) {
                Serial.print(authorizedBondHandles.data(i));
                Serial.print(" ");
            }
            Serial.println("}");
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
