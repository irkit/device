#ifndef __LONGPRESSBUTTON_H__
#define __LONGPRESSBUTTON_H__

// reverse logic
#define BUTTON_ON  LOW
#define BUTTON_OFF HIGH

class LongPressButton
{
public:
    LongPressButton(int pin, uint32_t longThreshold);
    void loop();

    // called when button pressed longer than threshold
    // will be called repeatedly with threshold interval
    void (*callback)();

private:
    int pin_;
    unsigned long longThreshold_;
    unsigned long buttonDownAt_;
    bool buttonState_;
};

#endif // __LONGPRESSBUTTON_H__
