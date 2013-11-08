#ifndef __MORSE_LISTENER__
#define __MORSE_LISTENER__

#include "Arduino.h"

class MorseListener {
public:
    MorseListener(int pin, uint16_t wps);
    void clear();
    void setWPM(uint16_t wpm);
    void setup();
    void loop();
    void enable(bool);

    // called when a letter is detected
    void (*letterCallback)(char letter);

    // called when a word space is detected
    void (*wordCallback)();

private:
    int pin_;
    uint16_t wpm_;
    int8_t index_; // index of morseTable
    bool enabled_;
    bool isOn_;
    bool wordStarted_;
    bool didCallLetterCallback_;
    unsigned long lastChanged_;
    unsigned long lastOn_;

    // if raw input was higher than threshold more than once per period: it's HIGH
    // if raw input was always lower than threshold for period: it's LOW
    uint16_t debouncePeriod_;
    uint16_t minLetterSpace_;
    uint16_t minWordSpace_;
};

#endif // __MORSE_LISTENER__
