#include "Arduino.h"
#include <avr/wdt.h>
#include "pins.h"
#include "GSwifi.h"
#include "const.h"

GSwifi gs(&Serial1X);

void setup() {
    // modify if you want a different password
    gs.saveLimitedAPPassword( "0123456789" );
}

void loop() {
}

void software_reset() {
    wdt_enable(WDTO_15MS);
    while (1) ;
}
