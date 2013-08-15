#include "IrPacker.h"
#include "nanotap.h"
#include "utils.h"

int main() {
    uint8_t length;
    uint16_t buff[256] = { 0 };
    uint8_t packed[512] = { 0 };

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
        SetBuffer16( buff, 5, 0x461A, 0x231D, 0x491, 0x476, 0x0490 );

        length = packer.Pack( buff, packed, 5 );

        ok( length == 8, "bit packed" );
        ok( packed[ 0 ] == 0xD7 );
        ok( packed[ 1 ] == 0xC3 );
        ok( packed[ 2 ] == 0x00 );
        ok( packed[ 3 ] == 0x03 );
        ok( packed[ 4 ] == 0x00 );
        ok( packed[ 5 ] == 0x88 );
        ok( packed[ 6 ] == 0xFE ); // not defined
        ok( packed[ 7 ] == 0x00 );
    }

    {
        IrPacker packer;
        memset( buff, 0, sizeof(buff) );
        memset( packed, 0, sizeof(packed) );
        SetBuffer16( buff, 6, 0x461A, 0x231D, 0x491, 0x476, 0x0490, 0x0D24 );

        length = packer.Pack( buff, packed, 6 );

        ok( length == 8, "bit packed 2" );
        ok( packed[ 0 ] == 0xD7 );
        ok( packed[ 1 ] == 0xC3 );
        ok( packed[ 2 ] == 0x00 );
        ok( packed[ 3 ] == 0x04 );
        ok( packed[ 4 ] == 0x00 );
        ok( packed[ 5 ] == 0x88 );
        ok( packed[ 6 ] == 0xA7 );
        ok( packed[ 7 ] == 0x10 ); // 0b00010000
    }

    done_testing();
}
