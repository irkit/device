#ifndef __AUTH_SWITCH_H__
#define __AUTH_SWITCH_H__

#include "BGLib.h" // for uint8

class AuthSwitch {
    public:
        AuthSwitch(int);
        void (*callback)(void);
        // bonding handle if there is stored bonding for this device 0xff otherwise
        uint8 currentBondHandle;

        void setup();
        bool isAuthorized();
        bool isFull();
        void authorize();
        void setCurrentBondHandle(uint8);
        void loop();
        void save();
        void clear();

    private:
        int pin;
};

#define INVALID_BOND_HANDLE 0xFF

#endif
