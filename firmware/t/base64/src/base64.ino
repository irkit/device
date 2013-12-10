#include "Arduino.h"
#include "base64encoder.h"

void base64encoded( char encoded ) {
    Serial.print( encoded );
}

void setup() {
    Serial.begin(115200);

    // wait for leonardo
    while ( ! Serial );

    Serial.println("started");

    uint8_t input[256];
    for (uint16_t i=0; i<256; i++) {
        input[ i ] = i;
    }

    Serial.print("length: "); Serial.println( base64_length(256) );

    base64_encode( input, 256, &base64encoded );
}

void loop() {
}
