#include <Arduino.h>
#include "LongPressButton.h"
#include "Global.h"
#include "timer.h"

LongPressButton::LongPressButton(int pin, uint8_t threshold_time) :
    pin_(pin),
    threshold_time_(threshold_time)
{
    pinMode( pin_, INPUT );
}

void LongPressButton::loop() {
    static bool button_state = BUTTON_OFF;

    bool next_button_state = digitalRead( pin_ );

    if ((BUTTON_OFF == button_state) &&
        (BUTTON_ON  == next_button_state)) {
        // OFF -> ON
        TIMER_START( timer_, threshold_time_ );
    }
    else if ((BUTTON_ON == button_state) &&
             (BUTTON_ON == next_button_state)) {
        // still pressing
        if (TIMER_FIRED( timer_ )) {
            callback();
            button_state = BUTTON_OFF;
            return;
        }
    }
    button_state = next_button_state;
}

void LongPressButton::onTimer() {
    TIMER_TICK( timer_ );
}
