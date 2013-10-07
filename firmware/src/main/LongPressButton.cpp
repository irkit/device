#include <Arduino.h>
#include "LongPressButton.h"

LongPressButton::LongPressButton(int pin, unsigned long longThreshold) :
    pin_(pin),
    longThreshold_(longThreshold)
{
    pinMode( pin_, INPUT );
    buttonState_ = BUTTON_OFF;
}

void LongPressButton::loop() {
    bool nextButtonState = digitalRead( pin_ );

    if ((BUTTON_OFF == buttonState_) &&
        (BUTTON_ON  == nextButtonState)) {
        // OFF -> ON
        buttonDownAt_ = millis();
    }
    else if ((BUTTON_ON == buttonState_) &&
             (BUTTON_ON == nextButtonState)) {
        // still pressing
        unsigned long now = millis();
        if (now - buttonDownAt_ > longThreshold_) {
            callback();
            buttonState_ = BUTTON_OFF;
            return;
        }
    }
    buttonState_ = nextButtonState;
}
