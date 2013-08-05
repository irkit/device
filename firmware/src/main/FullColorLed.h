#ifndef __FULLCOLORLED_H__
#define __FULLCOLORLED_H__

class FullColorLed
{
public:
    FullColorLed(int R_pin, int G_pin, int B_pin);
    void SetLedColor(bool REDColor, bool BLUEColor, bool GREENColor);
    void LedOff();

private:
    int REDLEDpin;
    int GREENLEDpin;
    int BLUELEDpin;
};

#endif // __FULLCOLORLED_H__
