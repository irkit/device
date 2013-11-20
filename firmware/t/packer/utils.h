#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdarg.h> // for va_list

void SetBuffer8( uint8_t *buff, uint16_t num, ... ){
    va_list list;
    int i;

    va_start( list, num );

    for( i = 0; i < num; ++i ){
        buff[i] = (uint8_t)va_arg( list, int );
    }

    va_end( list );
}

void SetBuffer16( uint16_t *buff, uint16_t num, ... ){
    va_list list;
    int i;

    va_start( list, num );

    for( i = 0; i < num; ++i ){
        buff[i] = (uint16_t)va_arg( list, int );
    }

    va_end( list );
}

#endif // __UTILS_H__
