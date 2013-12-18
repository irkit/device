#ifndef __MORSE_LISTENER__
#define __MORSE_LISTENER__

#include "Arduino.h"
#include "stdbool.h"

struct morse_t {
    // called when a letter is detected
    void (*letter_callback)(char letter);
    // called when a word space is detected
    void (*word_callback)();
    int           pin;
    uint16_t      wpm;
    int8_t        index;       // index of morseTable
    bool          enabled;
    bool          is_on;
    bool          word_started;
    bool          did_call_letter_callback;
    unsigned long last_changed;
    unsigned long last_on;

    // if raw input was higher than threshold more than once per period: it's HIGH
    // if raw input was always lower than threshold for period: it's LOW
    uint16_t      debounce_period;
    uint16_t      min_letter_space;
    uint16_t      min_word_space;
};

#ifdef __cplusplus
extern "C" {
#endif

void morse_setup( struct morse_t *state, uint16_t wpm );
void morse_loop( struct morse_t *state );
void morse_enable( struct morse_t *state, bool enabled );

#ifdef __cplusplus
}
#endif

#endif
