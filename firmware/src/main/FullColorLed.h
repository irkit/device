#ifndef __FULLCOLORLED_H__
#define __FULLCOLORLED_H__

class FullColorLed
{
public:
    FullColorLed(int pinR, int pinG, int pinB);
    void setLedColor(bool colorR, bool colorG, bool colorB);
    void setLedColor(bool colorR, bool colorG, bool colorB, bool blink);
    void setLedColor(bool colorR, bool colorG, bool colorB, bool blink, uint8_t blink_timeout);
    void off();
    void onTimer();

private:
    int pinR_;
    int pinG_;
    int pinB_;
    bool colorR_;
    bool colorG_;
    bool colorB_;
    bool isBlinking_; // defaults to off
    volatile bool blinkOn_; // altered inside timer ISR
    volatile uint8_t blink_timer_;
};

#endif // __FULLCOLORLED_H__
