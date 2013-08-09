#ifndef __PINS_H__
#define __PINS_H__

#define BLE112_TX       A3 // pin26 -> BLE112 P0_5 RX
#define BLE112_RX       A4 // pin27 -> BLE112 P0_4 TX
#define BLE112_RESET    A2 // pin25 -> BLE112 !BLE/RST

#define IR_IN            8 // pin12 PB0 ICP1 Counter1 (PCINT0/CLKO/ICP1)
#define IR_OUT           3 // pin1  PD3 OC2B Timer2 (PCINT19/OC2B/INT1)

#define AUTH_SWITCH      2 // pin32

#define FULLCOLOR_LED_R  5 // pin9
// temporary toggle to fix circuit's wrong wiring
#define FULLCOLOR_LED_B  6 // pin10
#define FULLCOLOR_LED_G  7 // pin11

#endif
