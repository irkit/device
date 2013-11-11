#include "Arduino.h"
#include "pins.h"
#include "LongPressButton.h"
#include "Global.h"
#include "FlexiTimer2.h"
#include "timer.h"

LongPressButton button( RESET_SWITCH, 3 );

void longPressed() {
    Serial.println("long pressed");
}

// inside ISR, be careful
void onTimer() {
    button.onTimer();
}

void setup() {
    button.callback = &longPressed;

    // USB serial
    Serial.begin(115200);

    while (! Serial) ;

    FlexiTimer2::set( TIMER_INTERVAL, &onTimer );
    FlexiTimer2::start();
}

void loop() {
    global.loop();
    button.loop();

    delay(100);
    Serial.print(".");
}
