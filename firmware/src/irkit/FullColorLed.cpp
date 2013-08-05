
#include "Arduino.h"
#include "FullColorLed.h"

//コンストラクタ
FullColorLed::FullColorLed(int R_pin, int G_pin, int B_pin)
{
    REDLEDpin=R_pin;
    pinMode(REDLEDpin,OUTPUT);
    
    GREENLEDpin=G_pin;
    pinMode(GREENLEDpin,OUTPUT);
    
    BLUELEDpin=B_pin;
    pinMode(BLUELEDpin,OUTPUT);
}
void FullColorLed::SetLedColor(bool REDColor, bool BLUEColor, bool GREENColor)
{
    digitalWrite(REDLEDpin, REDColor);
    digitalWrite(GREENLEDpin, BLUEColor);
    digitalWrite(BLUELEDpin, GREENColor);
}

void FullColorLed::LedOff()
{
    digitalWrite(REDLEDpin, LOW);
    digitalWrite(GREENLEDpin, LOW);
    digitalWrite(BLUELEDpin, LOW);
}

