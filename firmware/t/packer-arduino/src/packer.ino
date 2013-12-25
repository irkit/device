#include "Arduino.h"
#include "IrPacker.h"

void setup() {
    Serial.begin(115200);

    while ( ! Serial ) ; // wait for leonardo

    int8_t result = irpacker_save( (void*) 169 );
    if (result == 0) {
        Serial.println("saved!");
    }
    else {
        Serial.println("not saved :(");
    }
}

void loop() {
}
