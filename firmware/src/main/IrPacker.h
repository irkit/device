#ifndef __IRPACKER_H__
#define __IRPACKER_H__

#include <inttypes.h>
#include <stdbool.h>

// 512(=IR_BUFF_SIZE) / 8
#define IRBITPACK_VALUE_SIZE 64

class IrBitPack
{
public:
    IrBitPack();
    // packing
    bool Add16(uint16_t val);
    uint8_t Write( uint8_t *out );

    // unpacking
    uint8_t StreamParse(uint8_t value, uint16_t *unpacked, uint16_t unpacked_index, uint16_t maxsize);

    void Clear();

    // class methods
    static bool IsStartOfBits( const uint16_t *data, uint16_t datasize, uint16_t input_index );
private:
    uint16_t val0_;
    uint16_t val1_;
    uint8_t values_[IRBITPACK_VALUE_SIZE];
    uint16_t bit_index_; // 0-511
    // uint8_t  byte_index_; // 0-63
    uint16_t bit_length_; // 0-511
    uint8_t  bit_length_received_count_; // 0-1

    void AddBit(bool value);
};

class IrPacker
{
public:
    IrPacker();
    uint16_t Pack( const uint16_t *data, uint8_t *packed, uint16_t datasize );
    uint16_t Unpack( const uint8_t *data, uint16_t *unpacked, uint16_t datasize, uint16_t maxsize );

    // class methods
    static uint8_t BitPack( uint16_t value );
    static uint16_t BitUnpack( uint8_t value );

private:
    bool is_bit_packing_;
    IrBitPack bitpack_;

    void PackSingle( uint16_t value, uint8_t *packed_value, uint16_t packed_index );
    void UnpackSingle( uint8_t value, uint16_t *unpacked_value, uint16_t unpacked_index );
    bool IsStartOfAbsence( const uint16_t *data, uint16_t datasize, uint16_t input_index );
};

#endif // __IRPACKER_H__
