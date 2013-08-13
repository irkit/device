#ifndef __EEPROMSET_H__
#define __EEPROMSET_H__

#include <Arduino.h>

class EEPROMSet {
    public:
        EEPROMSet();

        uint8_t count();
        uint8_t data(uint8_t);
        void setup();
        bool isMember(uint8_t);
        bool isFull();
        void add(uint8_t);
        void save();
        void clear();

    private:
};

#endif
