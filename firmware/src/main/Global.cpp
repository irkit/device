#include "Arduino.h"
#include "Global.h"

Global::Global() :
    now(0),
    buffer_mode(GBufferModeUnused)
{
}

void Global::loop() {
    now = millis();
}

Global global;
