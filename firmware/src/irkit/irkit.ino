#include "Arduino.h"
#include <SoftwareSerial.h>
#include "pins.h"
#include "MemoryFree.h"
#include "pgmStrToRAM.h"
#include "BLE112.h"
#include "IrCtrl.h"

// iMote git:8fa00b089894132e3f6906fea1009a4e53ce5834
SoftwareSerial ble112uart( BLE112_RX, BLE112_TX );
BLE112 ble112( (HardwareSerial *)&ble112uart );

void setup() {
    pinMode(BUSY_LED,      OUTPUT);
    digitalWrite(BUSY_LED, LOW);

    pinMode(IR_OUT,        OUTPUT);

    // pull-up
    pinMode(IR_IN,         INPUT);
    digitalWrite(IR_IN,    HIGH);

    // USB serial
    Serial.begin(115200);

    ble112.setup();

    // set the data rate for the SoftwareSerial port
    ble112uart.begin(38400);

    IR_initialize();
}

void ir_recv_loop(void)
{
    /* static uint16_t counter = 0; */
    /* if ( counter ++ == 0 ) { */
    /*     Serial.print(P("state:")); Serial.println(IrCtrl.state, HEX); */
    /*     Serial.print(P("len:"));   Serial.println(IrCtrl.len, HEX); */
    /*     Serial.print(P("txIndex:")); Serial.println(IrCtrl.txIndex, HEX); */
    /*     Serial.print(P("buff:")); */
    /*     for (uint16_t i=0; i<IrCtrl.len; i++) { */
    /*         Serial.print(P(" ")); */
    /*         Serial.print(IrCtrl.buff[i], HEX); */
    /*     } */
    /*     Serial.println(); */
    /* } */

    if (IrCtrl.state != IR_RECVED){
        return;
    }

    Serial.print(P("received len:")); Serial.println(IrCtrl.len,HEX);
    for (uint16_t i=0; i<IrCtrl.len; i++) {
        Serial.print(IrCtrl.buff[i], HEX);
        Serial.print(P(" "));
    }
    Serial.println();

    IR_state( IR_IDLE ); /* Ready to receive next frame */
}

void loop() {
    static uint8_t writeCount = 1;
    static uint8_t lastCharacter = '0';

    Serial.println(P("Operations Menu:"));
    Serial.println(P("0) Reset BLE112 module"));
    Serial.println(P("1) Hello"));
    Serial.println(P("2) Set gap mode(2,2)"));
    Serial.println(P("3) Get rssi"));
    Serial.println(P("4) Write attribute"));
    Serial.println(P("5) Read attribute"));
    Serial.println(P("6) Disconnect"));
    Serial.println(P("a) Encrypt Start"));
    Serial.println(P("b) Get Bonds"));
    Serial.println(P("c) Passkey Entry"));
    Serial.println(P("d) Set Bondable Mode"));
    Serial.println(P("e) Set Oob Data"));
    Serial.println(P("f) Set Parameters"));
    Serial.println(P("Command?"));
    while (1) {
        // keep polling for new data from BLE
        ble112.loop();

        // check if received
        ir_recv_loop();

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
                Serial.println(P("-->\tsystem_reset: { boot_in_dfu: 0 }"));

                ble112.reset();
            }
            else if (lastCharacter == '1') {
                // Say hello to the BLE112 and wait for response
                Serial.println(P("-->\tsystem_hello"));

                ble112.hello();
            }
            else if (lastCharacter == '2') {
                Serial.println(P("-->\tgap_set_mode: { discover: 0x2, connect: 0x2 }"));

                ble112.setMode();
            }
            else if (lastCharacter == '3') {
                Serial.println(P("-->\tconnection_get_rssi"));

                ble112.getRSSI();
            }
            else if (lastCharacter == '4') {
                ble112.writeAttribute();
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
            else if (lastCharacter == 'd') {
                ble112.setBondableMode();
            }
            else if (lastCharacter == 'e') {
                ble112.setOobData();
            }
            else if (lastCharacter == 'f') {
                ble112.setParameters();
            }
        }
    }

    // AirCon Off -> OK
    // if ( IR_xmit(AEHA, (uint8_t*)"\x14\x63\x00\x10\x10\x02\xFD", 7*8) ) {
    // AirCon On -> OK
    // if ( IR_xmit(AEHA, (uint8_t*)"\x14\x63\x00\x10\x10\xFE\x09\x30\xC1\x04\x50\x00\x00\x00\x28\x93", 16*8) ) {
    /*     Serial.println( "." ); */
    /* } */
}
