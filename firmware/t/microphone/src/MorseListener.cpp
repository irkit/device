#include "MorseListener.h"
#include "Global.h"

// #define DEBUG

// 0-1023
#define ON_MIN_THRESHOLD 700

prog_uchar morseTable[] PROGMEM = "ETIANMSURWDKGOHVF*L*PJBXCYZQ**54*3***2**+****16=/*****7***8*90*";

MorseListener::MorseListener(int pin, uint16_t wpm) :
    pin_(pin),
    wpm_(wpm)
{
    float t = (float)1200 / (float)wpm_;
    minLetterSpace_ = (uint16_t)( t * 2. );
    minWordSpace_   = (uint16_t)( t * 4. );

    enabled_ = false;

    clear();
}

void MorseListener::clear() {
    index_                 = -1; // next index = (index + 1) * 2 + (isDah ? 1 : 0)
    isOn_                  = false;
    wordStarted_           = false;
    didCallLetterCallback_ = false;
    lastChanged_           = 0;
}

void MorseListener::setup() {
#ifdef DEBUG
    // when 13:
    //  minLetterSpace_ 184
    //  minWordSpace_   369
    Serial.print("minLetter:"); Serial.println(minLetterSpace_);
    Serial.print("minWord:");   Serial.println(minWordSpace_);
#endif
}

void MorseListener::loop() {
    if (! enabled_) {
        return;
    }

    int           input    = analogRead(pin_);
#ifdef DEBUG
    Serial.print("input: "); Serial.println(input); // add delay when enabling this
#endif

    unsigned long interval = 0;

    // check ON/OFF state change

    if ( input > ON_MIN_THRESHOLD ) {
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
        }
    }
    else {
        // OFF
        interval = global.now - lastChanged_;
        if ( isOn_ ) {
            // ON -> OFF
            // interval: duration of ON time
            isOn_        = false;
            lastChanged_ = global.now;
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
            Serial.print("index: "); Serial.println(index_);
#endif

            uint8_t letter = pgm_read_byte_near(morseTable + index_);

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
