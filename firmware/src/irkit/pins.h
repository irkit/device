#ifndef __PINS_H__
#define __PINS_H__

#define BUSY_LED  13

// NOTE: this demo REQUIRES the BLE112 be programmed with the UART connected
// to the "api" endpoint in hardware.xml, have "mode" set to "packet", and be
// configured for 38400 baud, 8/N/1. This may change in the future, but be
// aware. The BLE SDK archive contains an /examples/uartdemo project which is
// a good starting point for this communication, though some changes are
// required to enable packet mode and change the baud rate. The BGLib
// repository also includes a project you can use for this in the folder
// /BLEFirmware/BGLib_U1A1P_38400_noflow_wake16_hwake15.

// BLE112 module connections:
// - BLE P0_2 -> GND (CTS tied to ground to bypass flow control)
// - BLE P0_4 -> Arduino Digital Pin 6 (BLE TX -> Arduino soft RX)
// - BLE P0_5 -> Arduino Digital Pin 5 (BLE RX -> Arduino soft TX)
#define BLE112_RX  6
#define BLE112_TX  5

#define IR_IN      8 // PB0 pin12 ICP1 Counter1 (PCINT0/CLKO/ICP1)
#define IR_OUT     3 // PD3 pin1  OC2B Timer2 (PCINT19/OC2B/INT1)

#define AUTH_SWITCH 10 // pin14

#endif
