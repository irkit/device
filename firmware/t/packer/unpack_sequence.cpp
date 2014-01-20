#include "IrPacker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char sharedbuffer[ 400 ];
extern uint16_t tree[TREE_SIZE];

// Arduino can load from EEPROM, but we can't
void fill_tree() {
    uint16_t tree_[TREE_SIZE] = {
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
    memcpy( tree, tree_, sizeof(tree) );
}

void on_unpack(uint16_t out) {
    printf( "%d (%04x) ", out, out );
}

int main(int argc,char *argv[]) {
    volatile struct irpacker_t packer_state;
    irpacker_init( &packer_state, (volatile uint8_t*)sharedbuffer );
    fill_tree();
    irpacker_unpack_start( &packer_state );

    uint8_t binary[1024];
    memset(binary, 0, sizeof(binary));

    uint16_t offset_binary = 0;
    char hexstring[3];
    while (fgets(hexstring, 3, stdin)) {
        if (hexstring[0] == '\n') {
            break;
        }
        // printf("hex: %s %d\n", hexstring, hexstring[0]);
        uint8_t letter = strtol( (const char*)hexstring, NULL, 16 );
        binary[ offset_binary ++ ] = letter;
    }

    irpacker_unpack_sequence( &packer_state, binary, offset_binary, &on_unpack );

    exit(0);
}
