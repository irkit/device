#ifndef __EEPROMSET_H__
#define __EEPROMSET_H__

#include <Arduino.h>

class EEPROMSet {
    public:
        EEPROMSet();

        void Setup();
        bool IsMember(uint8_t);
        void Add(uint8_t);
        void Save();
        void Clear();
        void Dump();

    private:
};

#endif
