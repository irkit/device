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
