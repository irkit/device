#ifndef __MORSE_LISTENER__
#define __MORSE_LISTENER__

#include "Arduino.h"

class MorseListener {
public:
    MorseListener(int pin, uint16_t wps);
    void clear();
    void loop();

    // called when a letter is detected
    void (*letterCallback)(uint8_t letter);

    // called when a word space is detected
    void (*wordCallback)();

    // called when an error occured
    void (*errorCallback)();

private:
    int pin_;
    uint16_t wpm_;
    int8_t index_; // index of morseTable
    bool isOn_;
    bool wordStarted_;
    bool didCallLetterCallback_;
    unsigned long lastChanged_;

    uint16_t minDah_;
    uint16_t minLetterSpace_;
    uint16_t minWordSpace_;
};

#endif // __MORSE_LISTENER__
