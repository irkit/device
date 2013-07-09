#include "SetSwitch.h"
#include <avr/eeprom.h>

//
// DESCRIPTION:
// provide a callback to be called when switch is pressed
// callback's return value (uint8_t) is added to a set,
// and persistently saved in eeprom
//

// There can be a maximum of 8 bonded devices. The information related to the bonded devices is stored in the Flash memory, so it is persistent across resets and power-cycles.
// p158 or Bluetooth_Smart_API_11_11032013.pdf
#define MAX_NUMBER_OF_MEMBERS 8

// if callback returns INVALID_KEY, it's not stored in set
#define INVALID_KEY 0xFF

// !SWITCH
#define ON  LOW
#define OFF HIGH

struct
{
    uint8_t members[MAX_NUMBER_OF_MEMBERS];
    uint8_t count;
} setData;

SetSwitch::SetSwitch(int pin_) :
    pin(pin_),
    callback(NULL),
    clearCallback(NULL),
    lastState(OFF),
    pressStarted(0)
{

}

void SetSwitch::setup()
{
    eeprom_read_block((void*)&setData, (void*)0, sizeof(setData));
}

bool SetSwitch::isMember(uint8_t key)
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

bool SetSwitch::isFull()
{
    return setData.count == MAX_NUMBER_OF_MEMBERS ? 1 : 0;
}

void SetSwitch::add(uint8_t key)
{
    setData.members[ setData.count ++ ] = key;
}

uint8_t SetSwitch::count()
{
    return setData.count;
}

uint8_t SetSwitch::data(uint8_t index)
{
    return setData.members[index];
}

void SetSwitch::loop(uint8_t key)
{
    uint8_t state = digitalRead(pin);
    if ( ON == state ) {
        if ( (lastState == ON) &&
             (millis() - pressStarted > 10*1000) )
        {
            // long press detected
            // millis() overflows, but who can press the button precisely when 32bit overflows?
            clear();
            if (clearCallback != 0) {
                clearCallback();
            }
        }
        else if ( (key != INVALID_KEY) &&
                  ! isMember(key) )
        {
            add(key);
            save();
            if ( callback != 0 ) {
                callback();
            }
        }
        else if (lastState == OFF)
        {
            pressStarted = millis();
        }
    }
    lastState = state;
}

void SetSwitch::save(void)
{
    eeprom_write_block((const void*)&setData, (void*)0, sizeof(setData));
}

void SetSwitch::clear(void)
{
    setData.count = 0;
    save();
}
