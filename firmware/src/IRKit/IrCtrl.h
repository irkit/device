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
#include "const.h"

// Structure of IR function work area
// Known most longest IR data uses 363bytes (after compressed using IrPacker)
#define IR_BUFF_SIZE       SHARED_BUFFER_SIZE

#define IR_DEFAULT_CARRIER 38

/* IR control state */
#define IR_IDLE        0    /* In idle state, ready to receive/transmit */
#define IR_RECVING     1    /* An IR frame is being received */
#define IR_RECVED      2    /* An IR frame has been received and data is valid */
#define IR_RECVED_IDLE 3    /* An IR frame is POSTed to server. Still holds IR data, but ready to receive next */
#define IR_WRITING     10   /* somebody is wrinting IR data, can't receive IR */
#define IR_XMITTING    11   /* IR transmission is in progress */
#define IR_READING     20
#define IR_DISABLED    0xFF /* disabled */

typedef void (*IRReceiveCallback)();

typedef struct _irstruct {
    uint8_t           enabled;
    uint8_t           state;    // Communication state
    uint8_t           trailer_count; // Number of T_TRAIL time to wait to determine signal ended
    uint8_t           freq;     // carrier wave freq in kHz
    uint8_t           xmit_timer; // xmit timeout timer
    uint8_t           recv_timer; // recv timeout timer
    uint8_t           looks_like_noise; // data range looks like noise
    IRReceiveCallback on_receive;
    uint16_t          len;      // Size of buff used
    uint16_t          tx_index; // 0 < tx_index < len
    uint16_t          next_interval;
} IR_STRUCT;

/* The work area for IR_CTRL is defined in ir_ctrl.c */
extern
volatile IR_STRUCT IrCtrl;

#ifdef __cplusplus
extern "C" {
#endif

/* Prototypes */
extern void IR_initialize (IRReceiveCallback on_receive);
extern int  IR_xmit (void);
extern void IR_clear (void);
extern uint16_t IR_get ();
extern void IR_put (uint16_t);
extern uint16_t IR_packedlength (void);
extern uint16_t IR_rawlength (void);
extern uint8_t IR_looks_like_noise (void);
extern void IR_timer (void);
extern void IR_loop (void);
extern void IR_state (uint8_t);
extern void IR_dump (void);

#ifdef __cplusplus
}
#endif

#endif // __IRCTRL_H__
