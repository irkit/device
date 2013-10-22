#ifndef __FULLCOLORLED_H__
#define __FULLCOLORLED_H__

class FullColorLed
{
public:
    FullColorLed(int pinR, int pinG, int pinB);
    void setLedColor(bool colorR, bool colorG, bool colorB);
    void setLedColor(bool colorR, bool colorG, bool colorB, bool blink);
    void off();
    void toggleBlink();

private:
    int pinR_;
    int pinG_;
    int pinB_;
    bool colorR_;
    bool colorG_;
    bool colorB_;
    bool isBlinking_; // defaults to off
    volatile bool blinkOn_; // altered inside timer ISR
};

#endif // __FULLCOLORLED_H__
