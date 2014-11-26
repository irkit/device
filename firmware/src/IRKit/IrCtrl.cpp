/*----------------------------------------------------------------------------/
/  IR_CTRL - IR remote control module                         (C)ChaN, 2008
/-----------------------------------------------------------------------------/
/ The IR_CTRL is a generic Transmisson/Reception control module for IR remote
/ control systems. This is a free software and is opened for education,
/ research and development under license policy of following trems.
/
/  Copyright (C) 2008, ChaN, all right reserved.
/  Copyright (C) 2013-, Masakazu Ohtsuka (mash), all right reserved.
/
/ * The IR_CTRL module is a free software and there is no warranty.
/ * You can use, modify and/or redistribute it for personal, non-profit or
/   commercial use without restriction under your responsibility.
/ * Redistributions of source code must retain the above copyright notice.
/
/-----------------------------------------------------------------------------/
/ Aug 30,'08 R0.01  First release.
/ May 16,'13 R0.    mash heavily modified everything
/----------------------------------------------------------------------------*/

#include <Arduino.h>
#include "IrCtrl.h"
#include "pgmStrToRAM.h"
#include "timer.h"
#include "IrPacker.h"
#include "env.h"
#include "const.h"
#include "log.h"

// avr/sfr_defs.h
#define _BV(bit) (1 << (bit))

/*----------------------------------------------------------------------------/
/ How this works (RX)
/-----------------------------------------------------------------------------/
/ When a change of the logic level occurs on Input Capture pin (ICP3)
/ and this edge conforms to Input Capture Edge Select (TCCR3B[ICES3]),
/ the 16bit value of the counter (TCNT3) is written to Input Capture Register (ICR3).
/ If Input Capture interrupt is enabled (ICIE3=1), ISR_CAPTURE() runs.
/ In ISR_CAPTURE(), we save interval of ICR3 since last edge
/ and toggle ICES3 to capture next edge
/ until the trailer (idle time without any edges) is detected.
/ see p119 of datasheet
/-----------------------------------------------------------------------------/
/ How this works (TX)
/-----------------------------------------------------------------------------/
/ We use Timer/Counter1's Fast PWM mode to generate IR carrier frequency(38kHz)
/ on Output Compare pin (OC1B).
/ 38kHz is determined by dividing our clock(16MHz) by 8 to make 2MHz, and counting 52 cycles.
/ Output Compare Register (OCR1A) is set to 52.
/ Provide an array of "Number of 2MHz cycles" in IrCtrl.buff .
/ Each entry of the array is set into Timer/Counter3's Output Compare Register (OCR3A),
/ which triggers an interrupt and run ISR_COMPARE() when compare matches.
/ In ISR_COMPARE(), we toggle ON/OFF of IR transmission (write 0/1 into TCCR1A[COM1B1])
/ and continue til the end of IrCtrl.buff .
/----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------/
/ Platform dependent definitions
/----------------------------------------------------------------------------*/
// Define interrupt service functions

// Rx: Timer input capture ISR
#define ISR_CAPTURE()   ISR(TIMER3_CAPT_vect)

// Timer compare match ISR
#define ISR_COMPARE()   ISR(TIMER3_COMPA_vect)

// --------------- RECV ---------------

// Initialize Timer3 for transmission/reception timing: Free running, clk/8
// TCCR3A : Timer/Counter3 Control Register A
//   0b00000000 = Normal port operation (don't output OC3[ABC] to any I/O pin)
// TCCR3B : Timer/Counter3 Control Register B
//   0bxxxxx001 = clk/   1
//   0bxxxxx010 = clk/   8 * 16MHz / 8 = 2MHz
//   0bxxxxx011 = clk/  64
//   0bxxxxx100 = clk/ 256
//   0bxxxxx110 = clk/1024
//   0b1xxxxxxx = Input Capture Noise Canceler: Active
//                Setting this bit (to one) activates the Input Capture Noise Canceler.
//                When the noise canceler is activated, the input from the Input Capture pin (ICP1)
//                is filtered. The filter function requires four successive equal valued samples of
//                the ICP1 pin for changing its output. The Input Capture is therefore delayed by
//                four Oscillator cycles when the noise canceler is enabled.
//   0bx0xxxxxx = Input Capture Edge Select
//                Toggle to detect rising/falling edge of ICP1 pin.
#define IR_INIT_TIMER() \
  TCCR3A = 0b00000000;  \
  TCCR3B = 0b10000010

// Check which edge generated the capture interrupt
// TCCR3B : Timer/Counter3 Control Register B
// ICES3 : Input Capture Edge Select
//   0 : falling (negative) edge is used as trigger
//   1 : rising (positive)  edge will trigger capture
// TIFR3 : Timer/Counter3 Interrupt Flag Register
// ICF3 : Timer/Counter3, Input Capture Flag
//   ICF3 can be cleared by writing a logic one to its bit location
// TOV3 : Timer/Counter3, Overflow Flag
// TIMSK3 : Timer/Counter3 Interrupt Mask Register
// ICIE3 : Timer/Counter3, Input Capture Interrupt Enable
//   When this bit is written to one,
//   and the I-flag in the Status Register is set (interrupts globally enabled),
//   the Timer/Counter3 Input Capture interrupt is enabled.
//   The corresponding Interrupt Vector (see “Interrupts” on page 57) is executed when the ICF3 Flag,
//   located in TIFR3, is set.
#define IR_CAPTURED_RISING() TCCR3B &   _BV(ICES3)
#define IR_CAPTURE_RISE()    TCCR3B |=  _BV(ICES3)
#define IR_CAPTURE_FALL()    TCCR3B &= ~_BV(ICES3)
#define IR_CAPTURE_ENABLE()  TIFR3  =   _BV(ICF3); TIMSK3 = _BV(ICIE3)
#define IR_CAPTURE_DISABLE() TIMSK3 &= ~_BV(ICIE3)   // Tx && Rx: Disable captureing interrupt

// ICR3 : Input Capture Register
#define IR_CAPTURE_REG()     ICR3

// --------------- XMIT ---------------

// Initialize Timer1 for IR subcarrier: Fast PWM, clk/8
// Countup from BOTTOM to TOP(=OCR1A), At BOTTOM set 1, At OCR1B set 0.
// "In fast PWM mode the counter is incremented until the counter value matches either one of the fixed values 0x00FF, 0x01FF, or 0x03FF (WGMn3:0 = 5, 6, or 7), the value in ICRn (WGMn3:0 = 14), or the value in OCRnA (WGMn3:0 = 15)." - we want the last behavior
// TCCR1A : Timer/Counter Control Register A
// TCCR1B : Timer/Counter Control Register B
//   0b010 : clk(T2S) / 8
// WGM10, WGM11, WGM12, WGM13 all 1 :
//   Timer/Counter Mode of Operation : Fast PWM - p148
//   TOP                             : OCRA
//   Update of OCRnX at              : TOP
//   TOV Flag Set on                 : TOP
// OCR1B : Timer/Counter1 Output Compare Register B
//   16 / 52 (OCR1A:38kHz) = 31% (PWM Duty)
#define IR_INIT_XMIT() \
  OCR1B  = 16; \
  TCCR1A = _BV(WGM11)|_BV(WGM10); \
  TCCR1B = _BV(WGM13)|_BV(WGM12)|0b010

// [atmega32u4 16MHz]
//   16MHz/8  = 2.00MHz
//   OCR1A=53 : 2.00MHz/(53+1) =  37.03KHz
//   OCR1A=52 : 2.00MHz/(52+1) =  37.73KHz * 38K
//   OCR1A=51 : 2.00MHz/(51+1) =  38.46KHz
//   OCR1A=50 : 2.00MHz/(50+1) =  39.21KHz
//   OCR1A=49 : 2.00MHz/(49+1) =  40.00KHz * 40K
#define IR_TX_38K()     OCR1A = 52; TCNT1 = 0   // Tx: Set IR burst frequency to 38kHz
#define IR_TX_40K()     OCR1A = 49; TCNT1 = 0   // Tx: Set IR burst frequency to 40kHz
#define IR_TX_ON()      TCCR1A |=  _BV(COM1B1)  // Tx: Start IR burst
#define IR_TX_OFF()     TCCR1A &= ~_BV(COM1B1)  // Tx: Stop IR burst
#define IR_TX_IS_ON()   TCCR1A &   _BV(COM1B1)  // Tx: Check if IR is being transmitted or not

// Enable compare interrupt n count after now
// OCR3A : Output Compare Register 3A
// TCNT3 : Timer/Counter3 direct access, both for read and for write operations,
//         to the Timer/Counter unit 16-bit counter.
// TIFR3 : Timer/Counter3 Interrupt Flag Register
// OCF3A : Timer/Counter3, Output Compare A Match Flag
// TIMSK3 : Timer/Counter3 Interrupt Mask Register
// OCIE3A : Timer/Counter3, Output Compare A Match Interrupt Enable
#define IR_COMPARE_ENABLE(n) \
  OCR3A   = TCNT3 + (n); \
  TIFR3   = _BV(OCF3A);  \
  TIMSK3 |= _BV(OCIE3A)

#define IR_COMPARE_DISABLE() TIMSK3 &= ~_BV(OCIE3A)  // Disable compare interrupt
#define IR_COMPARE_NEXT(n)   OCR3A += (n)            // Tx: Increase compare register by n count

// Counter clock rate and register width
// [Arduino 16MHz]
//   16MHz/8 => 1000000000ns/(16000000/8) = 500ns
#define T_CLK           500                    // Timer tick period [ns]
#define _timer_reg_t    uint16_t               // Integer type of timer register
/*---------------------------------------------------------------------------*/

// 65535 x 500[ns/tick] = 32_767_500[ns] = 32.8[ms]
// is too short,
// we're gonna wait for 4cycles of silence
// 4cycles = 131[ms] > 110[ms] (NEC protocol frame length)
#define T_TRAIL       65535
#define T_TRAIL_COUNT 4

// if interval between Rising and Falling edges are smaller than this time interval,
// this should be noise
// decided by experience
#define MAX_NOISE_INTERVAL 100

// my air conditioner takes 270ms, 2sec should be enough
#define RECV_TIMEOUT           2
#define XMIT_TIMEOUT           2

// down -1- up -2- down -3- up -4- down -5- up -6- down -7- up
#define VALID_IR_LEN_MIN   7

/// IR packed data storage size.
/// Known most longest IR data uses 363bytes (after compressed using IrPacker).
#define IR_BUFF_SIZE       SHARED_BUFFER_SIZE

// Working area for IR communication

volatile IR_STRUCT IrCtrl;
volatile struct irpacker_t packer_state;

// don't Serial.print inside ISR
static void IR_put_ (uint16_t data)
{
    if (irpacker_safelength(&packer_state) >= IR_BUFF_SIZE) {
        return;
    }
    if ((data != 0) && (data < MAX_NOISE_INTERVAL)) {
        IrCtrl.looks_like_noise = 1;
    }
    irpacker_pack(&packer_state, data);
    IrCtrl.len ++;
}

// IR receiving interrupt on either edge of input
ISR_CAPTURE()
{
    static _timer_reg_t last_interrupt;

    _timer_reg_t counter = IR_CAPTURE_REG();

    if ((IrCtrl.state == IR_RECVED) || (IrCtrl.state == IR_RECVED_IDLE)) {
        IR_state( IR_IDLE );
    }
    if ((IrCtrl.state != IR_IDLE) && (IrCtrl.state != IR_RECVING)) {
        return;
    }
    if (irpacker_safelength(&packer_state) >= IR_BUFF_SIZE) {
        // receive buffer overflow
        // data in buffer might be valid (later data might just be "repeat" data)
        // so wait for recv timeout and successfully transit to RECVED state
        return;
    }

    if (IR_CAPTURED_RISING()) {

        // Rising edge: on stop of burst

        if (IrCtrl.state == IR_IDLE) {
            // can't happen
            return;
        }

        _timer_reg_t low_width = counter - last_interrupt;
        last_interrupt         = counter;

        IrCtrl.trailer_count = T_TRAIL_COUNT;

        IR_CAPTURE_FALL();
        IR_COMPARE_ENABLE(T_TRAIL); // Enable trailer timer

        IR_put_(low_width); // use cycles after enabling timer
        return;
    }

    // Falling edge: on start of burst

    _timer_reg_t high_width = counter - last_interrupt;
    last_interrupt          = counter;

    if (IrCtrl.state == IR_IDLE) {
        IR_state( IR_RECVING );
    }
    else { // is IR_RECVING
        uint8_t trailer;
        for (trailer=T_TRAIL_COUNT; trailer>IrCtrl.trailer_count; trailer--) {
            IR_put_(65535);
            IR_put_(0);
            if (irpacker_safelength(&packer_state) >= IR_BUFF_SIZE) {
                return;
            }
        }
        IR_put_(high_width);
    }

    IR_CAPTURE_RISE();
    IR_COMPARE_DISABLE(); // Disable trailer timer
}

// Transmission timing and Trailer detection
ISR_COMPARE()
{
    if (IrCtrl.state == IR_XMITTING) {
        if (IrCtrl.tx_index >= IrCtrl.len) {
            // tx successfully finished
            IR_TX_OFF();
            IR_state( IR_IDLE );
            return;
        }
        uint16_t interval = IrCtrl.next_interval;
        if (IR_TX_IS_ON()) {
            // toggle
            IR_TX_OFF();
        }
        else {
            if ( IrCtrl.next_interval != 0 ) {
                // toggle
                IR_TX_ON();
            }
            else {
                // 65535,0,XXXX sequence means:
                // continue TX_OFF for 65535 + XXXX interval and turn TX_ON
                interval = IR_get();
                IrCtrl.tx_index ++;
            }
        }

        IR_COMPARE_NEXT( interval );

        // run heavy packer.unpack after setting timer
        IrCtrl.next_interval = IR_get();
        IrCtrl.tx_index ++;
        return;
    }
    else if (IrCtrl.state == IR_RECVING) {
        IrCtrl.trailer_count --;
        if (IrCtrl.trailer_count == 0) {
            // Trailer detected
            IR_state( IR_RECVED );
        }
        else {
            // wait for next compare interrupt
        }
        return;
    }

    IR_state( IR_IDLE );
}

int IR_xmit ()
{
    if (IrCtrl.len == 0) {
        IRLOG_PRINTLN("!E26");
        IR_state( IR_IDLE );
        return 0;
    }
#ifndef FACTORY_CHECKER
    // factory checker xmits after receiving
    if ( IrCtrl.state != IR_WRITING ) {
        return 0; // Abort when collision detected
    }
#endif

    irpacker_packend( &packer_state );

    IR_state( IR_XMITTING );
    if (IrCtrl.freq == 40) {
        IR_TX_40K();
    }
    else {
        IR_TX_38K();
    }
    IR_TX_ON();

    irpacker_unpack_start( &packer_state );

    IR_COMPARE_ENABLE( IR_get() );
    IrCtrl.tx_index ++;

    // unpacking takes time, so we want to run unpack while timer is running
    IrCtrl.next_interval = IR_get();

    return 1;
}

// _clear clears data, but _reset just resets it's state
void IR_clear (void)
{
    IrCtrl.len              = 0;
    IrCtrl.tx_index         = 0;
    IrCtrl.freq             = IR_DEFAULT_CARRIER; // reset to 38kHz every time
    IrCtrl.looks_like_noise = 0;
    memset( (void*)sharedbuffer, 0, sizeof(uint8_t) * IR_BUFF_SIZE );
    irpacker_clear( &packer_state );
}

void IR_reset (void)
{
    irpacker_reset( &packer_state );
}

uint16_t IR_get ()
{
    return irpacker_unpack( &packer_state );
}

// don't Serial.print inside ISR
void IR_put (uint16_t data)
{
    if ( IrCtrl.state != IR_WRITING ) {
        // caller should change state to IR_WRITING before calling
        return;
    }
    IR_put_(data);
}

uint16_t IR_packedlength (void)
{
    return irpacker_length( &packer_state );
}

uint16_t IR_rawlength (void)
{
    return IrCtrl.len;
}

uint8_t IR_looks_like_noise (void)
{
    return IrCtrl.looks_like_noise;
}

void IR_timer (void)
{
    if (IrCtrl.state == IR_RECVING) {
        TIMER_TICK( IrCtrl.recv_timer );

        if ( TIMER_FIRED( IrCtrl.recv_timer ) ) {
            TIMER_STOP( IrCtrl.recv_timer );
            IR_state( IR_RECVED );
        }
    }

    if (IrCtrl.state == IR_XMITTING) {
        TIMER_TICK( IrCtrl.xmit_timer );

        if ( TIMER_FIRED( IrCtrl.xmit_timer ) ) {
            TIMER_STOP( IrCtrl.xmit_timer );
            IR_state( IR_IDLE );
        }
    }
}

void IR_loop ()
{
    if (IrCtrl.state == IR_RECVED) {
        IrCtrl.on_receive();
#ifndef FACTORY_CHECKER
        // factory checker xmits after receive
        IR_state( IR_RECVED_IDLE );
#endif
    }
}

void IR_state (uint8_t nextState)
{
    switch (nextState) {
    case IR_IDLE:
        IR_TX_OFF();
        IR_COMPARE_DISABLE();

        // 1st interrupt when receiving ir must be falling edge
        IR_CAPTURE_FALL();
        IR_CAPTURE_ENABLE();

        IR_clear();
        break;
    case IR_RECVING:
        IR_clear();
        TIMER_START( IrCtrl.recv_timer, RECV_TIMEOUT );
        break;
    case IR_RECVED:
        TIMER_STOP( IrCtrl.recv_timer );

        irpacker_packend( &packer_state );

        // disable til IRKit.cpp reports IR data to server
        IR_CAPTURE_DISABLE();
        IR_COMPARE_DISABLE();

        if (IrCtrl.len < VALID_IR_LEN_MIN) {
            // received, but probably noise
            IR_state( IR_IDLE );
            return;
        }
        break;
    case IR_RECVED_IDLE:
        // holds received IR data, and ready to receive next
        IR_CAPTURE_FALL();
        IR_CAPTURE_ENABLE();
        break;
    case IR_READING:
        irpacker_unpack_start( &packer_state );
        IR_CAPTURE_DISABLE();
        IR_COMPARE_DISABLE();
        break;
    case IR_WRITING:
        IR_CAPTURE_DISABLE();
        IR_COMPARE_DISABLE();
        IR_clear();
        break;
    case IR_XMITTING:
        IR_CAPTURE_DISABLE();
        IR_COMPARE_DISABLE();
        IrCtrl.tx_index = 0;
        TIMER_START( IrCtrl.xmit_timer, XMIT_TIMEOUT );
        break;
    case IR_DISABLED:
        IR_CAPTURE_DISABLE();
        IR_COMPARE_DISABLE();
        break;
    }
    IrCtrl.state = nextState;
}

void IR_initialize (IRReceiveCallback _on_receive)
{
    IR_INIT_TIMER();
    IR_INIT_XMIT();

    IrCtrl.on_receive  = _on_receive;

    IR_state( IR_DISABLED );

    irpacker_init( &packer_state, (volatile uint8_t*)sharedbuffer );
}

void IR_dump (void)
{
    // IRLOG_PRINT(P("IR.s:")); IRLOG_PRINTLN(IrCtrl.state);
    // IRLOG_PRINT(P(".l:"));   IRLOG_PRINTLN(IrCtrl.len,HEX);
    // IRLOG_PRINT(P(".t:"));   IRLOG_PRINTLN(IrCtrl.trailer_count,HEX);
    // IRLOG_PRINT(P(".x:"));   IRLOG_PRINTLN(IrCtrl.tx_index,HEX);
    // IRLOG_PRINT(P(".r:"));   IRLOG_PRINTLN(IrCtrl.recv_timer);
    // IRLOG_PRINT(P(".x:"));   IRLOG_PRINTLN(IrCtrl.xmit_timer);
    // IRLOG_PRINT(P("p.l:"));  IRLOG_PRINTLN(IR_packedlength(),HEX);
    // for (uint16_t i=0; i<IR_packedlength(); i++) {
    //     IRLOG_PRINT((uint8_t)sharedbuffer[i], HEX);
    //     IRLOG_PRINT(" ");
    // }
    // IRLOG_PRINTLN();
    // IRLOG_PRINT("tree:");
    // IRLOG_PRINTLN(tree[ 0 ]);
    // IRLOG_PRINTLN(tree[ 1 ]);
}
