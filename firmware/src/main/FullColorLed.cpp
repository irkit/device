#include "Arduino.h"
#include "FullColorLed.h"

FullColorLed::FullColorLed(int R_pin, int G_pin, int B_pin) :
    REDLEDpin(R_pin),
    GREENLEDpin(G_pin),
    BLUELEDpin(B_pin)
{
    pinMode(REDLEDpin,  OUTPUT);
    pinMode(GREENLEDpin,OUTPUT);
    pinMode(BLUELEDpin, OUTPUT);
}
void FullColorLed::SetLedColor(bool REDColor, bool BLUEColor, bool GREENColor)
{
    digitalWrite(REDLEDpin,   REDColor);
    digitalWrite(GREENLEDpin, BLUEColor);
    digitalWrite(BLUELEDpin,  GREENColor);
}

void FullColorLed::LedOff()
{
    digitalWrite(REDLEDpin,   LOW);
    digitalWrite(GREENLEDpin, LOW);
    digitalWrite(BLUELEDpin,  LOW);
}
