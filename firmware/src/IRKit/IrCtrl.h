#ifndef __IRCTRL_H__
#define __IRCTRL_H__

/** @file IrCtrl.h */
/*----------------------------------------------------------------------------/
/  IR_CTRL - IR remote control module                         (C)ChaN, 2008
/-----------------------------------------------------------------------------/
/  Common include file for IR_CTRL module and application
/----------------------------------------------------------------------------*/

#include <stdint.h>

/* Put hardware dependent include files here */
#include <avr/io.h>
#include <avr/interrupt.h>
#include "IrPacker.h"
#include "const.h"

/// [kHz]
#define IR_DEFAULT_CARRIER 38

/* IR control state */

/// Idle state. We're ready to receive/transmit.
#define IR_IDLE        0

/// IR data is being received
#define IR_RECVING     1

/// IR data has been received
#define IR_RECVED      2

/// IR data is POSTed to server. We still preserve IR data, but are ready to receive next
#define IR_RECVED_IDLE 3

/// Somebody is wrinting IR data
#define IR_WRITING     10

/// IR transmission is in progress
#define IR_XMITTING    11

/// Somebody is reading IR data
#define IR_READING     20

/// IR receive/xmit is disabled. This is our initial state
#define IR_DISABLED    0xFF

/// IRReceiveCallback is called inside IR_loop() in IR_RECVED state.
typedef void (*IRReceiveCallback)();

/// IR internal state
typedef struct _irstruct {
    /// IR_XXXX state
    uint8_t           state;

    /// Number of T_TRAIL time to wait to determine signal ended
    uint8_t           trailer_count;

    /// IR carrier wave freq in kHz
    uint8_t           freq;

    /// Xmit timeout timer
    uint8_t           xmit_timer;

    /// Receive timeout timer
    uint8_t           recv_timer;

    /// True if signal looks like noise
    uint8_t           looks_like_noise;

    /// IR receive callback
    IRReceiveCallback on_receive;

    /// Size of buff used
    uint16_t          len;

    /// 0 < tx_index < len
    uint16_t          tx_index;

    /// Number of 2MHz ticks till next rising/falling edge while xmitting
    uint16_t          next_interval;
} IR_STRUCT;

/// IR internal state
extern volatile IR_STRUCT IrCtrl;

/// IR packed data storage, used as Wi-Fi SSID/password temporary buffer on boot (when IR_DISABLED).
extern volatile char sharedbuffer[SHARED_BUFFER_SIZE];

#ifdef __cplusplus
extern "C" {
#endif

/// Initializer.
/// Call on boot.
/// Provide callback to be called after receiving IR signal.
extern void IR_initialize (IRReceiveCallback);

/// IR receiving interrupt handler.
/// Call on all rising/falling edges of IR receiver input.
// ISR_CAPTURE()
extern void IR_ISR_Capture (void);

/// IR transmission timing and trailer detection handler.
/// Call on timer compare match.
// ISR_COMPARE()
extern void IR_ISR_Compare (void);

/// Transmit IR data from IR LEDs which is stored in buffer.
/// Call IR_put beforehand to set IR data in buffer.
extern int  IR_xmit (void);

/// Reset IR packer state but preserve IR data.
/// Only use in factory checker.
extern void IR_reset (void);

/// Get and unpack 1 data from buffer.
/// Set state to IR_READING beforehand.
/// Call number of IR_rawlength() times to respond to HTTP IR fetch request(Get /messages) with IR data.
extern uint16_t IR_get (void);

/// Store and pack 1 data into buffer.
/// Set state to IR_WRITING beforehand.
/// Call multiple times while receiving HTTP IR xmit request(POST /messages).
/// Silently ignores input if IR buffer is full.
/// Call IR_xmit after you've put all IR data.
extern void IR_put (uint16_t);

/// Number of bytes in buffer used after packed.
extern uint16_t IR_packedlength (void);

/// Number of unpacked 2MHz rise/fall intervals stored in buffer
extern uint16_t IR_rawlength (void);

/// Does the IR signal look like noise?
extern uint8_t IR_looks_like_noise (void);

/// Call every 200ms, to check if receive timeout or xmit timeout occured
extern void IR_timer (void);

/// Periodically call to check if we've received an IR signal.
/// Calls IRReceiveCallback if we're in IR_RECVED state.
extern void IR_loop (void);

/// Set IR state.
extern void IR_state (uint8_t);

extern void IR_dump (void);

#ifdef __cplusplus
}
#endif

#endif // __IRCTRL_H__
