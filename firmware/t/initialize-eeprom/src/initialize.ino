#include "Arduino.h"
#include <avr/wdt.h>
#include "pins.h"
#include "GSwifi.h"
#include "IrPacker.h"
#include "const.h"
#include "FullColorLed.h"
#include "timer.h"

static FullColorLed color( FULLCOLOR_LED_R, FULLCOLOR_LED_G, FULLCOLOR_LED_B );
static GSwifi gs(&Serial1X);

void setup() {
    //--- initialize timer
    timer_init( on_timer );
    timer_start( TIMER_INTERVAL );

    //--- initialize full color led

    pinMode(FULLCOLOR_LED_R, OUTPUT);
    pinMode(FULLCOLOR_LED_G, OUTPUT);
    pinMode(FULLCOLOR_LED_B, OUTPUT);
    color.setLedColor( 1, 0, 0, false ); // red: started

    // modify if you want a different password
    gs.saveLimitedAPPassword( "0123456789" );

    irpacker_save( (void*) EEPROM_PACKERTREE_OFFSET );

    color.setLedColor( 0, 0, 1, false ); // blue: ready
}

void loop() {
}

void software_reset() {
    wdt_enable(WDTO_15MS);
    while (1) ;
}

void on_timer(){
    color.onTimer();
}
