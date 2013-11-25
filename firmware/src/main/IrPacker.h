#ifndef __IRPACKER_H__
#define __IRPACKER_H__

#include <inttypes.h>
#include <stdbool.h>

#define TREE_SIZE 224

class IrPacker
{
 public:
    IrPacker(volatile uint8_t *buff);
    void     pack( uint16_t data );
    void     packEnd();
    uint8_t  packSingle( uint16_t data );
    uint16_t unpack();
    void     unpackStart();
    uint16_t unpackSingle( uint8_t data );

    // buff space used (not total unpacked length, caller should manage that)
    uint16_t  length();

    void     clear();

    void     save( void *offset );
    void     load( void *offset );

    uint16_t tree[TREE_SIZE];

 private:
    // packing
    void bitpack( uint8_t data );
    void addBit( bool );

    // unpacking
    uint16_t unpackBit();
    volatile uint8_t *buff_;
    uint16_t  length_;

    // bitpack
    uint8_t  val0_;
    uint8_t  val1_;
    uint8_t  bit_index_;
    uint8_t  byte_index_;
    uint8_t  bitpack_length_;
};

#endif // __IRPACKER_H__
