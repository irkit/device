#include <Arduino.h>
#include "longpressbutton.h"
#include "timer.h"

void long_press_button_loop( struct long_press_button_state_t* state ) {
    static uint8_t button_state = BUTTON_OFF;

    uint8_t next_button_state = digitalRead( state->pin );

    if ((BUTTON_OFF == button_state) &&
        (BUTTON_ON  == next_button_state)) {
        // OFF -> ON
        TIMER_START( state->timer, state->threshold_time );
    }
    else if ((BUTTON_ON == button_state) &&
             (BUTTON_ON == next_button_state)) {
        // still pressing
        if (TIMER_FIRED( state->timer )) {
            state->callback();
            button_state = BUTTON_OFF;
            return;
        }
    }
    button_state = next_button_state;
}

void long_press_button_ontimer( struct long_press_button_state_t* state ) {
    TIMER_TICK( state->timer );
}
