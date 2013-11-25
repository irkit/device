#include "IrPacker.h"
#include "nanotap.h"
#include "utils.h"
#include "string.h"

// Arduino can load from EEPROM, but we can't
void fillTree(IrPacker *packer) {
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
    memcpy( packer->tree, tree, sizeof(tree) );
}

void dump( uint8_t *buff, uint8_t length ) {
    printf("dump:\n");
    for (uint8_t i=0; i<length; i++) {
        printf("%3d 0x%x\n", i, buff[i]);
    }
}

void dump16( uint16_t *buff, uint16_t length ) {
    printf("dump16:\n");
    for (uint16_t i=0; i<length; i++) {
        printf("%d 0x%04x ", i, buff[i]);
        if (i % 8 == 7) {
            printf("\n");
        }
    }
}

void pack( IrPacker *packer, uint16_t *input, uint16_t length ) {
    for (uint16_t i=0; i<length; i++) {
        uint16_t val = input[ i ];
        packer->pack( val );
    }
    packer->packEnd();
}

void unpack( IrPacker *packer, uint16_t *output, uint16_t length ) {
    packer->unpackStart();
    for (uint16_t i=0; i<length; i++) {
        output[ i ] = packer->unpack();
    }
}

int main() {
    ok( 1, "ok" );
    uint8_t buff[ 100 ];

    {
        IrPacker packer( buff );
        fillTree( &packer );
        uint16_t input = 17946; // 0x461A

        packer.pack( input );
        packer.packEnd();

        ok( buff[ 0 ] == 215 );
        ok( packer.length() == 6 );

        packer.unpackStart();
        uint16_t unpacked = packer.unpack();

        ok( unpacked == 17421 );
    }

    {
        IrPacker packer( buff );
        fillTree( &packer );
        uint16_t input = 30;

        packer.pack( input );
        packer.packEnd();

        ok( buff[ 0 ] == 30 );
        ok( packer.length() == 6 );

        packer.unpackStart();
        uint16_t unpacked = packer.unpack();

        ok( unpacked == 30 );
    }

    {
        IrPacker packer( buff );
        fillTree( &packer );
        uint16_t input = 65000;

        packer.pack( input );
        packer.packEnd();

        ok( buff[ 0 ] == 253 );
        ok( packer.length() == 6 );

        packer.unpackStart();
        uint16_t unpacked = packer.unpack();

        ok( unpacked == 64390 );
    }

    {
        IrPacker packer( buff );
        fillTree( &packer );
        uint16_t input = 60108;

        packer.pack( input );
        packer.packEnd();

        ok( buff[ 0 ] == 251 );
        ok( packer.length() == 6 );

        packer.unpackStart();
        uint16_t unpacked = packer.unpack();

        ok( unpacked == 60108 );
    }

    {
        IrPacker packer( buff );
        fillTree( &packer );
        uint16_t input = 65535;

        packer.pack( input );
        packer.packEnd();

        ok( buff[ 0 ] == 255 );
        ok( packer.length() == 6 );

        packer.unpackStart();
        uint16_t unpacked = packer.unpack();

        ok( unpacked == 65535 );
    }

    {
        IrPacker packer( buff );
        fillTree( &packer );
        uint16_t input = 0;

        packer.pack( input );
        packer.packEnd();

        ok( buff[ 0 ] == 0 );
        ok( packer.length() == 6 );

        packer.unpackStart();
        uint16_t unpacked = packer.unpack();

        ok( unpacked == 0 );
    }

    {
        IrPacker packer( buff );
        fillTree( &packer );

        uint16_t input[ 100 ];
        setBuffer16( input, 2,
                     0x440d, 0x2233 );
        pack( &packer, input, 2 );

        uint8_t expected[ 100 ];
        memset( expected, 0, sizeof(expected) );
        setBuffer8( expected, 2,
                    215, 195 );
        ok( (memcmp(buff, expected, 2) == 0), "packed ok" );
        ok( packer.length() == 7 );

        uint16_t unpacked[ 100 ];
        unpack( &packer, unpacked, 2 );

        ok( (memcmp(unpacked, input, 2) == 0), "unpacked ok" );
        // dump16( unpacked, 2 );
    }

    {
        IrPacker packer( buff );
        fillTree( &packer );

        uint16_t input[ 100 ];
        setBuffer16( input, 2,
                     0x440d, 0x440d );
        pack( &packer, input, 2 );

        uint8_t expected[ 100 ];
        memset( expected, 0, sizeof(expected) );
        setBuffer8( expected, 5,
                    1, 215, 0, 2, 0 );
        ok( (memcmp(buff, expected, 5) == 0), "compared ok" );
        ok( packer.length() == 10 );

        uint16_t unpacked[ 100 ];
        unpack( &packer, unpacked, 2 );

        ok( (memcmp(unpacked, input, 2) == 0), "unpacked ok" );
        // dump16( unpacked, 2 );
    }

    {
        IrPacker packer( buff );
        fillTree( &packer );

        uint16_t input[ 100 ];
        setBuffer16( input, 3,
                     0x440d, 0x440d, 0x2233 );
        pack( &packer, input, 3 );

        uint8_t expected[ 100 ];
        memset( expected, 0, sizeof(expected) );
        setBuffer8( expected, 5,
                    1, 215, 195, 3, 0b00100000 );

        ok( (memcmp(buff, expected, 5) == 0), "compared ok" );
        ok( packer.length() == 10 );
        // printf("length: %d\n", packer.length());
        // dump8( buff, 5 );

        uint16_t unpacked[ 100 ];
        unpack( &packer, unpacked, 3 );

        ok( (memcmp(unpacked, input, 3) == 0), "unpacked ok" );
        // dump16( unpacked, 3 );
    }

    {
        IrPacker packer( buff );
        fillTree( &packer );

        uint16_t input[ 100 ];
        setBuffer16( input, 3,
                     0x440d, 0x2233, 0x440d );
        pack( &packer, input, 3 );

        uint8_t expected[ 100 ];
        memset( expected, 0, sizeof(expected) );
        setBuffer8( expected, 5,
                    1, 215, 195, 3, 0b01000000 );

        ok( (memcmp(buff, expected, 5) == 0), "compared ok" );
        ok( packer.length() == 10 );
        // printf("length: %d\n", packer.length());
        // dump8( buff, 5 );

        uint16_t unpacked[ 100 ];
        unpack( &packer, unpacked, 3 );

        ok( (memcmp(unpacked, input, 3) == 0), "unpacked ok" );
        // dump16( unpacked, 3 );
    }

    {
        IrPacker packer( buff );
        fillTree( &packer );

        uint16_t input[ 100 ];
        setBuffer16( input, 73,
                     0x440d, 0x2233, 0x047e, 0x047e, 0x047e, 0x0d0d, 0x047e, 0x0d0d,
                     0x047e, 0x0d0d, 0x047e, 0x047e, 0x047e, 0x0d0d, 0x047e, 0x0d0d,
                     0x047e, 0x0d0d, 0x047e, 0x0d0d, 0x047e, 0x0d0d, 0x047e, 0x0d0d,
                     0x047e, 0x047e, 0x047e, 0x047e, 0x047e, 0x047e, 0x047e, 0x047e,
                     0x047e, 0x0d0d, 0x047e, 0x0d0d, 0x047e, 0x0d0d, 0x047e, 0x047e,
                     0x047e, 0x0d0d, 0x047e, 0x047e, 0x047e, 0x047e, 0x047e, 0x047e,
                     0x047e, 0x047e, 0x047e, 0x047e, 0x047e, 0x0d0d, 0x047e, 0x0d0d,
                     0x047e, 0x047e, 0x047e, 0x0d0d, 0x047e, 0x047e, 0x047e, 0x0d0d,
                     0x047e, 0x047e, 0x047e, 0xFFFF, 0x0000, 0x2233, 0x440d, 0x1130,
                     0x047e );

        pack( &packer, input, 73 );

        uint8_t expected[ 100 ];
        memset( expected, 0, sizeof(expected) );
        setBuffer8( expected, 21,
                    0xd7 /* 0x461a */, 0xc3 /* 0x231d */,
                    0x01 /* marker */, 0x88 /* val0 */, 0xa7 /*val1*/, 0x41 /* length */,
                    0x15, 0x15, 0x54, 0x01, 0x51, 0x00, 0x14, 0x44, 0x00, /* bits */
                    0xff, 0x00,
                    0xc3, 0xd7, 0xaf, 0x88
                    );

        ok( (memcmp(buff, expected, 21) == 0), "compared ok" );
        ok( packer.length() == 26 );
        // printf("length: %d\n", packer.length());
        // dump8( buff, 21 );

        uint16_t unpacked[ 100 ];
        unpack( &packer, unpacked, 73 );

        ok( (memcmp(unpacked, input, 73) == 0), "unpacked ok" );
        // dump16( unpacked, 73 );
    }

    {
        IrPacker packer( buff );
        fillTree( &packer );
        uint16_t input;
        char buf[100];

        for (uint8_t i=0; i<224; i++) {
            input = packer.tree[ i ];
            packer.clear();
            packer.pack( input );
            packer.packEnd();
            uint8_t packed = buff[ 0 ];
            sprintf( buf, "%d (0x%x) --pack-> %d (0x%x)", input, input, packed, packed );
            ok( packed == i + 30, buf );

            packer.unpackStart();
            uint16_t unpacked = packer.unpack();
            sprintf( buf, "%d (0x%x) --unpack-> %d (0x%x)", packed, packed, unpacked, unpacked );
            ok( unpacked == input, buf );
        }
    }

    {
        IrPacker packer( buff );
        fillTree( &packer );

        uint16_t input[ 115 ];
        // エアコンオフ
        setBuffer16( input, 115,
                     6424,3228,
                     815,815,815,815,815,2451,815,815,
                     815,2451,815,815,815,815,815,815,
                     815,2451,815,2451,815,815,815,815,
                     815,815,815,2451,815,2451,815,815,
                     815,815,815,815,815,815,815,815,
                     815,815,815,815,815,815,815,815,
                     815,815,815,815,815,815,815,815,
                     815,2451,815,815,815,815,815,815,
                     815,815,815,815,815,815,815,815,
                     815,2451,815,815,815,815,815,815,
                     815,815,815,2451,815,815,815,815,
                     815,815,815,815,815,815,815,815,
                     815,2451,815,815,815,2451,815,2451,
                     815,2451,815,2451,815,2451,815,2451,
                     815 );

        pack( &packer, input, 115 );

        uint8_t expected[ 115 ];
        memset( expected, 0, sizeof(expected) );
        setBuffer8( expected, 21,
                    0xba, 0xa6,
                    0x01 /* marker */,
                    0x7e /* val0:815 */,
                    0x9e /* val1:2451 */,
                    0x71 /* length: 113bits = 15byte */,
                    0x04, 0x40, 0x50, 0x14, 0x00, 0x00, 0x00, 0x40,
                    0x00, 0x40, 0x10, 0x00, 0x45, 0x55, 0x00
                    );

        ok( (memcmp(buff, expected, 21) == 0), "compared ok" );
        ok( packer.length() == 26 ); // safe length
        // printf("length: %d\n", packer.length());
        // dump8( buff, 21 );

        uint16_t unpacked[ 115 ];
        unpack( &packer, unpacked, 115 );

        ok( (memcmp(unpacked, input, 115) == 0), "unpacked ok" );
        // dump16( unpacked, 115 );
    }

    {
        IrPacker packer( buff );
        fillTree( &packer );

        uint16_t input[ 259 ];
        // エアコンオフ
        setBuffer16( input, 259,
                     6424,3228,815,815,815,815,815,2451,
                     815,815,815,2451,815,815,815,815,
                     815,815,815,2451,815,2451,815,815,
                     815,815,815,815,815,2451,815,2451,
                     815,815,815,815,815,815,815,815,
                     815,815,815,815,815,815,815,815,
                     815,815,815,815,815,815,815,815,
                     815,815,815,2451,815,815,815,815,
                     815,815,815,815,815,815,815,815,
                     815,815,815,2451,815,815,815,815,
                     815,815,815,815,815,2451,815,2451,
                     815,2451,815,2451,815,2451,815,2451,
                     815,2451,815,2451,815,815,815,815,
                     815,2451,815,815,815,815,815,815,
                     815,815,815,815,815,815,815,815,
                     815,815,815,2451,815,2451,815,815,
                     815,815,815,2451,815,815,815,815,
                     815,815,815,815,815,2451,815,815,
                     815,2451,815,2451,815,815,815,815,
                     815,815,815,815,815,815,815,815,
                     815,815,815,815,815,815,815,815,
                     815,815,815,815,815,815,815,2451,
                     815,815,815,815,815,815,815,815,
                     815,815,815,815,815,815,815,815,
                     815,815,815,815,815,815,815,815,
                     815,815,815,815,815,815,815,815,
                     815,815,815,815,815,815,815,815,
                     815,815,815,815,815,815,815,815,
                     815,815,815,815,815,815,815,815,
                     815,2451,815,815,815,2451,815,815,
                     815,815,815,815,815,2451,815,2451,
                     815,815,815,815,815,815,815,2451,
                     815,2451,815
                     );
        pack( &packer, input, 259 );

        uint8_t expected[ 259 ];
        memset( expected, 0, sizeof(expected) );
        setBuffer8( expected, 40,
                    0xba, 0xa6,
                    0x01 /* marker */,
                    0x7e /* val0:815 */,
                    0x9e /* val1:2451 */,
                    0xff /* length: 255bits = 32byte */,
                    0x04, 0x40, 0x50, 0x14, 0x00, 0x00, 0x00, 0x40,
                    0x00, 0x40, 0x15, 0x55, 0x41, 0x00, 0x00, 0x50,
                    0x40, 0x11, 0x40, 0x00, 0x00, 0x04, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x01, 0x10, 0x14, 0x04,
                    0x9e, 0x7e
                    );

        ok( (memcmp(buff, expected, 40) == 0), "compared ok" );
        ok( packer.length() == 45 ); // safe length
        // printf("length: %d\n", packer.length());
        // dump8( buff, 40 );

        uint16_t unpacked[ 259 ];
        unpack( &packer, unpacked, 259 );

        ok( (memcmp(unpacked, input, 259) == 0), "unpacked ok" );
        // dump16( unpacked, 115 );
    }

    {
        IrPacker packer( buff );
        fillTree( &packer );

        uint16_t input[ 17 ];
        // エアコンオフ
        setBuffer16( input, 17,
                     873, 2368, 873, 2368, 873, 2368, 873, 2368,
                     873, 2368, 873, 2368, 873, 2368, 873, 2368,
                     6424
                     );
        pack( &packer, input, 17 );

        uint8_t expected[ 7 ];
        memset( expected, 0, sizeof(expected) );
        setBuffer8( expected, 7,
                    0x01, 0x80, 0x9d, 0x10, 0x55, 0x55,
                    0xba
                    );

        ok( (memcmp(buff, expected, 7) == 0), "compared ok" );
        ok( packer.length() == 12 ); // safe length
        // printf("length: %d\n", packer.length());
        // dump8( buff, 7 );

        uint16_t unpacked[ 17 ];
        unpack( &packer, unpacked, 17 );

        ok( (memcmp(unpacked, input, 17) == 0), "unpacked ok" );
        // dump16( unpacked, 17 );
    }

    {
        IrPacker packer( buff );
        fillTree( &packer );

        uint16_t input[ 595 ];
        // エアコンオフ
        setBuffer16( input, 595,
                     0x1A70 /* 0xbb */, 0xD2C /* 0xa7 */,
                     0x346 /* 0x7e */, 0x9B7 /* 0x9e */, 0x36C /* 0x80 */, 0x3C8, 0x365, 0x3F3, 0x344, 0x3ED,
                     0x343, 0x3EF, 0x33E, 0x3C8, 0x336, 0x424, 0x30E, 0x3FA,
                     0x368, 0x3F2, 0x342, 0x3EF, 0x342, 0x3EF, 0x33C, 0x3CB,
                     0x36A, 0x9E3, 0x345, 0x3EE, 0x340, 0x3C6, 0x36C, 0x3F0,
                     0x33E, 0x3F2, 0x344, 0x3C3, 0x36A, 0x3C8, 0x366, 0x3F3,
                     0x340, 0x3F1, 0x341, 0x3F1, 0x33F, 0x3C6, 0x36B, 0x3F0,
                     0x33C, 0x3F5, 0x340, 0x3F0, 0x343, 0x3EF, 0x33F, 0x3C6,
                     0x36A, 0x3F1, 0x33E, 0x3F3, 0x33F, 0x9B9, 0x371, 0x3EE,
                     0x33C, 0x9E8, 0x343, 0x9B9, 0x368, 0x9E7, 0x341, 0x9BC,
                     0x337, 0xA17, 0x346, 0x9B6, 0x36C, 0x3C8, 0x368, 0x9B9,
                     0x371, 0x9B8, 0x36A, 0x9E5, 0x345, 0x9B6, 0x33A, 0xA17,
                     0x342, 0x9BA, 0x338, 0xA18, 0x342, 0x9B8, 0x370, 0x9BA,
                     0x369, 0x3C8, 0x367, 0x3F3, 0x341, 0x3EF, 0x342, 0x3EF,
                     0x33F, 0x3C9, 0x335, 0x424, 0x341, 0x3EF, 0x30E, 0x424,
                     0x342, 0x3EF, 0x33D, 0x3F3, 0x341, 0x9E3, 0x33F, 0x9BA,
                     0x33B, 0x425, 0x340, 0x3F0, 0x341, 0x9B7, 0x36D, 0x9B9,
                     0x370, 0x9E1, 0x342, 0x9B7, 0x33C, 0x424, 0x342, 0x3EF,
                     0x33F, 0x9B9, 0x36F, 0x9E3, 0x346, 0x3ED, 0x30D, 0x3F9,
                     0x36A, 0x9B8, 0x36F, 0x9E2, 0x344, 0x3F0, 0x33F, 0x3C9,
                     0x363, 0x3F6, 0x341, 0x9B8, 0x36D, 0x3C7, 0x369, 0x9B8,
                     0x36F, 0x3F0, 0x340, 0x3C8, 0x36B, 0x9E1, 0x342, 0x9B9,
                     0x36E, 0x9E3, 0x344, 0x3EF, 0x33E, 0x9E6, 0x342, 0x3CA,
                     0x368, 0x9E2, 0x342, 0x9B7, 0x33C, 0x424, 0x342, 0x3EE,
                     0x33E, 0x9BA, 0x36C, 0x3F3, 0x342, 0x3EF, 0x33D, 0x3F3,
                     0x341, 0x3F0, 0x342, 0x3F0, 0x30C, 0xA17, 0x343, 0x9B9,
                     0x36B, 0x3F2, 0x340, 0x9E2, 0x344, 0x9B6, 0x370, 0x9B6,
                     0x36A, 0x3F5, 0x341, 0x3F0, 0x342, 0x3F0, 0x33E, 0x9E5,
                     0x341, 0x9BD, 0x366, 0x9E7, 0x343, 0x9BB, 0x36A, 0x3F1,
                     0x340, 0x9B8, 0x36E, 0x9E4, 0x33F, 0x9BC, 0x369, 0x3F5,
                     0x340, 0x3F0, 0x342, 0x3EF, 0x33F, 0x3CB, 0x365, 0x9B8,
                     0x33C, 0x424, 0x340, 0x3F0, 0x342, 0x3F0, 0x33D, 0x3F4,
                     0x33B, 0x3F5, 0x340, 0x3F1, 0x340, 0x3F1, 0x33F, 0x3F1,
                     0x33F, 0x9B9, 0x36E, 0x9E3, 0x340, 0x9BB, 0x36E, 0x9E3,
                     0x342, 0x9B8, 0x36F, 0x9E3, 0x33E, 0x9BD, 0x36A, 0x9BA,
                     0x36C, 0x3F3, 0x340, 0x3F2, 0x33F, 0x3F2, 0x33D, 0x3F3,
                     0x30C, 0x424, 0x33F, 0x3F3, 0x33E, 0x3F3, 0x33E, 0x3F1,
                     0x340, 0x9B8, 0x36F, 0x9E3, 0x340, 0x9BB, 0x36D, 0x9E5,
                     0x341, 0x9B9, 0x36C, 0x9E4, 0x340, 0x9BB, 0x36D, 0x9B8,
                     0x36A, 0x3F5, 0x33E, 0x3F4, 0x33D, 0x3F3, 0x33F, 0x3C9,
                     0x363, 0x3F5, 0x33F, 0x3F2, 0x33E, 0x3F4, 0x340, 0x3F0,
                     0x33E, 0x9BA, 0x36E, 0x9E3, 0x340, 0x9BB, 0x36C, 0x9E5,
                     0x341, 0x9BA, 0x36D, 0x9E4, 0x342, 0x9B7, 0x36C, 0x9BB,
                     0x33A, 0x425, 0x33F, 0x3F2, 0x33E, 0x3F3, 0x33C, 0x3F4,
                     0x33C, 0x3F5, 0x33E, 0x3F3, 0x340, 0x3F1, 0x33E, 0x3F2,
                     0x33D, 0x9BC, 0x36B, 0x9E4, 0x341, 0x9BA, 0x36D, 0x9E5,
                     0x340, 0x9BB, 0x33A, 0xA17, 0x33F, 0x9BA, 0x36C, 0x9BC,
                     0x369, 0x3F5, 0x33D, 0x3F2, 0x33F, 0x3F4, 0x33D, 0x3F3,
                     0x33C, 0x3F4, 0x33F, 0x3F2, 0x33E, 0x3F3, 0x33E, 0x3F3,
                     0x33C, 0x9BB, 0x36B, 0x9E6, 0x341, 0x9BA, 0x36B, 0x9E6,
                     0x341, 0x9BB, 0x36C, 0x9E4, 0x33F, 0x9BC, 0x339, 0x9ED,
                     0x368, 0x3F6, 0x33F, 0x9BA, 0x368, 0x9E8, 0x341, 0x3F3,
                     0x33D, 0x3F4, 0x33C, 0x3F4, 0x33D, 0x9E6, 0x30D, 0x427,
                     0x33D, 0x9BE, 0x367, 0x3F4, 0x30B, 0x426, 0x33B, 0x9C0,
                     0x361, 0x9ED, 0x341, 0x9BC, 0x367, 0x3F6, 0x33B, 0x9BC,
                     0x36B, 0x9E6, 0x340, 0x3F4, 0x30A, 0x426, 0x33A, 0x3F7,
                     0x33E, 0x3F4, 0x33A, 0x9BC, 0x36B, 0x9E6, 0x340, 0x9E7,
                     0x33F, 0x3F4, 0x33A, 0x9E9, 0x33F, 0x9C0, 0x363, 0x9EB,
                     0x33D, 0x9BF, 0x367, 0x3F5, 0x331, 0x400, 0x33B, 0x3F6,
                     0x339, 0x3F7, 0x33D, 0x3F5, 0x33B, 0x3F6, 0x33A, 0x3F6,
                     0x33A, 0x3F7, 0x339, 0x3F7, 0x33B, 0x3F7, 0x339, 0x3F6,
                     0x33A, 0x9C0, 0x367, 0x9E9, 0x30C, 0x9EE, 0x369, 0x9E8,
                     0x33F, 0x9BC, 0x36A, 0x9E7, 0x33C, 0x9BF, 0x369, 0x9BC,
                     0x367, 0x3F8, 0x33C, 0x3F5, 0x33A, 0x3F7, 0x339, 0x3F7,
                     0x339, 0x3F8, 0x33A, 0x3F7, 0x33A, 0x3F7, 0x33A, 0x3F6,
                     0x338, 0x9C1, 0x367, 0x9E9, 0x33D, 0x9BE, 0x368, 0x9E9,
                     0x33C, 0x9BE, 0x368, 0x9E9, 0x33E, 0x9BD, 0x367, 0x9BF,
                     0x367, 0x3F8, 0x339, 0x3F8, 0x33A, 0x3F7, 0x338, 0x3F8,
                     0x339, 0x9E9, 0x30E, 0x9F0, 0x334, 0x429, 0x339, 0x3F7,
                     0x33A, 0x9EA, 0x33B, 0x9C0, 0x368, 0x9E7, 0x33D, 0x9BE,
                     0x366, 0x3F9, 0x338, 0x3FA, 0x307, 0x9F0, 0x368, 0x9BE,
                     0x366, 0x9EC, 0x33C, 0x3F7, 0x337, 0x3F9, 0x307, 0x42A,
                     0x339, 0x3F7, 0x33A, 0x3F7, 0x338, 0x3F9, 0x33A, 0x3F7,
                     0x337, 0x3FA, 0x338, 0x9EB, 0x33A, 0x9C0, 0x368, 0x9E9,
                     0x33B, 0x9C1, 0x366, 0x9EA, 0x33B, 0x9BF, 0x367, 0x9C4,
                     0x330
          );
        pack( &packer, input, 595 );

        uint8_t expected[ 363 ];
        memset( expected, 0, sizeof(expected) );
        setBuffer8( expected, 363,
                    0xbb, 0xa7,
                    0x01, 0x7e, 0x9e, 0x03, 0x40,
                    0x01, 0x83, 0x7f, 0x0b, 0x55, 0x40, 0x7c, 0x84,
                    0x01, 0x7f, 0x84, 0x09, 0x55, 0x00, 0x9e, 0x7e,
                    0x01, 0x84, 0x7e, 0x22, 0x55, 0x55, 0x55, 0x55, 0x40, 0x9e, 0x80, 0x84, 0x7e,
                    0x01, 0x9e, 0x7e, 0x0c, 0x55, 0x50, 0x83, 0x7f,
                    0x01, 0x9e, 0x80, 0x12, 0x55, 0x55, 0x40,
                    0x01, 0x83, 0x7f, 0x0d, 0x55, 0x50,
                    0x01, 0x7c, 0x85, 0x07, 0x54,
                    0x01, 0x9e, 0x7e, 0x04, 0x50,
                    0x01, 0x85, 0x7e, 0x04, 0x50,
                    0x01, 0x9e, 0x80, 0x08, 0x55,
                    0x01, 0x85, 0x7e, 0x04, 0x50,
                    0x01, 0x9e, 0x80, 0x04, 0x50,
                    0x01, 0x84, 0x7c, 0x03, 0x40,
                    0x01, 0x80, 0x9e, 0x05, 0x50,
                    0x01, 0x84, 0x7e, 0x06, 0x54,
                    0x01, 0x9e, 0x80, 0x06, 0x74,
                    0x01, 0x84, 0x7e, 0x04, 0x50,
                    0x01, 0x9e, 0x7e, 0x06, 0x54, 0x84, 0x7e, 0x9e, 0x7e, 0x83, 0x7f,
                    0x01, 0x9e, 0x7e, 0x04, 0x50,
                    0x01, 0x85, 0x7e, 0x04, 0x50, 0x9e, 0x80,
                    0x01, 0x84, 0x7e, 0x0a, 0x55, 0x40,
                    0x01, 0x9f, 0x7e, 0x04, 0x50, 0x84, 0x7e,
                    0x01, 0x9e, 0x7e, 0x06, 0x54,
                    0x01, 0x84, 0x7e, 0x06, 0x54,
                    0x01, 0x9e, 0x7e, 0x08, 0x55, 0x84, 0x7e,
                    0x01, 0x9e, 0x80, 0x06, 0x54,
                    0x01, 0x84, 0x7e, 0x08, 0x55, 0x9e, 0x7e,
                    0x01, 0x85, 0x7e, 0x10, 0x55, 0x55,
                    0x01, 0x9e, 0x80, 0x10, 0x55, 0x55,
                    0x01, 0x84, 0x7e, 0x10, 0x55, 0x55,
                    0x01, 0x9e, 0x80, 0x10, 0x55, 0x55,
                    0x01, 0x84, 0x7e, 0x10, 0x55, 0x55,
                    0x01, 0x9e, 0x80, 0x10, 0x55, 0x55,
                    0x01, 0x85, 0x7e, 0x10, 0x55, 0x55,
                    0x01, 0x9e, 0x80, 0x10, 0x55, 0x55,
                    0x01, 0x84, 0x7e, 0x10, 0x55, 0x55,
                    0x01, 0x9e, 0x80, 0x10, 0x55, 0x55, 0x84, 0x7e,
                    0x01, 0x9e, 0x7f, 0x04, 0x50,
                    0x01, 0x84, 0x7e, 0x06, 0x54, 0x9e, 0x7c, 0x85, 0x7e, 0x9e, 0x7f,
                    0x01, 0x84, 0x7c, 0x04, 0x50,
                    0x01, 0x9e, 0x7f, 0x06, 0x54, 0x84, 0x7e,
                    0x01, 0x9e, 0x80, 0x04, 0x50,
                    0x01, 0x84, 0x7c, 0x08, 0x55,
                    0x01, 0x9e, 0x80, 0x06, 0x54, 0x84, 0x7e,
                    0x01, 0x9f, 0x7e, 0x08, 0x55,
                    0x01, 0x84, 0x7e, 0x16, 0x55, 0x55, 0x54,
                    0x01, 0x9e, 0x7f, 0x03, 0x40, 0x7c, 0x9f,
                    0x01, 0x80, 0x9e, 0x0b, 0x55, 0x40,
                    0x01, 0x84, 0x7e, 0x10, 0x55, 0x55,
                    0x01, 0x9e, 0x7f, 0x10, 0x55, 0x55,
                    0x01, 0x84, 0x7e, 0x08, 0x55,
                    0x01, 0x9f, 0x7c, 0x04, 0x50,
                    0x01, 0x85, 0x7e, 0x04, 0x50,
                    0x01, 0x9f, 0x7e, 0x08, 0x55,
                    0x01, 0x84, 0x7e, 0x04, 0x50,
                    0x01, 0x9f, 0x7f, 0x06, 0x54,
                    0x01, 0x84, 0x7e, 0x10, 0x55, 0x55,
                    0x01, 0x9f, 0x7e, 0x0e, 0x55, 0x54
                    );

        ok( (memcmp(buff, expected, 363) == 0), "compared ok" );
        ok( packer.length() == 368 ); // safe length
        // printf("length: %d\n", packer.length());
        // dump8( buff, 363 );

        // uint16_t unpacked[ 595 ];
        // unpack( &packer, unpacked, 595 );

        // IrPack is not reversible, this test fails, but fine
        // ok( (memcmp(unpacked, input, 595) == 0), "unpacked ok" );
        // dump16( unpacked, 595 );
    }

    done_testing();
}
