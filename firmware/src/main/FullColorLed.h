#ifndef __FULLCOLORLED_H__
#define __FULLCOLORLED_H__

class FullColorLed
{
public:
    FullColorLed(int pinR, int pinG, int pinB);
    void SetLedColor(bool colorR, bool colorG, bool colorB);
    void SetLedColor(bool colorR, bool colorG, bool colorB, unsigned long cycle);
    void LedOff();
    void Loop();

private:
    int pinR_;
    int pinG_;
    int pinB_;
    bool colorR_;
    bool colorG_;
    bool colorB_;
    bool blinkOn_;
    unsigned long nextMillis_;
    unsigned long cycleMillis_;
};

#endif // __FULLCOLORLED_H__
