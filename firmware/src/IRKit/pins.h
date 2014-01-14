/*
 Copyright (C) 2013-2014 Masakazu Ohtsuka
  
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
  
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
  
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __PINS_H__
#define __PINS_H__

#include <Arduino.h>

// #define BLE112_TX       A3 // pin26 -> BLE112 P0_5 RX
// #define BLE112_RX       A4 // pin27 -> BLE112 P0_4 TX
// #define BLE112_RESET    A2 // pin25 -> BLE112 !BLE/RST
#define WIFI_TX          1 // D1 pin21
#define WIFI_RX          0 // D0 pin20

// #define IR_IN            8 // pin12 PB0 ICP1 Counter1 (PCINT0/CLKO/ICP1)
// #define IR_OUT           3 // pin1  PD3 OC2B Timer2 (PCINT19/OC2B/INT1)
#define IR_IN           13 // pin32 PC7 (ICP3/CLK0/OC4A)
#define IR_OUT          10 // pin30 PB6 (PCINT6/OC1B/OC4B/ADC13)

#define FULLCOLOR_LED_R  6 // pin27 PD7 (T0/OC4D/ADC10)
#define FULLCOLOR_LED_G  9 // pin29 PB5 (PCINT5/OC1A/!OC4B/ADC12)
#define FULLCOLOR_LED_B  8 // pin28 PB4 (PCINT4/ADC11)

#define LDO33_ENABLE     7 // pin1

#define MICROPHONE      A4 // pin40 (ADC1)PF1
// #define MICROPHONE      A2 // v1.1.0's A4 was connnected to Wifi, use A2 on v1.1.0 for testing
#define CLEAR_BUTTON    A5 // pin41 (ADC0)PF0

#endif
