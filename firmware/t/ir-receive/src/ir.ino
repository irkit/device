#include "Arduino.h"
#include "pins.h"
#include "IrCtrl.h"
#include "pgmStrToRAM.h"
#include "MemoryFree.h"
#include "timer.h"
#include "IrPacker.h"

volatile char sharedbuffer[ SHARED_BUFFER_SIZE ];
extern uint16_t tree[TREE_SIZE];

void onReceivedIR() {
    Serial.println(P("received!!"));
    IR_dump();
}

void setup() {
    // USB serial
    Serial.begin(115200);

    // wait for connection
    while ( ! Serial ) ;

    timer_init( &onTimer );
    timer_start( TIMER_INTERVAL );

    // pull-up
    pinMode(IR_IN,      INPUT);
    digitalWrite(IR_IN, HIGH);

    // output
    pinMode(IR_OUT,     OUTPUT);
    digitalWrite(IR_OUT, LOW);

    // disable 3.3V
    pinMode( LDO33_ENABLE, OUTPUT );
    digitalWrite( LDO33_ENABLE, LOW );

    IR_initialize( &onReceivedIR );
    IR_state(IR_IDLE);

    printGuide();
}

// inside ISR, be careful
void onTimer() {
    IR_timer();
}

void printGuide() {
    Serial.println(P("Operations Menu:"));
    Serial.println(P("d) Dump IrCtrl data"));
    Serial.println(P("h) Help (this)"));
    Serial.println(P("s) Send"));
    Serial.println(P("Command?"));
}

void loop() {
    static uint8_t writeCount = 1;
    static uint8_t lastCharacter = '0';

    IR_loop();

    // check for input from the user
    if (Serial.available()) {

        lastCharacter = Serial.read();
        Serial.print(P("last character: 0x")); Serial.println( lastCharacter, HEX );
        Serial.print(P("free memory:    0x")); Serial.println( freeMemory(), HEX );

        uint8_t status;
        if (lastCharacter == 'd') {
            IR_dump();
            Serial.print("t:"); Serial.println(tree[0]); Serial.println(tree[1]);
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
