#include <Arduino.h>
#include "pgmStrToRAM.h"
#include "DebugHelper.h"

void DumpIR(volatile IR_STRUCT *ir) {
    Serial.print(P("IR .state: "));      Serial.print(ir->state,HEX);
    Serial.print(P(" .len: "));          Serial.println(ir->len,HEX);
    Serial.print(P(" .trailerCount: ")); Serial.println(ir->trailerCount,HEX);
    Serial.print(P(" .overflowed: "));   Serial.println(ir->overflowed);
    for (uint16_t i=0; i<ir->len; i++) {
        if (ir->buff[i] < 0x1000) { Serial.write('0'); }
        if (ir->buff[i] < 0x0100) { Serial.write('0'); }
        if (ir->buff[i] < 0x0010) { Serial.write('0'); }
        Serial.print(ir->buff[i], HEX);
        Serial.print(P(" "));
        if (i % 16 == 15) { Serial.println(); }
    }
    Serial.println();
}
