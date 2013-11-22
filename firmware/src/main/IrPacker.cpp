#include "IrPacker.h"

#ifdef ARDUINO
# include <avr/eeprom.h>
#else
# include <stdio.h>
#endif

#define IRPACKER_OFFSET 30
#define BITPACK_MARKER  0x01

// Packing/Unpacking uint16_t data into uint8_t data
//
// We have limited knowledge about raw data:
// Sequence of number of Timer/Counter ticks between Falling/Rising edge of input (IR data).
// IR data commonly have:
// * A fixed size header
// * A fixed size trailer
// * A mutable size of 1/0
// * Sequence of 0xFFFF 0x0000 means: 2 x 65536 number of ticks without any edges
// * High accuracy not required (5% seems fine)
//
// So, we're going to:
// * Use this reccurence formula: `y(x) = 1.05 * y(x-1)` to map uint16_t values into uint8_t values
// * If pair of 0/1 values are found, pack it into bits
//
// see t/packer/test.cpp for detail

// input -> packed
// A     ->
// B     ->
// A     -> [1][A][B][?][0b010]
// A     -> [1][A][B][?][0b0100]
// B     -> [1][A][B][?][0b01001]
// C     -> [1][A][B][5][0b01001]
// end   -> [1][A][B][5][0b01001][C]

// input -> packed
// A     ->
// A     -> [1][A][0][?][0b00]
// B     -> [1][A][B][?][0b001]
// B     -> [1][A][B][?][0b0011]
// end   -> [1][A][B][4][0b0011]

// input -> packed
// A     ->
// A     ->
// end   -> [1][A][0][2][0b00]

// input -> packed
// A     ->
// B     ->
// C     -> [A][B]

#ifndef bitRead
# define bitRead(value, bit)  (((value) >> (bit)) & 0x01)
#endif
#ifndef bitSet
# define bitSet(value, bit)   ((value) |=  (1 << (bit)))
#endif
#ifndef bitClear
# define bitClear(value, bit) ((value) &= ~(1 << (bit)))
#endif

IrPacker::IrPacker(volatile uint8_t *buff) :
    buff_( buff ) {
    clear();
}

void IrPacker::clear() {
    length_    = 0;
    val0_      = 0;
    val1_      = 0;
    bit_index_ = 0;
}

void IrPacker::pack( uint16_t data ) {
    uint8_t packed = packSingle( data );
    bitpack( packed );
}

void IrPacker::packEnd() {
    if (bit_index_ == 0) {
        if (val0_) {
            buff_[ length_ ++ ] = val0_;
        }
        if (val1_) {
            buff_[ length_ ++ ] = val1_;
        }
    }
    else {
        buff_[ bitpack_start_     ] = BITPACK_MARKER;
        buff_[ bitpack_start_ + 1 ] = val0_;
        buff_[ bitpack_start_ + 2 ] = val1_;
        buff_[ bitpack_start_ + 3 ] = bit_index_;
        length_ += (5 + ((bit_index_-1) >> 3));
    }
    val0_      = 0;
    val1_      = 0;
    bit_index_ = 0;
}

// returns safe length (we might consume less, but not more)
uint8_t IrPacker::length() {
    return length_ + 5 + (bit_index_ >> 3);
}

void IrPacker::bitpack( uint8_t data ) {
    if ( (data == 0) || (data == 0xFF) ) {
        packEnd();
        buff_[ length_ ++ ] = data;
    }
    // don't pack 0, val0 is empty if equal to 0
    else if ( ! val0_ ) {
        val0_          = data;
        bitpack_start_ = length_;
    }
    // if data is similar to val0, pack it
    // 2: less than 12% (3.5 * 3.5) error rate
    else if ( ( (data  <= val0_) && (val0_ - data  <= 2) ) ||
              ( (val0_ <= data)  && (data  - val0_ <= 2) ) ) {
        if ( ! bit_index_ ) {
            addBit(0);
            if ( val1_ ) {
                addBit(1);
            }
        }
        addBit(0);
    }
    else if ( ! val1_ ) {
        val1_ = data;
        if ( bit_index_ ) {
            addBit(1);
        }
    }
    else if ( ( (data  <= val1_) && (val1_ - data  <= 2) ) ||
              ( (val1_ <= data)  && (data  - val1_ <= 2) ) ) {
        if ( ! bit_index_ ) {
            if ( val0_ ) {
                addBit(0);
            }
            addBit(1);
        }
        addBit(1);
    }
    else {
        packEnd();
        val0_          = data;
        bitpack_start_ = length_;
    }
}

void IrPacker::addBit(bool value) {
    uint8_t byte_index = bit_index_ >> 3;
    uint8_t offset     = bitpack_start_ + 4 + byte_index;
    uint8_t odd        = bit_index_ % 8;
    if (odd == 0) {
        buff_[ offset ] = 0;
    }
    if (value) {
        bitSet  ( buff_[ offset ], 7 - odd);
    }
    else {
        bitClear( buff_[ offset ], 7 - odd );
    }
    bit_index_ ++;

    if (bit_index_ == 255) {
        packEnd();
    }
}

uint8_t IrPacker::packSingle( uint16_t data ) {
    if (data == 0) {
        return 0;
    }
    if (data <= tree[0]) {
        return IRPACKER_OFFSET;
    }
    if (data == 0xFFFF) {
        return 0xFF;
    }
    if (data >= tree[TREE_SIZE - 1]) {
        return IRPACKER_OFFSET + TREE_SIZE - 1;
    }
    uint8_t min_index = 0;
    uint8_t max_index = TREE_SIZE - 1;
    uint8_t index;
    while (1) {
        index = ((max_index - min_index) >> 1) + min_index;
        uint16_t val = tree[ index ];
        if (data >= val) {
            min_index = index;
        }
        else {
            max_index = index;
        }
        if (max_index - min_index == 1) {
            break;
        }
    }
    return min_index + IRPACKER_OFFSET;
}

void IrPacker::unpackStart() {
    bit_index_      = 0;
    byte_index_     = 0;
    bitpack_length_ = 0;
}

uint16_t IrPacker::unpack() {
    if (byte_index_ == length_) {
        return 0;
    }
    if (bitpack_length_ > 0) {
        return unpackBit();
    }
    uint8_t data = buff_[ byte_index_ ];
    if (data == BITPACK_MARKER) {
        bitpack_start_  = byte_index_;
        val0_           = buff_[ bitpack_start_ + 1 ];
        val1_           = buff_[ bitpack_start_ + 2 ];
        bitpack_length_ = buff_[ bitpack_start_ + 3 ];
        bit_index_      = 0;
        return unpackBit();
    }
    else {
        byte_index_ ++;
        return unpackSingle( data );
    }
}

uint16_t IrPacker::unpackBit() {
    uint8_t  odd             = bit_index_ % 8;
    uint8_t  bitpacked_bytes = bit_index_ >> 3;
    uint8_t  packed          = bitRead( buff_[ bitpack_start_ + 4 + bitpacked_bytes ], 7 - odd )
                                   ? val1_ : val0_;
    uint16_t unpacked        = unpackSingle( packed );

    bit_index_ ++;
    if (bit_index_ == bitpack_length_) {
        bitpack_length_  = 0;
        byte_index_     += (5 + bitpacked_bytes);
    }

    return unpacked;
}

uint16_t IrPacker::unpackSingle( uint8_t data ) {
    if (data == 0) {
        return 0;
    }
    if (data <= IRPACKER_OFFSET) {
        return tree[ 0 ];
    }
    if (data == 0xFF) {
        return 0xFFFF;
    }
    if (data - 30 >= TREE_SIZE - 1) {
        return tree[TREE_SIZE - 1];
    }
    return tree[ data - IRPACKER_OFFSET ];
}

void IrPacker::save( void *offset ) {
#ifdef ARDUINO
    uint16_t tree[TREE_SIZE] = {
        30, 31, 32, 33, 34, 35, 36, 38,
        39, 40, 42, 43, 45, 46, 48, 50,
        52, 53, 55, 57, 59, 61, 63, 66,
        68, 70, 73, 75, 78, 81, 84, 87,
        90, 93, 96, 100, 103, 107, 110, 114,
        118, 122, 127, 131, 136, 141, 146, 151,
        156, 161, 167, 173, 179, 185, 192, 198,
        205, 213, 220, 228, 236, 244, 253, 262,
        271, 280, 290, 300, 311, 322, 333, 345,
        357, 369, 382, 395, 409, 424, 439, 454,
        470, 486, 503, 521, 539, 558, 578, 598,
        619, 640, 663, 686, 710, 735, 761, 787,
        815, 843, 873, 904, 935, 968, 1002, 1037,
        1073, 1111, 1150, 1190, 1232, 1275, 1319, 1366,
        1413, 1463, 1514, 1567, 1622, 1679, 1738, 1798,
        1861, 1927, 1994, 2064, 2136, 2211, 2288, 2368,
        2451, 2537, 2626, 2718, 2813, 2911, 3013, 3119,
        3228, 3341, 3458, 3579, 3704, 3834, 3968, 4107,
        4251, 4400, 4554, 4713, 4878, 5049, 5226, 5408,
        5598, 5794, 5997, 6206, 6424, 6648, 6881, 7122,
        7371, 7629, 7896, 8173, 8459, 8755, 9061, 9379,
        9707, 10047, 10398, 10762, 11139, 11529, 11932, 12350,
        12782, 13230, 13693, 14172, 14668, 15181, 15713, 16263,
        16832, 17421, 18031, 18662, 19315, 19991, 20691, 21415,
        22165, 22940, 23743, 24574, 25434, 26325, 27246, 28200,
        29187, 30208, 31265, 32360, 33492, 34665, 35878, 37134,
        38433, 39779, 41171, 42612, 44103, 45647, 47245, 48898,
        50610, 52381, 54214, 56112, 58076, 60108, 62212, 64390
    };
    eeprom_write_block( tree, offset, sizeof(tree) );
#endif
}

void IrPacker::load( void *offset ) {
#ifdef ARDUINO
    eeprom_read_block( tree, offset, sizeof(tree) );
#endif
}
