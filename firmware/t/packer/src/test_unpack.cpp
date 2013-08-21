#include "IrPacker.h"
#include "nanotap.h"
#include "utils.h"

int main() {
    uint8_t length;
    uint8_t packed[512] = { 0 };
    uint16_t unpacked[256] = { 0 };
    uint8_t expected[512] = { 0 };

    ok( 1, "ok" );

    {
        IrPacker packer;
        memset( packed, 0, sizeof(packed) );
        memset( unpacked, 0, sizeof(unpacked) );
        SetBuffer8( packed, 1, 0xD7 );

        length = packer.Unpack( packed, unpacked, 1, 10 );

        ok( length == 1, "1 uint8_t turns into 1 uint16_t" );
        ok( unpacked[ 0 ] == 0x440D, "0xD7 turns into 0x440D" );
    }

    {
        IrPacker packer;
        memset( packed, 0, sizeof(packed) );
        memset( unpacked, 0, sizeof(unpacked) );
        SetBuffer8( packed, 1, 0xFF );

        length = packer.Unpack( packed, unpacked, 1, 10 );

        ok( length == 2, "0xFF turns into 2 uint16_t" );
        ok( unpacked[ 0 ] == 0xFFFF, "0xFF turns into 0xFFFF and," );
        ok( unpacked[ 1 ] == 0x0000, "0x0000" );
    }

    {
        IrPacker packer;
        memset( packed, 0, sizeof(packed) );
        memset( unpacked, 0, sizeof(unpacked) );
        SetBuffer8( packed, 4, 0xD7, 0xC3, 0x88, 0x87 );

        length = packer.Unpack( packed, unpacked, 4, 10 );

        ok( length == 4, "4 uint8_t turns into 4 uint16_t" );
        ok( unpacked[ 0 ] == 0x440D );
        ok( unpacked[ 1 ] == 0x2233 );
        ok( unpacked[ 2 ] == 0x047E );
        ok( unpacked[ 3 ] == 0x0457 );
    }

    {
        IrPacker packer;
        memset( packed, 0, sizeof(packed) );
        memset( unpacked, 0, sizeof(unpacked) );
        SetBuffer8( packed, 8, 0xD7, 0xC3, 0x00, 0x03, 0x00, 0x88, 0xFE, 0x00 );

        length = packer.Unpack( packed, unpacked, 8, 10 );

        ok( length == 5, "bit unpacked" );
        ok( unpacked[ 0 ] == 0x440D );
        ok( unpacked[ 1 ] == 0x2233 );
        ok( unpacked[ 2 ] == 0x047E );
        ok( unpacked[ 3 ] == 0x047E );
        ok( unpacked[ 4 ] == 0x047E );
    }

    {
        IrPacker packer;
        memset( packed, 0, sizeof(packed) );
        memset( unpacked, 0, sizeof(unpacked) );
        SetBuffer8( packed, 8, 0xD7, 0xC3, 0x00, 0x04, 0x00, 0x88, 0xA7, 0x10 );

        length = packer.Unpack( packed, unpacked, 8, 10 );

        ok( length == 6, "bit unpacked 2" );
        ok( unpacked[ 0 ] == 0x440D );
        ok( unpacked[ 1 ] == 0x2233 );
        ok( unpacked[ 2 ] == 0x047E );
        ok( unpacked[ 3 ] == 0x047E );
        ok( unpacked[ 4 ] == 0x047E );
        ok( unpacked[ 5 ] == 0x0D0D );
    }

    {
        IrPacker packer;
        memset( packed, 0, sizeof(packed) );
        memset( unpacked, 0, sizeof(unpacked) );

        SetBuffer8( packed, 4, 0xD7, 0xC3, 0x00, 0x04 );

        length = packer.Unpack( packed, unpacked, 4, 10 );

        ok( length == 2, "streaming unpack 1" );

        SetBuffer8( packed, 4, 0x00, 0x88, 0xA7, 0x10 );

        length = packer.Unpack( packed, unpacked + length, 4, 10 );

        ok( length == 4, "streaming unpack 2" );

        ok( unpacked[ 0 ] == 0x440D );
        ok( unpacked[ 1 ] == 0x2233 );
        ok( unpacked[ 2 ] == 0x047E );
        ok( unpacked[ 3 ] == 0x047E );
        ok( unpacked[ 4 ] == 0x047E );
        ok( unpacked[ 5 ] == 0x0D0D );
    }

    {
        IrPacker packer;
        memset( packed, 0, sizeof(packed) );
        memset( unpacked, 0, sizeof(unpacked) );
        memset( expected, 0, sizeof(expected) );

        SetBuffer8( packed, 4, 0xD7, 0xC3, 0x00, 0x04 );

        length = packer.Unpack( packed, unpacked, 4, 1 );

        ok( length == 1, "streaming unpack reached max" );

        ok( unpacked[ 0 ] == 0x440D );
        ok( unpacked[ 1 ] == 0x0000, "not filled" );
    }

    {
        IrPacker packer;
        memset( packed, 0, sizeof(packed) );
        memset( unpacked, 0, sizeof(unpacked) );

        SetBuffer8( packed, 4, 0xD7, 0xC3, 0x00, 0x04 );

        length = packer.Unpack( packed, unpacked, 4, 10 );

        ok( length == 2, "streaming unpack 1" );

        SetBuffer8( packed, 4, 0x00, 0x88, 0xA7, 0x10 );

        length = packer.Unpack( packed, unpacked + length, 4, 3 );

        ok( length == 3, "streaming unpack reached max" );

        ok( unpacked[ 0 ] == 0x440D );
        ok( unpacked[ 1 ] == 0x2233 );
        ok( unpacked[ 2 ] == 0x047E );
        ok( unpacked[ 3 ] == 0x047E );
        ok( unpacked[ 4 ] == 0x047E );
        ok( unpacked[ 5 ] == 0x0000, "not filled" );
    }

    done_testing();
}
