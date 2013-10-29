#include "MorseListener.h"
#include "Global.h"
#include "pgmStrToRAM.h"

// #define DEBUG

// 0-1023
#define ON_MIN_THRESHOLD 800

prog_char morseTable[] PROGMEM = "ETIANMSURWDKGOHVF*L*PJBXCYZQ**54*3***2**+****16=/*****7***8*90*";

MorseListener::MorseListener(int pin, uint16_t wpm) :
    pin_(pin)
{
    setWPM(wpm);

    enabled_ = false;

    clear();
}

void MorseListener::clear() {
    index_                 = -1; // next index = (index + 1) * 2 + (isDah ? 1 : 0)
    isOn_                  = false;
    wordStarted_           = false;
    didCallLetterCallback_ = false;
    lastChanged_           = 0;
    lastOn_                = 0;
}

void MorseListener::setWPM(uint16_t wpm) {
    wpm_ = wpm;

    uint16_t t = 1200 / wpm_;
    debouncePeriod_ = t / 2;
    minLetterSpace_ = t * 2; // TODO: is this too short?
    minWordSpace_   = t * 4;
}

void MorseListener::setup() {
    // when 13:
    //  minLetterSpace_ 184
    //  minWordSpace_   369
#ifdef DEBUG
    Serial.print(P("t/2 debouncePeriod:")); Serial.println(debouncePeriod_);
    Serial.print(P("tx2 minLetterSpace:")); Serial.println(minLetterSpace_);
    Serial.print(P("tx4 minWordSpace:"));   Serial.println(minWordSpace_);
    float letter = 1200. / (float)wpm_;
    Serial.print(P("tx1 dit interval:")); Serial.println(letter);
    Serial.print(P("tx3 dah interval:")); Serial.println(letter * 3);
#endif
}

void MorseListener::loop() {
    if (! enabled_) {
        return;
    }

    int  raw   = analogRead(pin_);
    static bool input = false;
#ifdef DEBUG
    // Serial.print("raw: "); Serial.println(raw); // add delay when enabling this
#endif

    unsigned long interval = 0;

    // analogRead input is 1kHz audio
    // we smooth it here

    if ( raw > ON_MIN_THRESHOLD ) {
        input   = true;
        lastOn_ = global.now;
    }
    else if ( global.now - lastOn_ > debouncePeriod_ ) {
        input   = false;
    }
    else if ( global.now < lastOn_ ) {
        lastOn_ = 0; // just in case, millis() passed unsigned long limit
    }

    // check ON/OFF state change

    if ( input ) {
        // ON
        if ( ! isOn_ ) {
            // OFF -> ON
            if (wordStarted_) {
                // interval: duration of OFF time
                interval = global.now - lastChanged_;
            }
            isOn_        = true;
            lastChanged_ = global.now;
            wordStarted_ = true;

#ifdef DEBUG
            Serial.print(P("off->on: ")); Serial.println(interval);
            Serial.print(P(" raw: ")); Serial.println(raw);
#endif
        }
    }
    else {
        // OFF
        interval = global.now - lastChanged_;
        if ( isOn_ && wordStarted_ ) {
            // ON -> OFF
            // interval: duration of ON time
            isOn_                  = false;
            lastChanged_           = global.now;
            didCallLetterCallback_ = false; // can call again after 1st letter

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

    if ( (interval > 0) && (! isOn_) && (lastChanged_ == global.now) ) {
        // ON -> OFF
        // interval: duration of ON time

        index_ = (index_ + 1) * 2;
        // dah length == letter space length
        if (interval > minLetterSpace_) {
            // dah detected
            index_ ++;
        }
        else {
            // dit detected
        }
    }
    else if ( interval > 0 ) {
        // OFF -> ON
        // or OFF continuously

        // interval: duration of OFF time

        if ( ! wordStarted_ ) {
            // OFF continously
        }
        else if ( (! didCallLetterCallback_) && (interval > minLetterSpace_) ) {
            // detected letter space
            didCallLetterCallback_ = true;

#ifdef DEBUG
            Serial.print(P("index: ")); Serial.println(index_);
#endif

            char letter = pgm_read_byte_near(morseTable + index_);

            letterCallback( letter );

            // after letter detected
            index_ = -1;
        }
        else if ( interval > minWordSpace_ ) {
            // detected word space

            wordCallback();

            // after word detected
            clear();
        }
    }
}

void MorseListener::enable(bool enabled) {
    enabled_ = enabled;
}
