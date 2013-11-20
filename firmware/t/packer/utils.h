#ifndef __UTILS_H__
#define __UTILS_H__

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

void setBuffer16( uint16_t *buff, uint16_t num, ... ){
    va_list list;
    int i;

    va_start( list, num );

    for( i = 0; i < num; ++i ){
        buff[i] = (uint16_t)va_arg( list, int );
    }

    va_end( list );
}

void dump8( const uint8_t *data, uint16_t datasize ) {
    uint16_t i;

    printf("{ ");
    for (i=0; i<datasize; i++) {
        printf("0x%02x ",data[i]);
    }
    printf("}\n");
}

void dump16( const uint16_t *data, uint16_t datasize ) {
    uint16_t i;

    printf("{ ");
    for (i=0; i<datasize; i++) {
        printf("0x%04x ",data[i]);
    }
    printf("}\n");
}

#endif // __UTILS_H__
