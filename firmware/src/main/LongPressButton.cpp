#include <Arduino.h>
#include "LongPressButton.h"
#include "Global.h"

LongPressButton::LongPressButton(int pin, unsigned long long_threshold) :
    pin_(pin),
    long_threshold_(long_threshold)
{
    pinMode( pin_, INPUT );
    button_state_ = BUTTON_OFF;
}

void LongPressButton::loop() {
    bool next_button_state = digitalRead( pin_ );

    if ((BUTTON_OFF == button_state_) &&
        (BUTTON_ON  == next_button_state)) {
        // OFF -> ON
        button_down_at_ = global.now;
    }
    else if ((BUTTON_ON == button_state_) &&
             (BUTTON_ON == next_button_state)) {
        // still pressing
        // forget overflow, it won't happen within between button down/up
        if (global.now - button_down_at_ > long_threshold_) {
            callback();
            button_state_ = BUTTON_OFF;
            return;
        }
    }
    button_state_ = next_button_state;
}
