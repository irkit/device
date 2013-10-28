#include "Arduino.h"
#include "Global.h"

Global::Global() :
    now(0)
{
}

void Global::loop() {
    now = millis();
}

Global global;

// must be more than: 16Byte x 8bit/Byte x 2(HIGH and LOW) x uint16_t
volatile char gBuffer[ 1024 ];
GBufferMode gBufferMode;
