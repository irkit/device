#include "IrPacker.h"
#include "nanotap.h"
#include "utils.h"
#include "string.h"

int main() {
    uint8_t length;
    uint16_t buff[256] = { 0 };
    uint8_t packed[512] = { 0 };
    uint8_t expected[512] = { 0 };

    ok( 1, "ok" );

    {
        IrPacker packer;
        memset( buff, 0, sizeof(buff) );
        memset( packed, 0, sizeof(packed) );
        SetBuffer16( buff, 1, 0x461A );

        length = packer.Pack( buff, packed, 1 );

        ok( length == 1, "1 uint16_t turns into 1 uint8_t" );
        ok( packed[ 0 ] == 0xD7, "0x461A turns into 0xD7" );
    }

    {
        IrPacker packer;
        memset( buff, 0, sizeof(buff) );
        memset( packed, 0, sizeof(packed) );
        SetBuffer16( buff, 2, 0xFFFF, 0x0000 );

        length = packer.Pack( buff, packed, 2 );

        ok( length == 1, "0xFFFF 0x0000 pattern turns into 1 uint8_t" );
        ok( packed[ 0 ] == 0xFF, "0xFFFF 0x0000 turns into 0xFF" );
    }

    {
        IrPacker packer;
        memset( buff, 0, sizeof(buff) );
        memset( packed, 0, sizeof(packed) );
        SetBuffer16( buff, 4, 0x461A, 0x231D, 0x491, 0x476 );

        length = packer.Pack( buff, packed, 4 );

        ok( length == 4, "normal packed" );
        ok( packed[ 0 ] == 0xD7 );
        ok( packed[ 1 ] == 0xC3 );
        ok( packed[ 2 ] == 0x88 );
        ok( packed[ 3 ] == 0x87 );
    }

    {
        IrPacker packer;
        memset( buff, 0, sizeof(buff) );
        memset( packed, 0, sizeof(packed) );
        memset( expected, 0, sizeof(expected) );
        SetBuffer16( buff, 5, 0x461A, 0x231D, 0x491, 0x476, 0x0490 );

        length = packer.Pack( buff, packed, 5 );

        ok( length == 8, "bit packed" );
        SetBuffer8( expected, 8,
                    0xd7, 0xc3, 0x00, 0x03, 0x00, 0x88, 0xFE, 0x00 );
        ok( (memcmp(packed, expected, 8) == 0), "compared ok" );
    }

    {
        IrPacker packer;
        memset( buff, 0, sizeof(buff) );
        memset( packed, 0, sizeof(packed) );
        memset( expected, 0, sizeof(expected) );
        SetBuffer16( buff, 6, 0x461A, 0x231D, 0x491, 0x476, 0x0490, 0x0D24 );

        length = packer.Pack( buff, packed, 6 );

        ok( length == 8, "bit packed 2" );

        SetBuffer8( expected, 8,
                    0xd7, 0xc3, 0x00, 0x04, 0x00, 0x88, 0xa7, 0x10 );
        ok( (memcmp(packed, expected, 8) == 0), "compared ok" );
    }

    {
        IrPacker packer;
        memset( buff, 0, sizeof(buff) );
        memset( packed, 0, sizeof(packed) );
        memset( expected, 0, sizeof(expected) );

        SetBuffer16( buff, 73,
                     0x461A, 0x231D, 0x0491, 0x0476, 0x0490, 0x0D24, 0x0493, 0x0D24,
                     0x0492, 0x0D23, 0x0494, 0x0475, 0x0493, 0x0D22, 0x0495, 0x0D21,
                     0x0494, 0x0D21, 0x0495, 0x0D20, 0x0496, 0x0D1F, 0x0496, 0x0D20,
                     0x0496, 0x0472, 0x0495, 0x0472, 0x0493, 0x0473, 0x0494, 0x0473,
                     0x0493, 0x0D30, 0x0495, 0x0D20, 0x0494, 0x0D21, 0x0496, 0x0472,
                     0x0492, 0x0D21, 0x0495, 0x0473, 0x0493, 0x0474, 0x0493, 0x0474,
                     0x0492, 0x0474, 0x0492, 0x0474, 0x0492, 0x0D22, 0x0493, 0x0D22,
                     0x0494, 0x0474, 0x0492, 0x0D21, 0x0494, 0x0474, 0x0491, 0x0D22,
                     0x0493, 0x0477, 0x0492, 0xFFFF, 0x0000, 0x229F, 0x4623, 0x1164,
                     0x0494 );

        length = packer.Pack( buff, packed, 73 );

        // 0x461A, 0x231D,
        // -> 0xD7, 0xC3
        // 0x0491, 0x0476, 0x0490, 0x0D24, 0x0493, 0x0D24, 0x0492, 0x0D23,
        // 0x0494, 0x0475, 0x0493, 0x0D22, 0x0495, 0x0D21, 0x0494, 0x0D21,
        // 0x0495, 0x0D20, 0x0496, 0x0D1F, 0x0496, 0x0D20, 0x0496, 0x0472,
        // 0x0495, 0x0472, 0x0493, 0x0473, 0x0494, 0x0473, 0x0493, 0x0D30,
        // 0x0495, 0x0D20, 0x0494, 0x0D21, 0x0496, 0x0472, 0x0492, 0x0D21,
        // 0x0495, 0x0473, 0x0493, 0x0474, 0x0493, 0x0474, 0x0492, 0x0474,
        // 0x0492, 0x0474, 0x0492, 0x0D22, 0x0493, 0x0D22, 0x0494, 0x0474,
        // 0x0492, 0x0D21, 0x0494, 0x0474, 0x0491, 0x0D22, 0x0493, 0x0477,
        // 0x0492,
        // -> 0x00 marker
        // -> 0x0041 uint16_t number of bits = 65d
        // -> 0x88 value of bit:0
        // -> 0xa7 value of bit:1
        // -> 0x15       0x15       0x54       0x01
        //    0b00010101 0b00010101 0b01010100 0b00000001
        // -> 0x51       0x00       0x14       0x44       0x00
        //    0b01010001 0b00000000 0b00010100 0b01000100 0b00000000
        // 0xFFFF 0x0000
        // -> 0xff
        // 0x229F, 0x4623, 0x1164, 0x0494
        // -> 0xc3 0xd7 0xaf 0x88

        ok( length == 21, "bit packed real" );
        SetBuffer8( expected, 21,
                    0xd7, 0xc3, 0x00, 0x41, 0x00, 0x88, 0xa7, 0x15,
                    0x15, 0x54, 0x01, 0x51, 0x00, 0x14, 0x44, 0x00,
                    0xff, 0xc3, 0xd7, 0xaf, 0x88 );
        ok( (memcmp(packed, expected, 21) == 0), "compared ok" );
    }

    done_testing();
}
