#include "Arduino.h"
#include "pins.h"
#include "IrCtrl.h"
#include "MemoryFree.h"
#include "pgmStrToRAM.h"
#include "FullColorLed.h"
#include "timer.h"

volatile char sharedbuffer[ SHARED_BUFFER_SIZE ];
static FullColorLed color( FULLCOLOR_LED_R, FULLCOLOR_LED_G, FULLCOLOR_LED_B );

void on_timer();
void on_ir_receive();
void on_ir_xmit();
void send();

void setup() {
    Serial.begin(115200);

    //--- initialize timer

    timer_init( on_timer );
    timer_start( TIMER_INTERVAL );

    //--- initialize full color led

    pinMode(FULLCOLOR_LED_R, OUTPUT);
    pinMode(FULLCOLOR_LED_G, OUTPUT);
    pinMode(FULLCOLOR_LED_B, OUTPUT);
    color.setLedColor( 1, 0, 0, false ); // red: error

    //--- initialize IR

    pinMode(IR_OUT,           OUTPUT);

    // pull-up
    pinMode(IR_IN,            INPUT);
    digitalWrite(IR_IN,       HIGH);

    IR_initialize( &on_ir_receive );
    IR_state( IR_IDLE );

    color.setLedColor( 1, 1, 0, true ); // yellow blink: sending
}

void loop() {
    IR_loop();

    if (IrCtrl.state == IR_IDLE) {
        send();
    }
}

void send() {
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
    IR_xmit();
}

// inside ISR, be careful
void on_timer() {
    color.onTimer(); // 200msec blink

    IR_timer();
}

void on_ir_receive() {
    Serial.println("<i");
}

void on_ir_xmit() {
    Serial.println("i>");
}

