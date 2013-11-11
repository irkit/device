#include "Arduino.h"
#include "pins.h"
#include "IrCtrl.h"
#include "pgmStrToRAM.h"
#include "MemoryFree.h"

void setup() {
    // pull-up
    pinMode(IR_IN,      INPUT);
    digitalWrite(IR_IN, HIGH);

    // output
    pinMode(IR_OUT,     OUTPUT);
    digitalWrite(IR_OUT, LOW);

    // disable 3.3V
    pinMode( LDO33_ENABLE, OUTPUT );
    digitalWrite( LDO33_ENABLE, LOW );

    IR_initialize();
    IR_state(IR_IDLE);

    // USB serial
    Serial.begin(9600);

    // wait for connection
    while ( ! Serial ) ;

    printGuide();
}

void printGuide() {
    Serial.println(P("Operations Menu:"));
    Serial.println(P("d) Dump IrCtrl data"));
    Serial.println(P("h) Help (this)"));
    Serial.println(P("s) Send"));
    Serial.println(P("Command?"));
}

void ir_recv_loop(void) {
    static uint8_t last_state;
    if ((last_state == IR_XMITTING) &&
        (IrCtrl.state == IR_IDLE) ) {
        Serial.println(P("xmit finished"));
    }
    last_state = IrCtrl.state;

    if ( IRDidRecvTimeout() ) {
        Serial.println(P("!!!\tIR recv timeout"));
        IR_state(IR_IDLE);
        return;
    }
    if (IrCtrl.state != IR_RECVED) {
        return;
    }
    /* if (IrCtrl.len < VALID_IR_LEN_MIN) { */
    /*     // data is too short = should be noise */
    /*     IR_state(IR_IDLE); */
    /*     return; */
    /* } */

    // can't receive here
    Serial.print(P("overflowed: "));  Serial.println( IrCtrl.overflowed );
    Serial.print(P("received len:")); Serial.println( IrCtrl.len, HEX );

    // start receiving again while leaving received data readable from central
    IR_state( IR_RECVED_IDLE );

    IR_dump();

    // if (ble112.current_bond_handle != INVALID_BOND_HANDLE) {
    //     // notify only when connected & authenticated
    //     ble112.writeAttributeUnreadStatus( 1 );
    // }
}

void loop() {
    static uint8_t writeCount = 1;
    static uint8_t lastCharacter = '0';

    // check if received
    ir_recv_loop();

    // check for input from the user
    if (Serial.available()) {

        lastCharacter = Serial.read();
        Serial.print(P("last character: 0x")); Serial.println( lastCharacter, HEX );
        Serial.print(P("free memory:    0x")); Serial.println( freeMemory(), HEX );

        uint8_t status;
        if (lastCharacter == 'd') {
            IR_dump();
        }
        else if (lastCharacter == 'h') {
            printGuide();
        }
        else if (lastCharacter == 's') {
            Serial.println(P("writing"));
            // Apple Remote Play/Pause
            IR_state( IR_WRITING );
            IR_put(0x46E1); IR_put(0x2325); IR_put(0x04CE); IR_put(0x044C);
            IR_put(0x04CC); IR_put(0x0D0B); IR_put(0x04CF); IR_put(0x0D09);
            IR_put(0x049E); IR_put(0x0D3B); IR_put(0x049E); IR_put(0x047C);
            IR_put(0x04CC); IR_put(0x0D0A); IR_put(0x049E); IR_put(0x0D3A);
            IR_put(0x049E); IR_put(0x0D3A); IR_put(0x04CE); IR_put(0x0D0A);
            IR_put(0x049E); IR_put(0x0D3A); IR_put(0x04CD); IR_put(0x0D0C);
            IR_put(0x04CD); IR_put(0x044C); IR_put(0x04CC); IR_put(0x044C);
            IR_put(0x04CB); IR_put(0x044D); IR_put(0x049C); IR_put(0x047C);
            IR_put(0x04CD); IR_put(0x0D19); IR_put(0x049E); IR_put(0x0D3A);
            IR_put(0x04CA); IR_put(0x0451); IR_put(0x049B); IR_put(0x0D3B);
            IR_put(0x049D); IR_put(0x047C); IR_put(0x049B); IR_put(0x047E);
            IR_put(0x04CB); IR_put(0x044E); IR_put(0x04CB); IR_put(0x044D);
            IR_put(0x04CC); IR_put(0x044C); IR_put(0x049B); IR_put(0x047E);
            IR_put(0x049A); IR_put(0x047D); IR_put(0x04C9); IR_put(0x044F);
            IR_put(0x049A); IR_put(0x0D3C); IR_put(0x049C); IR_put(0x0D3B);
            IR_put(0x04CF); IR_put(0x0D08); IR_put(0x049D); IR_put(0x0D3B);
            IR_put(0x049C); IR_put(0x0D3E); IR_put(0x049C); IR_put(0xFFFF);
            IR_put(0x0000); IR_put(0x25DF); IR_put(0x46AD); IR_put(0x1188);
            IR_put(0x049C);

            Serial.println(P("sending"));
            delay(100);
            IR_xmit();
            IR_dump();
        }
    }
}
