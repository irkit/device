#include "Arduino.h"
#include "pins.h"
#include "longpressbutton.h"
#include "timer.h"

struct long_press_button_state_t long_press_button_state;

void long_pressed() {
    Serial.println("long pressed");
}

// inside ISR, be careful
void on_timer() {
    long_press_button_ontimer( &long_press_button_state );
}

void setup() {
    //--- initialize timer

    timer_init( on_timer );
    timer_start( TIMER_INTERVAL );

    //--- initialize long press button

    pinMode( CLEAR_BUTTON, INPUT );
    long_press_button_state.pin            = CLEAR_BUTTON;
    long_press_button_state.callback       = &long_pressed;
    long_press_button_state.threshold_time = 5;

    // USB serial
    Serial.begin(115200);

    while (! Serial) ;
}

void loop() {
    delay(100);
    Serial.print(".");
}
