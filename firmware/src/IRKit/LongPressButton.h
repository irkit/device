#ifndef __LONGPRESSBUTTON_H__
#define __LONGPRESSBUTTON_H__

// reverse logic
#define BUTTON_ON  LOW
#define BUTTON_OFF HIGH

struct long_press_button_state_t {
    int              pin;
    uint8_t          threshold_time;
    volatile uint8_t timer;
    void             (*callback)();
};

#ifdef __cplusplus
extern "C" {
#endif

void long_press_button_loop( struct long_press_button_state_t* state );
void long_press_button_ontimer( struct long_press_button_state_t* state );

#ifdef  __cplusplus
}
#endif

#endif // __LONGPRESSBUTTON_H__
