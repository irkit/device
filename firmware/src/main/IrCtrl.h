#ifndef __IRCTRL_H__
#define __IRCTRL_H__

/*----------------------------------------------------------------------------/
/  IR_CTRL - IR remote control module                         (C)ChaN, 2008
/-----------------------------------------------------------------------------/
/  Common include file for IR_CTRL module and application
/----------------------------------------------------------------------------*/

#include <stdint.h>

/* Put hardware dependent include files here */
#include <avr/io.h>
#include <avr/interrupt.h>

/* Enable/Disable transmission/reception functions <1/0> */
#define IR_USE_XMIT     1
#define IR_USE_RCVR     1
#define IR_USE_NEC      1
#define IR_USE_AEHA     1
#define IR_USE_SONY     1

// Structure of IR function work area
// buff size must be at least 259 to store 16byte of ir data
#define IR_BUFF_SIZE       512
#define IR_DEFAULT_CARRIER 38

/* IR control state */
#define IR_IDLE        0    /* In idle state, ready to receive/transmit */
#define IR_RECVING     1    /* An IR frame is being received */
#define IR_RECVED      2    /* An IR frame has been received and data is valid */
#define IR_READING     4    /* BLE central is reading IR data, can't receive IR */
#define IR_WRITING     10   /* BLE central is wrinting IR data, can't receive IR */
#define IR_XMITTING    11   /* IR transmission is in progress */
#define IR_DISABLED    0xFF /* disabled */

typedef void (*IRReceiveCallback)();

typedef struct _irstruct {
    uint8_t   enabled;
    uint8_t   state;        // Communication state
    uint8_t   trailer_count; // Number of T_TRAIL time to wait to determine signal ended
    uint8_t   freq;         // carrier wave freq in kHz
    uint8_t   xmit_timer;   // xmit timeout timer
    uint8_t   recv_timer;   // recv timeout timer
    bool      did_receive;  // just received IR signal now
    IRReceiveCallback on_receive;
    uint16_t  len;          // Size of buff used
    uint16_t  tx_index;      // 0 < tx_index < len
    uint16_t *buff;         // pointer to global buffer
} IR_STRUCT;

/* The work area for IR_CTRL is defined in ir_ctrl.c */
extern
volatile IR_STRUCT IrCtrl;

/* Prototypes */
void IR_initialize (IRReceiveCallback on_receive);
int IR_xmit (void);
void IR_put (uint16_t);
void IR_timer (void);
void IR_loop (void);
void IR_state (uint8_t);
void IR_dump (void);

#endif // __IRCTRL_H__
