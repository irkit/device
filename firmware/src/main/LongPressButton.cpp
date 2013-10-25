#include <Arduino.h>
#include "LongPressButton.h"
#include "Global.h"

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
        buttonDownAt_ = global.now;
    }
    else if ((BUTTON_ON == buttonState_) &&
             (BUTTON_ON == nextButtonState)) {
        // still pressing
        if (global.now - buttonDownAt_ > longThreshold_) {
            callback();
            buttonState_ = BUTTON_OFF;
            return;
        }
    }
    buttonState_ = nextButtonState;
}
