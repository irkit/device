#include "EEPROMSet.h"
#include <avr/eeprom.h>
#include "pgmStrToRAM.h"

//
// DESCRIPTION:
// Save a unique set of unit8_t values
//

// There can be a maximum of 8 bonded devices. The information related to the bonded devices is stored in the Flash memory, so it is persistent across resets and power-cycles.
// p158 or Bluetooth_Smart_API_11_11032013.pdf
#define MAX_NUMBER_OF_MEMBERS 8

// if callback returns INVALID_KEY, it's not stored in set
// 0xFF is the value of "bonding" in BLE112's connection status event, when the connected device is not bonded
#define INVALID_KEY 0xFF

struct SavedData
{
    uint8_t members[MAX_NUMBER_OF_MEMBERS];
    uint8_t count;
    uint8_t version;
    uint8_t crc8;
} saved_data;

struct CRCedData
{
    uint8_t members[MAX_NUMBER_OF_MEMBERS];
    uint8_t count;
    uint8_t version;
};

EEPROMSet::EEPROMSet()
{
}

void EEPROMSet::Setup()
{
    eeprom_read_block((void*)&saved_data, (void*)0, sizeof(saved_data));
}

bool EEPROMSet::IsMember(uint8_t key)
{
    if (key == INVALID_KEY) {
        return 0;
    }
    for (uint8_t i=0; i<saved_data.count; i++) {
        if (saved_data.members[i] == key) {
            return 1;
        }
    }
    return 0;
}

// uniquely add
void EEPROMSet::Add(uint8_t key)
{
    for (uint8_t i=0; i<saved_data.count; i++) {
        if (saved_data.members[i] == key) {
            return;
        }
    }
    saved_data.members[ saved_data.count ++ ] = key;
}

void EEPROMSet::Save(void)
{
    eeprom_write_block((const void*)&saved_data, (void*)0, sizeof(saved_data));
}

void EEPROMSet::Clear(void)
{
    saved_data.count = 0;
    Save();
}

void EEPROMSet::Dump(void)
{
    Serial.print(P("authenticated bond: ")); Serial.println(saved_data.count, HEX);
    Serial.print(P("{ "));
    for (uint8_t i=0; i<saved_data.count; i++) {
        Serial.print(saved_data.members[i]);
        Serial.print(P(" "));
    }
    Serial.println(P("}"));
}
