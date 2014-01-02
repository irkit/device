#include "morse.h"
#include "pgmStrToRAM.h"

// #define DEBUG

// 0-1023
#define ON_MIN_THRESHOLD 700

// # standard morse tree
// T _    M _ _    O _ _ _    CH _ _ _ _
//                            Ö _ _ _ .
//                 G _ _ .    Q _ _ . _
//                            Z _ _ . .
//        N _ .    K _ . _    Y _ . _ _
//                            C _ . _ .
//                 D _ . .    X _ . . _
//                            B _ . . .
// E .    A . _    W . _ _    J . _ _ _
//                            P . _ _ .
//                 R . _ .    Ä . _ . _
//                            L . _ . .
//        I . .    U . . _    Ü . . _ _
//                            F . . _ .
//                 S . . .    V . . . _
//                            H . . . .

// # hex+/ morse tree
// # 4,5 should be used more frequently than others because they're used in ascii alphabet
// 4 _    3 _ _    0 _ _ _    ? _ _ _ _
//                            ? _ _ _ .
//                 1 _ _ .    ? _ _ . _
//                            ? _ _ . .
//        6 _ .    8 _ . _    ? _ . _ _
//                            ? _ . _ .
//                 9 _ . .    ? _ . . _
//                            A _ . . .
// 5 .    2 . _    B . _ _    ? . _ _ _
//                            ? . _ _ .
//                 C . _ .    ? . _ . _
//                            ? . _ . .
//        7 . .    D . . _    ? . . _ _
//                            ? . . _ .
//                 E . . .    F . . . _
//                            / . . . .

// prog_char morseTable[] PROGMEM = "ETIANMSURWDKGOHVF*L*PJBXCYZQ**54*3***2**+****16=/*****7***8*90*";
// morse table specialized to hex+/ [0-9A-F/]
prog_char morseTable[] PROGMEM = "547263EDCB9810/F******A*******";

extern unsigned long now;

static void setWPM(struct morse_t *state, uint16_t wpm) {
    state->wpm = wpm;

    // when 13:
    //  min_letter_space_ 184
    //  min_word_space_   369
    uint16_t t = 1200 / state->wpm;
    state->debounce_period  = t / 2;
    state->min_letter_space = t * 2;  // TODO: is this too short?
    state->min_word_space   = t * 4;

#ifdef DEBUG
    Serial.print(P("t/2 debouncePeriod:")); Serial.println(debounce_period);
    Serial.print(P("tx2 minLetterSpace:")); Serial.println(min_letter_space);
    Serial.print(P("tx4 minWordSpace:"));   Serial.println(min_word_space);
    float letter = 1200. / (float)wpm_;
    Serial.print(P("tx1 dit interval:")); Serial.println(letter);
    Serial.print(P("tx3 dah interval:")); Serial.println(letter * 3);
#endif
}

static void clear( struct morse_t *state ) {
    state->index                    = -1; // next index = (index + 1) * 2 + (isDah ? 1 : 0)
    state->is_on                    = false;
    state->word_started             = false;
    state->did_call_letter_callback = false;
    state->last_changed             = 0;
    state->last_on                  = 0;
}

void morse_setup ( struct morse_t *state, uint16_t wpm) {
    setWPM( state, wpm );
    clear( state );

    state->enabled = false;
}

void morse_loop( struct morse_t *state ) {
    if (! state->enabled) {
        return;
    }

    int  raw   = analogRead(state->pin);
    static bool input = false;
#ifdef DEBUG
    Serial.print("raw: "); Serial.println(raw); // add delay when enabling this
    delay(1);
#endif

    unsigned long interval = 0;

    // analogRead input is 1kHz audio
    // we smooth it here

    if ( raw > ON_MIN_THRESHOLD ) {
        input    = true;
        state->last_on = now;
    }
    else if ( now - state->last_on > state->debounce_period ) {
        input    = false;
    }
    else if ( now < state->last_on ) {
        state->last_on = 0; // just in case, millis() passed unsigned long limit
    }

    // check ON/OFF state change

    if ( input ) {
        // ON
        if ( ! state->is_on ) {
            // OFF -> ON
            if (state->word_started) {
                // interval: duration of OFF time
                interval = now - state->last_changed;
            }
            state->is_on        = true;
            state->last_changed = now;
            state->word_started = true;

#ifdef DEBUG
            Serial.print(P("off->on: ")); Serial.println(interval);
            Serial.print(P(" raw: ")); Serial.println(raw);
#endif
        }
    }
    else {
        // OFF
        interval = now - state->last_changed;
        if ( state->is_on && state->word_started ) {
            // ON -> OFF
            // interval: duration of ON time
            state->is_on                    = false;
            state->last_changed             = now;
            state->did_call_letter_callback = false; // can call again after 1st letter

#ifdef DEBUG
            Serial.print(P("on->off: ")); Serial.println(interval);
            Serial.print(P(" raw: ")); Serial.println(raw);
#endif
        }
        else {
            // OFF continously
            // interval: duration of OFF time
        }
    }

    // decode

    if ( (interval > 0) && (! state->is_on) && (state->last_changed == now) ) {
        // ON -> OFF
        // interval: duration of ON time

        state->index = (state->index + 1) * 2;
        // dah length == letter space length
        if (interval > state->min_letter_space) {
            // dah detected
            state->index ++;
        }
        else {
            // dit detected
        }
    }
    else if ( interval > 0 ) {
        // OFF -> ON
        // or OFF continuously

        // interval: duration of OFF time

        if ( ! state->word_started ) {
            // OFF continously
        }
        else if ( (! state->did_call_letter_callback) && (interval > state->min_letter_space) ) {
            // detected letter space
            state->did_call_letter_callback = true;

#ifdef DEBUG
            Serial.print(P("index: ")); Serial.println(index_);
#endif

            char letter = pgm_read_byte_near(morseTable + state->index);

            state->letter_callback( letter );

            // after letter detected
            state->index = -1;
        }
        else if ( interval > state->min_word_space ) {
            // detected word space

            state->word_callback();

            // after word detected
            clear( state );
        }
    }
}

void morse_enable( struct morse_t *state, bool enabled ) {
    state->enabled = enabled;
}
