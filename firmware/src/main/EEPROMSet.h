#ifndef __EEPROMSET_H__
#define __EEPROMSET_H__

#include <Arduino.h>

// There can be a maximum of 8 bonded devices. The information related to the bonded devices is stored in the Flash memory, so it is persistent across resets and power-cycles.
// p158 or Bluetooth_Smart_API_11_11032013.pdf
#define EEPROMSET_MAX_NUMBER_OF_MEMBERS 8

// change version to invalidate stored data
#define EEPROMSET_NEWEST_VERSION 0x01

struct SavedData
{
    uint8_t members[EEPROMSET_MAX_NUMBER_OF_MEMBERS];
    uint8_t count;
    uint8_t version;
    uint8_t crc8;
};

struct CRCedData
{
    uint8_t members[EEPROMSET_MAX_NUMBER_OF_MEMBERS];
    uint8_t count;
    uint8_t version;
};

class EEPROMSet {
    public:
        EEPROMSet();

        void Setup();
        bool IsValid();
        bool IsMember(uint8_t);
        void Add(uint8_t);
        void Save();
        void Clear();
        void Dump();

        uint8_t crc();

    private:
        SavedData data;
};

#endif
