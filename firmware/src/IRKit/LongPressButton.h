#ifndef __LONGPRESSBUTTON_H__
#define __LONGPRESSBUTTON_H__

// reverse logic
#define BUTTON_ON  LOW
#define BUTTON_OFF HIGH

class LongPressButton
{
public:
    LongPressButton(int pin, uint8_t threshold_time);
    void loop();
    void onTimer(); // called from ISR

    // called when button pressed longer than threshold
    // will be called repeatedly with threshold interval
    void (*callback)();

private:
    int pin_;
    uint8_t threshold_time_;
    volatile uint8_t timer_;
};

#endif // __LONGPRESSBUTTON_H__
