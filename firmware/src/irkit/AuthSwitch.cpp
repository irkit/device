#include "AuthSwitch.h"
#include <avr/eeprom.h>
#include "pgmStrToRAM.h"

//
// DESCRIPTION:
// save ble_msg_connection_status_evt_t.bonding whenever new connection is made
// if user presses the switch,
// set the current connected bonding as "authorized"
// and save it inside persistent eeprom
//

// List all bonded devices. There can be a maximum of 8 bonded devices. The information related to the bonded devices is stored in the Flash memory, so it is persistent across resets and power-cycles.
// p158 or Bluetooth_Smart_API_11_11032013.pdf
#define MAX_NUMBER_OF_BONDED_DEVICES 8

// !AUTH_SWITCH
#define ON  LOW
#define OFF HIGH

struct
{
    uint8 authorized[MAX_NUMBER_OF_BONDED_DEVICES]; // bond handle
    uint8 count;
} authData;

AuthSwitch::AuthSwitch(int pin_) :
    pin(pin_),
    currentBondHandle(INVALID_BOND_HANDLE),
    callback(0)
{

}

void AuthSwitch::setup()
{
    eeprom_read_block((void*)&authData, (void*)0, sizeof(authData));
}

bool AuthSwitch::isAuthorized(void)
{
    if (currentBondHandle == INVALID_BOND_HANDLE) {
        return 0;
    }
    for (uint8_t i=0; i<authData.count; i++) {
        if (authData.authorized[i] == currentBondHandle) {
            return 1;
        }
    }
    return 0;
}

bool AuthSwitch::isFull()
{
    return authData.count == MAX_NUMBER_OF_BONDED_DEVICES ? 1 : 0;
}

void AuthSwitch::authorize(void)
{
    if ( isAuthorized() ) {
        return;
    }

    authData.authorized[ authData.count ++ ] = currentBondHandle;

    save();

    if ( callback ) {
        callback();
    }
}

void AuthSwitch::setCurrentBondHandle(uint8 bond)
{
    currentBondHandle = bond;
}

void AuthSwitch::loop(void)
{
    if ( (ON == digitalRead(pin)) &&
         (currentBondHandle != INVALID_BOND_HANDLE) ) {
        authorize();
    }
}

void AuthSwitch::save(void)
{
    eeprom_write_block((const void*)&authData, (void*)0, sizeof(authData));
}

void AuthSwitch::clear(void)
{
    authData.count = 0;
    save();
}
