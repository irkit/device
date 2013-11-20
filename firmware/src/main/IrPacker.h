#ifndef __IRPACKER_H__
#define __IRPACKER_H__

#include <inttypes.h>
#include <stdbool.h>

#define TREE_SIZE 224

class IrPacker
{
 public:
    IrPacker(uint8_t *buff);
    void     pack( uint16_t data );
    void     packEnd();
    uint16_t unpack();
    void     unpackStart();

    // buff space used (not total unpacked length, caller should manage that)
    uint8_t  length();

    void     clear();

    void     save( void *offset );
    void     load( void *offset );

    uint16_t tree[TREE_SIZE];

 private:
    // packing
    uint8_t packSingle( uint16_t data );
    void bitpack( uint8_t data );
    void addBit( bool );

    // unpacking
    uint16_t unpackSingle( uint8_t data );
    uint16_t unpackBit();
    uint8_t *buff_;
    uint8_t  length_;

    // bitpack
    uint8_t  val0_;
    uint8_t  val1_;
    uint8_t  bit_index_;
    uint8_t  byte_index_;
    uint8_t  bitpack_start_;
    uint8_t  bitpack_length_;
};

#endif // __IRPACKER_H__
