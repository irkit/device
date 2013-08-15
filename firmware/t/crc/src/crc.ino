#include "Arduino.h"
#include "CRC8.h"
#include "EEPROMSet.h"

EEPROMSet set;

void setup() {
    Serial.begin(115200);

    set.Setup();
    set.Clear();
    set.Add( 0x33 );
    set.Save();
    Serial.print("set.crc(1): 0x"); Serial.println(set.crc(), HEX);
    Serial.print("IsValid should be 1: "); Serial.println(set.IsValid(), HEX);
    // set.Dump();

    set.Add( 0x34 );
    Serial.print("IsValid should be 0 before save: "); Serial.println(set.IsValid(), HEX);
    set.Save();
    Serial.print("set.crc(2) (should be different from 1): 0x"); Serial.println(set.crc(), HEX);
    Serial.print("IsValid should be 1 after save: "); Serial.println(set.IsValid(), HEX);
    // set.Dump();

    set.Clear();
    set.Add( 0x33 );
    set.Save();
    Serial.print("set.crc(1): 0x"); Serial.println(set.crc(), HEX);
    Serial.print("IsValid should be 1: "); Serial.println(set.IsValid(), HEX);
    // set.Dump();
}

void loop() {
}
