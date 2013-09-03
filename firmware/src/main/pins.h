#ifndef __PINS_H__
#define __PINS_H__

#include <Arduino.h>

// #define BLE112_TX       A3 // pin26 -> BLE112 P0_5 RX
// #define BLE112_RX       A4 // pin27 -> BLE112 P0_4 TX
// #define BLE112_RESET    A2 // pin25 -> BLE112 !BLE/RST
#define WIFI_TX         A4 // pin40
#define WIFI_RX         A5 // pin41

// #define IR_IN            8 // pin12 PB0 ICP1 Counter1 (PCINT0/CLKO/ICP1)
// #define IR_OUT           3 // pin1  PD3 OC2B Timer2 (PCINT19/OC2B/INT1)
#define IR_IN           13 // pin32 PC7 (ICP3/CLK0/OC4A)
#define IR_OUT          10 // pin30 PB6 (PCINT6/OC1B/OC4B/ADC13)

#define FULLCOLOR_LED_R  6 // pin27 PD7 (T0/OC4D/ADC10)
#define FULLCOLOR_LED_G  9 // pin29 PB5 (PCINT5/OC1A/!OC4B/ADC12)
#define FULLCOLOR_LED_B  8 // pin28 PB4 (PCINT4/ADC11)

#endif
