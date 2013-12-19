#include "Arduino.h"
#include "pgmStrToRAM.h"
#include "MemoryFree.h"
#include "IrPacker.h"

#include <stdarg.h> // for va_list

void setBuffer8( uint8_t *buff, uint16_t num, ... ){
    va_list list;
    int i;

    va_start( list, num );

    for( i = 0; i < num; ++i ){
        buff[i] = (uint8_t)va_arg( list, int );
    }

    va_end( list );
}

void setup() {
    while ( ! Serial ) ; // wait for leonardo

    uint8_t buff[100];

    irpacker_save( (void*) 169 );
    Serial.println("saved!");
}

void loop() {
}
