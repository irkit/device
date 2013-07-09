#ifndef __SET_SWITCH_H__
#define __SET_SWITCH_H__

#include <Arduino.h>

class SetSwitch {
    public:
        SetSwitch(int);
        void (*callback)();
        void (*clearCallback)();

        uint8_t count();
        uint8_t data(uint8_t);
        void setup();
        bool isMember(uint8_t);
        bool isFull();
        void add(uint8_t);
        void loop(uint8_t);
        void save();
        void clear();

    private:
        int pin;
};

#endif
