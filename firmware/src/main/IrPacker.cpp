#include "IrPacker.h"

#ifdef ARDUINO
# include <avr/eeprom.h>
#else
# include <stdio.h>
#endif

#define IRPACKER_OFFSET 30

// Packing/Unpacking uint16_t data into uint8_t data
//
// Raw data is:
// Sequence of number of Timer/Counter1 ticks between Falling/Rising edge of input (IR data).
// IR data commonly (but not limited to) have:
// * A fixed size header
// * A fixed size trailer
// * A mutable size of 1/0
// * Sequence of 0xFFFF 0x0000 means: 2 x 65536 number of ticks without any edges
// * High accuracy not required (5% seems fine)
//
// So, we're going to:
// * Use this reccurence formula: `y(x) = 1.05 * y(x-1)` to map uint16_t values into uint8_t values
// * If pair of 0/1 values are found, pack it specially
//
// sample data:
// 461A 231D 0491 0476 0490 0D24 0493 0D24 0492 0D23 0494 0475 0493 0D22 0495 0D21
// 0494 0D21 0495 0D20 0496 0D1F 0496 0D20 0496 0472 0495 0472 0493 0473 0494 0473
// 0493 0D30 0495 0D20 0494 0D21 0496 0472 0492 0D21 0495 0473 0493 0474 0493 0474
// 0492 0474 0492 0474 0492 0D22 0493 0D22 0494 0474 0492 0D21 0494 0474 0491 0D22
// 0493 0477 0492 FFFF 0000 229F 4623 1164 0494
//
// becomes:
// 0xd7 0xc3 0x00 0x41 0x00 0x88 0xa7 0x15
// 0x15 0x54 0x01 0x51 0x00 0x14 0x44 0x00
// 0xff 0xc3 0xd7 0xaf 0x88

IrPacker::IrPacker() {
}

// * data : input
// * packed : packed output
// * datasize : number of uint16_t entries in data
uint8_t IrPacker::pack( uint16_t data ) {
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

// * data : input
// * unpacked : unpacked output
// * datasize : number of uint8_t entries in data
uint16_t IrPacker::unpack( uint8_t data ) {
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
}

void IrPacker::load( void *offset ) {
    eeprom_read_block( tree, offset, sizeof(tree) );
}
