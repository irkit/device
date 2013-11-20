#include "IrPacker.h"
#include "nanotap.h"
#include "utils.h"
#include "string.h"

int main() {
    ok( 1, "ok" );

    {
        IrPacker packer;
        uint16_t input = 17946; // 0x461A

        uint8_t packed = packer.pack( input );
        printf( "input: %d 0x%x\n", input, input );
        printf( "packd: %d 0x%x\n", packed, packed );

        ok( packed == 215 );
    }

    {
        IrPacker packer;
        uint16_t input = 30;

        uint8_t packed = packer.pack( input );
        printf( "input: %d 0x%x\n", input, input );
        printf( "packd: %d 0x%x\n", packed, packed );

        ok( packed == 30 );
    }

    {
        IrPacker packer;
        uint16_t input = 65000;

        uint8_t packed = packer.pack( input );
        printf( "input: %d 0x%x\n", input, input );
        printf( "packd: %d 0x%x\n", packed, packed );

        ok( packed == 253 );
    }

    {
        IrPacker packer;
        uint16_t input = 60108;

        uint8_t packed = packer.pack( input );
        printf( "input: %d 0x%x\n", input, input );
        printf( "packd: %d 0x%x\n", packed, packed );

        ok( packed == 251 );
    }

    {
        IrPacker packer;
        uint16_t input;
        char buf[100];

        for (uint8_t i=0; i<224; i++) {
            input = IrPacker::tree[ i ];
            uint8_t packed = packer.pack( input );
            sprintf( buf, "%d (0x%x) --pack-> %d (0x%x)", input, input, packed, packed );
            ok( packed == i + 30, buf );

            uint16_t unpacked = packer.unpack( packed );
            sprintf( buf, "%d (0x%x) --unpack-> %d (0x%x)", packed, packed, unpacked, unpacked );
            ok( unpacked == input, buf );
        }
    }

    done_testing();
}
