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
        printf("%d 0x%x\n", i, buff[i]);
    }
}

void dump16( uint16_t *buff, uint16_t length ) {
    printf("dump16:\n");
    for (uint16_t i=0; i<length; i++) {
        printf("%d 0x%x\n", i, buff[i]);
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

    done_testing();
}
