#include "Arduino.h"
#include <SoftwareSerial.h>
#include "pins.h"
#include "MemoryFree.h"
#include "pgmStrToRAM.h"
#include "BLE112.h"
#include "IrCtrl.h"
#include "SetSwitch.h"

// iMote git:8fa00b089894132e3f6906fea1009a4e53ce5834
SoftwareSerial ble112uart( BLE112_RX, BLE112_TX );
BLE112 ble112( (HardwareSerial *)&ble112uart );
SetSwitch authorizedBondHandles( AUTH_SWITCH );

void authorized() {
    Serial.print(P("authorized bond: ")); Serial.println(ble112.currentBondHandle);
    // ble112 will indicate iOS central device
    ble112.writeAttributeAuthorizationStatus(1);
}

void cleared() {
    Serial.println(P("authorized bond cleared"));
}

void setup() {
    pinMode(BUSY_LED,         OUTPUT);
    digitalWrite(BUSY_LED,    LOW);

    pinMode(IR_OUT,           OUTPUT);

    // pull-up
    pinMode(IR_IN,            INPUT);
    digitalWrite(IR_IN,       HIGH);

    // pull-up
    pinMode(AUTH_SWITCH,      INPUT);
    digitalWrite(AUTH_SWITCH, HIGH);

    authorizedBondHandles.setup();
    authorizedBondHandles.callback      = authorized;
    authorizedBondHandles.clearCallback = cleared;

    // USB serial
    Serial.begin(115200);

    ble112.setup();

    // set the data rate for the SoftwareSerial port
    ble112uart.begin(38400);

    IR_initialize();

    ble112.startAdvertising();
}

void ir_recv_loop(void) {
    if (IrCtrl.state != IR_RECVED) {
        return;
    }
    IR_state( IR_RECVED_IDLE );

    Serial.print(P("free:")); Serial.println( freeMemory() );
    Serial.print(P("received len:")); Serial.println(IrCtrl.len,HEX);
    for (uint16_t i=0; i<IrCtrl.len; i++) {
        Serial.print(IrCtrl.buff[i], HEX);
        Serial.print(P(" "));
    }
    Serial.println();

    // update received count in advertising packet
    // to let know disconnected central that we have new IR data
    ble112.incrementReceivedCount();
    ble112.updateAdvData();

    Serial.print(P("free:")); Serial.println( freeMemory() );
}

void loop() {
    static uint8_t writeCount = 1;
    static uint8_t lastCharacter = '0';

    Serial.println(P("Operations Menu:"));
    Serial.println(P("0) Reset BLE112 module"));
    Serial.println(P("1) Hello"));
    Serial.println(P("2) Start Advertising"));
    Serial.println(P("3) Get rssi"));
    Serial.println(P("5) Read attribute"));
    Serial.println(P("6) Disconnect"));
    Serial.println(P("a) Encrypt Start"));
    Serial.println(P("b) Get Bonds"));
    Serial.println(P("c) Passkey Entry"));
    Serial.println(P("f) Increment Received Count"));
    Serial.println(P("w) Dump bonding"));
    Serial.println(P("x) Dump IrCtrl.buff"));
    Serial.println(P("y) Delete bonding"));
    Serial.println(P("z) Clear Switch Auth saved data"));
    Serial.println(P("Command?"));
    while (1) {
        // keep polling for new data from BLE
        ble112.loop();

        // check if received
        ir_recv_loop();

        // check for auth switch pressed
        authorizedBondHandles.loop(ble112.currentBondHandle);

        // check for input from the user
        if (Serial.available()) {

            Serial.print(P("free:"));
            Serial.println( freeMemory() );

            lastCharacter = Serial.read();
            Serial.print(P("0x"));
            Serial.println( lastCharacter, HEX );

            uint8_t status;
            if (lastCharacter == '0') {
                // Reset BLE112 module
                ble112.reset();
            }
            else if (lastCharacter == '1') {
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
            else if (lastCharacter == 'f') {
                ble112.incrementReceivedCount();
            }
            else if (lastCharacter == 'g') {
                ble112.writeAttributeUnreadStatus( 1 );
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
                Serial.print(P("IrCtrl .state: ")); Serial.print(IrCtrl.state,HEX);
                Serial.print(P(" .len: "));         Serial.println(IrCtrl.len,HEX);
                Serial.print(P(" .trailerCount: ")); Serial.println(IrCtrl.trailerCount,HEX);
                for (uint16_t i=0; i<IrCtrl.len; i++) {
                    Serial.print(IrCtrl.buff[i], HEX);
                    Serial.print(P(" "));
                }
                Serial.println();
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
}
