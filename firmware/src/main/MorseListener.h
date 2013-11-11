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
    int           pin_;
    uint16_t      wpm_;
    int8_t        index_;       // index of morseTable
    bool          enabled_;
    bool          is_on_;
    bool          word_started_;
    bool          did_call_letter_callback_;
    unsigned long last_changed_;
    unsigned long last_on_;

    // if raw input was higher than threshold more than once per period: it's HIGH
    // if raw input was always lower than threshold for period: it's LOW
    uint16_t      debounce_period_;
    uint16_t      min_letter_space_;
    uint16_t      min_word_space_;
};

#endif // __MORSE_LISTENER__
