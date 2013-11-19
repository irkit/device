#ifndef __IRPACKER_H__
#define __IRPACKER_H__

#include <inttypes.h>
#include <stdbool.h>

#define TREE_SIZE 224

class IrPacker
{
public:
    IrPacker();
    uint8_t pack( uint16_t data );
    uint16_t unpack( uint8_t data );
    void save( void *offset );
    void load( void *offset );

    uint16_t tree[TREE_SIZE];
};

#endif // __IRPACKER_H__
