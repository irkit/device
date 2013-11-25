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
    IrPacker packer(buff);
    /* packer.load( (void*) 177 ); */
    packer.save( (void*) 169 );
    Serial.println("saved!");
    /* uint8_t packed = packer.pack( 30 ); */
    /* uint16_t unpacked = packer.unpack( packed ); */
    /* Serial.print("packed: "); Serial.println(packed); */
    /* Serial.print("unpacked: "); Serial.println(unpacked); */

    /* setBuffer8( buff, 21, */
    /*             0xba, 0xa6, */
    /*             0x01 /\* marker *\/, */
    /*             0x7e /\* val0:815 *\/, */
    /*             0x9e /\* val1:2451 *\/, */
    /*             0x71 /\* length: 113bits = 15byte *\/, */
    /*             0x04, 0x40, 0x50, 0x14, 0x00, 0x00, 0x00, 0x40, */
    /*             0x00, 0x40, 0x10, 0x00, 0x45, 0x55, 0x00 */
    /*             ); */
    /* packer.length_ = 21; */

    /* unsigned long before = micros(); */

    /* packer.unpackStart(); */
    /* for (uint8_t i=0; i<115; i++) { */
    /*     uint16_t unpacked = packer.unpack(); */
    /*     // Serial.println(unpacked); */
    /* } */

    /* unsigned long after = micros(); */
    /* Serial.print( "after-before[us]: " ); Serial.println(after-before); */
}

void loop() {
}
