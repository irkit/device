/*
 Copyright (C) 2013-2014 Masakazu Ohtsuka
  
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
  
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
  
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "IrPacker.h"
#include "env.h"

#ifdef ARDUINO
# include <avr/eeprom.h>
#else
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
#endif

#ifndef UINT16_MAX
# define UINT16_MAX 0xFFFF
#endif

#define IRPACKER_OFFSET 30 // 0-29 is reserved for special data
#define BITPACK_MARKER  0x01

uint16_t tree[TREE_SIZE];

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

void irpacker_clear( volatile struct irpacker_t *state ) {
    state->length    = 0;
    state->val0      = 0;
    state->val1      = 0;
    state->bit_index = 0;
}

void irpacker_reset( volatile struct irpacker_t *state ) {
    state->val0      = 0;
    state->val1      = 0;
    state->bit_index = 0;
}

void add_bit(volatile struct irpacker_t *state, bool value) {
    uint8_t byte_index = state->bit_index >> 3;
    uint16_t offset    = state->length + 4 + byte_index;
    uint8_t odd        = state->bit_index % 8;
    if (odd == 0) {
        state->buff[ offset ] = 0;
    }
    if (value) {
        bitSet  ( state->buff[ offset ], 7 - odd);
    }
    else {
        bitClear( state->buff[ offset ], 7 - odd );
    }
    state->bit_index ++;

    if (state->bit_index == 255) {
        irpacker_packend( state );
    }
}

void bitpack( volatile struct irpacker_t *state, uint8_t data ) {
    if ( (data == 0) || (data == 0xFF) ) {
        irpacker_packend( state );
        state->buff[ state->length ++ ] = data;
    }
    // don't pack 0, val0 is empty if equal to 0
    else if ( ! state->val0 ) {
        state->val0          = data;
    }
    // if data is similar to val0, pack it
    // 2: less than 12% (3.5 * 3.5) error rate
    else if ( ( (data  <= state->val0) && (state->val0 - data  <= 2) ) ||
              ( (state->val0 <= data)  && (data  - state->val0 <= 2) ) ) {
        if ( ! state->bit_index ) {
            add_bit(state, 0);
            if ( state->val1 ) {
                add_bit(state, 1);
            }
        }
        add_bit(state, 0);
    }
    else if ( ! state->val1 ) {
        state->val1 = data;
        if ( state->bit_index ) {
            add_bit(state, 1);
        }
    }
    else if ( ( (data  <= state->val1) && (state->val1 - data  <= 2) ) ||
              ( (state->val1 <= data)  && (data  - state->val1 <= 2) ) ) {
        if ( ! state->bit_index ) {
            if ( state->val0 ) {
                add_bit(state, 0);
            }
            add_bit(state, 1);
        }
        add_bit(state, 1);
    }
    else {
        irpacker_packend( state );
        state->val0          = data;
    }
}

uint8_t pack_single( uint16_t data ) {
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
        return TREE_SIZE - 1 + IRPACKER_OFFSET;
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

uint16_t unpack_single( uint8_t data ) {
    if (data == 0) {
        return 0;
    }
    if (data <= IRPACKER_OFFSET) {
        return tree[ 0 ];
    }
    if (data == 0xFF) {
        return 0xFFFF;
    }
    if (data >= TREE_SIZE - 1 + IRPACKER_OFFSET) {
        return tree[TREE_SIZE - 1];
    }
    return tree[ data - IRPACKER_OFFSET ];
}

uint16_t unpack_bit( volatile struct irpacker_t *state ) {
    uint8_t  odd             = state->bit_index % 8;
    uint8_t  bitpacked_bytes = state->bit_index >> 3;
    uint8_t  packed          = bitRead( state->buff[ state->byte_index + 4 + bitpacked_bytes ], 7 - odd )
        ? state->val1
        : state->val0;
    uint16_t unpacked        = unpack_single( packed );

    state->bit_index ++;
    if (state->bit_index == state->bitpack_length) {
        state->bitpack_length  = 0;
        state->byte_index     += (5 + bitpacked_bytes);
    }

    return unpacked;
}

void irpacker_init( volatile struct irpacker_t *state, volatile uint8_t *buff ) {
    state->buff = buff;
    irpacker_clear( state );
}

void irpacker_pack( volatile struct irpacker_t *state, uint16_t data ) {
    uint8_t packed = pack_single( data );
    bitpack( state, packed );
}

void irpacker_packend( volatile struct irpacker_t *state ) {
    if (state->bit_index == 0) {
        if (state->val0) {
            state->buff[ state->length ++ ] = state->val0;
        }
        if (state->val1) {
            state->buff[ state->length ++ ] = state->val1;
        }
    }
    else {
        state->buff[ state->length ++ ] = BITPACK_MARKER;
        state->buff[ state->length ++ ] = state->val0;
        state->buff[ state->length ++ ] = state->val1;
        state->buff[ state->length ++ ] = state->bit_index;
        state->length += (1 + ((state->bit_index-1) >> 3));
    }
    state->val0      = 0;
    state->val1      = 0;
    state->bit_index = 0;
}

// returns safe length (we might consume less, but not more)
uint16_t irpacker_safelength( const volatile struct irpacker_t *state ) {
    return state->length + 5 + (state->bit_index >> 3);
}

uint16_t irpacker_length( const volatile struct irpacker_t *state ) {
    return state->length;
}

void irpacker_unpack_start( volatile struct irpacker_t *state ) {
    state->bit_index      = 0;
    state->byte_index     = 0;
    state->bitpack_length = 0;
}

uint16_t irpacker_unpack( volatile struct irpacker_t *state ) {
    if ((state->byte_index == state->length) || (state->byte_index == UINT16_MAX)) {
        return 0;
    }
    if (state->bitpack_length > 0) {
        return unpack_bit(state);
    }
    uint8_t data = state->buff[ state->byte_index ];
    if (data == BITPACK_MARKER) {
        state->val0           = state->buff[ state->byte_index + 1 ];
        state->val1           = state->buff[ state->byte_index + 2 ];
        state->bitpack_length = state->buff[ state->byte_index + 3 ];
        state->bit_index      = 0;
        return unpack_bit(state);
    }
    state->byte_index ++;
    return unpack_single( data );
}

void irpacker_unpack_sequence( volatile struct irpacker_t *state, uint8_t *in, uint16_t length, IrPackerUnpackCallback callback ) {
#ifndef ARDUINO
    memcpy( (void*)state->buff, in, length );
    state->length = length;

    while (state->byte_index < state->length) {
        callback( irpacker_unpack(state) );
    }
#endif
}

int8_t irpacker_save( void *offset ) {
#ifdef SAVE_IRPACKER_TREE
    uint16_t tree[TREE_SIZE]  = {
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

    // verify
    uint16_t i;
    for (i=0; i<TREE_SIZE; i++) {
        uint16_t leaf = eeprom_read_word( (const uint16_t *)(offset + i * 2) );
        if (leaf != tree[ i ]) {
            return -1;
        }
    }

    return 0;
#endif
    return -1;
}

void irpacker_load( void *offset ) {
#ifdef ARDUINO
    eeprom_read_block( tree, offset, sizeof(tree) );
#endif
}
