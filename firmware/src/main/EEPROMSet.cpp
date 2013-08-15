#include "EEPROMSet.h"
#include <avr/eeprom.h>
#include "pgmStrToRAM.h"
#include "CRC8.h"

//
// DESCRIPTION:
// Save a unique set of unit8_t values
//

EEPROMSet::EEPROMSet()
{
}

void EEPROMSet::Setup()
{
    eeprom_read_block((void*)&data, (void*)0, sizeof(data));
}

// crc8 is ok && version is ok
bool EEPROMSet::IsValid()
{
    uint8_t crc = crc8( (uint8_t*)&data, sizeof(CRCedData) );
    return (crc == data.crc8) && (EEPROMSET_NEWEST_VERSION == data.version);
}

bool EEPROMSet::IsMember(uint8_t key)
{
    for (uint8_t i=0; i<data.count; i++) {
        if (data.members[i] == key) {
            return 1;
        }
    }
    return 0;
}

// uniquely add
void EEPROMSet::Add(uint8_t key)
{
    for (uint8_t i=0; i<data.count; i++) {
        if (data.members[i] == key) {
            return;
        }
    }
    data.members[ data.count ++ ] = key;
}

void EEPROMSet::Save(void)
{
    data.version = EEPROMSET_NEWEST_VERSION;
    data.crc8    = crc8( (uint8_t*)&data, sizeof(CRCedData) );
    eeprom_write_block((const void*)&data, (void*)0, sizeof(data));
}

void EEPROMSet::Clear(void)
{
    data.count = 0;
    Save();
}

void EEPROMSet::Dump(void)
{
    Serial.print(P("set entries: ")); Serial.println(data.count, HEX);
    Serial.print(P("{ "));
    for (uint8_t i=0; i<data.count; i++) {
        Serial.print(data.members[i]);
        Serial.print(P(" "));
    }
    Serial.println(P("}"));
}

uint8_t EEPROMSet::crc(void)
{
    return data.crc8;
}
