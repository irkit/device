#include "Arduino.h"
#include "pgmStrToRAM.h"
#include "MemoryFree.h"
#include "IrPacker.h"

void setup() {
    while ( ! Serial ) ; // wait for leonardo

    IrPacker packer;
    uint8_t packed = packer.pack( 30 );
    uint16_t unpacked = packer.unpack( packed );
    Serial.print("packed: "); Serial.println(packed);
    Serial.print("unpacked: "); Serial.println(unpacked);

    Serial.print("freeMemory: "); Serial.println(freeMemory());
}

void loop() {
}
