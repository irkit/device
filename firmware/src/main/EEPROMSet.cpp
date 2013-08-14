#include "EEPROMSet.h"
#include <avr/eeprom.h>

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

struct
{
    uint8_t members[MAX_NUMBER_OF_MEMBERS];
    uint8_t count;
} setData;

EEPROMSet::EEPROMSet()
{
}

void EEPROMSet::setup()
{
    eeprom_read_block((void*)&setData, (void*)0, sizeof(setData));
}

bool EEPROMSet::isMember(uint8_t key)
{
    if (key == INVALID_KEY) {
        return 0;
    }
    for (uint8_t i=0; i<setData.count; i++) {
        if (setData.members[i] == key) {
            return 1;
        }
    }
    return 0;
}

bool EEPROMSet::isFull()
{
    return setData.count == MAX_NUMBER_OF_MEMBERS ? 1 : 0;
}

// uniquely add
void EEPROMSet::add(uint8_t key)
{
    for (uint8_t i=0; i<setData.count; i++) {
        if (setData.members[i] == key) {
            return;
        }
    }
    setData.members[ setData.count ++ ] = key;
}

uint8_t EEPROMSet::count()
{
    return setData.count;
}

uint8_t EEPROMSet::data(uint8_t index)
{
    return setData.members[index];
}

void EEPROMSet::save(void)
{
    eeprom_write_block((const void*)&setData, (void*)0, sizeof(setData));
}

void EEPROMSet::clear(void)
{
    setData.count = 0;
    save();
}
