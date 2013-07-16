/*----------------------------------------------------------------------------/
/  IR_CTRL - IR remote control module                         (C)ChaN, 2008
/-----------------------------------------------------------------------------/
/ The IR_CTRL is a generic Transmisson/Reception control module for IR remote
/ control systems. This is a free software and is opened for education,
/ research and development under license policy of following trems.
/
/  Copyright (C) 2008, ChaN, all right reserved.
/  Copyright (C) 2013-, mash, all right reserved.
/
/ * The IR_CTRL module is a free software and there is no warranty.
/ * You can use, modify and/or redistribute it for personal, non-profit or
/   commercial use without restriction under your responsibility.
/ * Redistributions of source code must retain the above copyright notice.
/
/-----------------------------------------------------------------------------/
/ Aug 30,'08 R0.01  First release.
/ May 16,'13 R0.    mash added comments, readable macro names
/----------------------------------------------------------------------------*/

#include "IrCtrl.h"

/*----------------------------------------------------------------------------/
/ How this works (RX)
/-----------------------------------------------------------------------------/
/ When a change of the logic level occurs on Input Capture pin (ICP1)
/ and this edge conforms to Input Capture Edge Select (TCCR1B[ICES1]),
/ the 16bit value of the counter (TCNT1) is written to Input Capture Register (ICR1).
/ If Input Capture interrupt is enabled (ICIE1=1), ISR_CAPTURE() runs.
/ In ISR_CAPTURE(), we save interval of ICR1 since last edge
/ and toggle ICES1 to capture next edge
/ until the trailer (idle time without any edges) is detected.
/ see p119 of datasheet
/-----------------------------------------------------------------------------/
/ How this works (TX)
/-----------------------------------------------------------------------------/
/ We use Timer/Counter2's Fast PWM mode to generate IR carrier frequency(38kHz)
/ on Output Compare pin (OC2B).
/ 38kHz is determined by dividing our clock(16MHz) by 8 to make 2MHz, and counting 52 cycles.
/ Output Compare Register (OCR2A) is set to 52.
/ Provide an array of "Number of 2MHz cycles" in IrCtrl.buff .
/ Each entry of the array is set into Timer/Counter1's Output Compare Register (OCR1A),
/ which triggers an interrupt and run ISR_COMPARE() when compare matches.
/ In ISR_COMPARE(), we toggle ON/OFF of IR transmission (write 0/1 into TCCR2A[COM2B1])
/ and continue til the end of IrCtrl.buff .
/----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------/
/ Platform dependent definitions
/----------------------------------------------------------------------------*/
/* Define interrupt service functions */
#define ISR_COMPARE()   ISR(TIMER1_COMPA_vect)  /* Timer compare match ISR */
#define ISR_CAPTURE()   ISR(TIMER1_CAPT_vect)   /* Rx: Timer input capture ISR */

/* Define hardware control macros */
#define IR_INIT_TIMER()                         /* Initialize Timer (Timer1 for transmission/reception timing: Free running, clk/8) */ \
  TCCR1B = 0b10000010; \
  TCCR1A = 0b00000000
/*
  TCCR1B : Timer/Counter1 Control Register B
    0bxxxxx001 = clk/   1
    0bxxxxx010 = clk/   8 *
    0bxxxxx011 = clk/  64
    0bxxxxx100 = clk/ 256
    0bxxxxx110 = clk/1024

    0bxxxxx010 = clk/   8 : 16MHz / 8 = 2MHz
    0b1xxxxxxx = Input Capture Noise Canceler: Active
                 Setting this bit (to one) activates the Input Capture Noise Canceler.
                 When the noise canceler is activated, the input from the Input Capture pin (ICP1)
                 is filtered. The filter function requires four successive equal valued samples of
                 the ICP1 pin for changing its output. The Input Capture is therefore delayed by
                 four Oscillator cycles when the noise canceler is enabled.
    0bx0xxxxxx = Input Capture Edge Select
                 Toggle to detect rising/falling edge of ICP1 pin.

  TCCR1A : Timer/Counter1 Control Register B
    0b00000000 = Normal port operation

*/

#define IR_INIT_XMIT()                          /* Tx: Initialize Transmitter (Timer2 for IR subcarrier: Fast PWM, clk/8) */ \
    OCR2B = 16; \
    TCCR2A = _BV(WGM21)|_BV(WGM20); \
    TCCR2B = _BV(WGM22)|0b010
/*

 Counting from BOTTOM to TOP(=OCR2A), At BOTTOM set 1, At OCR2B set 0.

[mega328 16MHz]
  16MHz/8  = 2.00MHz
  OCR2A=53 : 2.00MHz/(53+1) =  37.03KHz
  OCR2A=52 : 2.00MHz/(52+1) =  37.73KHz * 38K
  OCR2A=51 : 2.00MHz/(51+1) =  38.46KHz
  OCR2A=50 : 2.00MHz/(50+1) =  39.21KHz
  OCR2A=49 : 2.00MHz/(49+1) =  40.00KHz * 40K
  OCR2B = 16
          The Output Compare Register B contains an 8-bit value that is continuously compared with
          the counter value (TCNT2). A match can be used to generate an Output Compare interrupt,
          or to generate a waveform output on the OC2B pin.

we use ATmega328P-AU
// avr/iom328p.h
#define WGM20 0
#define WGM21 1
#define WGM22 3

// avr/sfr_defs.h
#define _BV(bit) (1 << (bit))

TCCR2A : Timer/Counter Control Register A
WGM20, WGM21, WGM22 all 1 :
  Timer/Counter Mode of Operation : Fast PWM - p148
  TOP                             : OCRA
  Update of OCRx at               : BOTTOM
  TOV Flag Set on                 : TOP

TCCR2B : Timer/Counter Control Register B

CS21:0 : Clock Select
  0b010 : clk(T2S) / 8

*/

#define IR_TX_38K()     OCR2A = 52; TCNT2 = 0   /* Tx: Set IR burst frequency to 38kHz */
#define IR_TX_40K()     OCR2A = 49; TCNT2 = 0   /* Tx: Set IR burst frequency to 40kHz */
#define IR_TX_ON()      TCCR2A |=  _BV(COM2B1)  /* Tx: Start IR burst */
#define IR_TX_OFF()     TCCR2A &= ~_BV(COM2B1)  /* Tx: Stop IR burst */
#define IR_TX_IS_ON()   TCCR2A &   _BV(COM2B1)  /* Tx: Check if IR is being transmitted or not */

#define IR_CAPTURED_RISING() TCCR1B & _BV(ICES1) /* Rx: Check which edge generated the capture interrupt */
/*
TCCR1B : Timer/Counter1 Control Register B
ICES1 : Input Capture Edge Select - This bit selects which edge on the Input Capture pin (ICP1) that is used to trigger a capture event. When the ICES1 bit is written to zero, a falling (negative) edg

*/
#define IR_CAPTURE_RISE()  TCCR1B |=  _BV(ICES1)   /* Rx: Set captureing is triggered on rising edge */
#define IR_CAPTURE_FALL()  TCCR1B &= ~_BV(ICES1)   /* Rx: Set captureing is triggered on falling edge */
/*
TCCR1B : Timer/Conter1 Control Register B
ICES1 : Input Capture Edge Select
  0 : falling (negative) edge is used as trigger
  1 : rising (positive)  edge will trigger capture

 */


#define IR_CAPTURE_ENABLE()   TIFR1 = _BV(ICF1); TIMSK1 = _BV(ICIE1)  /* Rx: Enable captureing interrupt */

/*
TIFR1 : Timer/Counter1 Interrupt Flag Register
ICF1 : Timer/Counter1, Input Capture Flag
  ICF1 can be cleared by writing a logic one to its bit location
TOV1 : Timer/Counter1, Overflow Flag # TODO

TIMSK1 : Timer/Counter1 Interrupt Mask Register
ICIE1 : Timer/Counter1, Input Capture Interrupt Enable
  When this bit is written to one,
  and the I-flag in the Status Register is set (interrupts globally enabled),
  the Timer/Counter1 Input Capture interrupt is enabled.
  The corresponding Interrupt Vector (see “Interrupts” on page 57) is executed when the ICF1 Flag,
  located in TIFR1, is set.
 */

#define IR_CAPTURE_REG()   ICR1                    /* Rx: Returns the value in capture register */
/*
ICR1 : Input Capture Register
 */
#define IR_CAPTURE_DISABLE()   TIMSK1 &= ~_BV(ICIE1)   /* Tx && Rx: Disable captureing interrupt */

// Enable compare interrupt n count after now
#define IR_COMPARE_ENABLE(n)  OCR1A = TCNT1 + (n); TIFR1 = _BV(OCF1A); TIMSK1 |= _BV(OCIE1A)
/*
OCR1A : Output Compare Register 1A
TCNT1 : Timer/Counter1 direct access, both for read and for write operations,
        to the Timer/Counter unit 16-bit counter.
TIFR1 : Timer/Counter1 Interrupt Flag Register
OCF1A : Timer/Counter1, Output Compare A Match Flag
TIMSK1 : Timer/Counter1 Interrupt Mask Register
OCIE1A : Timer/Counter1, Output Compare A Match Interrupt Enable
 */
#define IR_COMPARE_DISABLE()   TIMSK1 &= ~_BV(OCIE1A)  /* Disable compare interrupt */
#define IR_COMPARE_NEXT(n) OCR1A += (n)            /* Tx: Increase compare register by n count */

/* Counter clock rate and register width */
#define T_CLK           500                    /* Timer tick period [ns] */
/*
[Arduino 16MHz]
  16MHz/8 => 1000000000ns/(16000000/8) = 500ns
 */
#define _timer_reg_t          uint16_t                /* Integer type of timer register */
/*---------------------------------------------------------------------------*/

// 65535 x 500[ns/tick] = 32_767_500[ns] = 32.8[ms]
// is too short,
// we're gonna wait for 4cycles of silence
// 4cycles = 131[ms] > 110[ms] (NEC protocol frame length)
#define T_TRAIL       65535
#define T_TRAIL_COUNT 4


/* Working area for IR communication  */

volatile IR_STRUCT IrCtrl;

/* IR receiving interrupt on either edge of input */
ISR_CAPTURE()
{
    static _timer_reg_t last_interrupt;

    _timer_reg_t counter = IR_CAPTURE_REG();

    if (IrCtrl.state == IR_RECVED_IDLE) {
        IR_state( IR_IDLE );
    }
    if ((IrCtrl.state != IR_IDLE) && (IrCtrl.state != IR_RECVING)) {
        return;
    }
    if (IrCtrl.len >= IR_BUFF_SIZE) {
        return; // TODO clear state?
    }

    if (IR_CAPTURED_RISING()) {

        // Rising edge: on stop of burst

        _timer_reg_t low_width = counter - last_interrupt;
        last_interrupt         = counter;

        IrCtrl.buff[ IrCtrl.len ++ ] = low_width;

        IR_CAPTURE_FALL();
        IR_COMPARE_ENABLE(T_TRAIL); // Enable trailer timer
        IrCtrl.trailerCount = T_TRAIL_COUNT;

        return;
    }

    // Falling edge: on start of burst

    _timer_reg_t high_width = counter - last_interrupt;
    last_interrupt          = counter;

    if (IrCtrl.state == IR_IDLE) {
        IR_state( IR_RECVING );
    }
    else { // is IR_RECVING
        for (uint8_t trailer=T_TRAIL_COUNT; trailer>IrCtrl.trailerCount; trailer--) {
            IrCtrl.buff[ IrCtrl.len ++ ] = 65535; // high
            IrCtrl.buff[ IrCtrl.len ++ ] = 0;     // low
        }
        IrCtrl.buff[ IrCtrl.len ++ ] = high_width;
    }

    IR_CAPTURE_RISE();
    IR_COMPARE_DISABLE(); // Disable trailer timer
}

/* Transmission timing and Trailer detection */
ISR_COMPARE()
{
    if (IrCtrl.state == IR_XMITTING) {
        if ((IrCtrl.txIndex >= IrCtrl.len) ||
            (IrCtrl.txIndex >= IR_BUFF_SIZE)) {
            // tx successfully finished
            IR_state( IR_IDLE );
            return;
        }
        uint16_t next = IrCtrl.buff[ IrCtrl.txIndex ++ ];
        if (IR_TX_IS_ON()) {
            // toggle
            IR_TX_OFF();
        }
        else {
            if ( next != 0 ) {
                // toggle
                IR_TX_ON();
            }
            else {
                // continue for another uin16_t loop
                next = IrCtrl.buff[ IrCtrl.txIndex ++ ];
                IR_TX_OFF();
            }
        }

        IR_COMPARE_NEXT( next );
        return;
    }
    else if (IrCtrl.state == IR_RECVING) {
        IrCtrl.trailerCount --;
        if (IrCtrl.trailerCount == 0) {
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
    // TODO errcode
    if ((IrCtrl.len == 0) ||
        (IrCtrl.len > IR_BUFF_SIZE)) {
        return 0;
    }
    if ( (IrCtrl.state != IR_IDLE) && (IrCtrl.state != IR_RECVED_IDLE) ) {
        return 0; // Abort when collision detected
    }

    IR_state( IR_XMITTING );
    if (IrCtrl.freq == 40) {
        IR_TX_40K();
    }
    else {
        IR_TX_38K();
    }
    IR_TX_ON();
    IR_COMPARE_ENABLE( IrCtrl.buff[ IrCtrl.txIndex ++ ] );

    return 1;
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

        IrCtrl.len     = 0;
        IrCtrl.txIndex = 0;
        IrCtrl.freq    = IR_DEFAULT_CARRIER; // reset to 38kHz every time
        for (uint16_t i=0; i<IR_BUFF_SIZE; i++) {
            IrCtrl.buff[i] = 0;
        }
        break;
    case IR_RECVING:
        IrCtrl.len     = 0;
        IrCtrl.txIndex = 0;
        for (uint16_t i=0; i<IR_BUFF_SIZE; i++) {
            IrCtrl.buff[i] = 0;
        }
        break;
    case IR_RECVED:
        IR_COMPARE_DISABLE();
        IR_CAPTURE_DISABLE();
        break;
    case IR_RECVED_IDLE:
        IR_CAPTURE_FALL();
        IR_CAPTURE_ENABLE();
        break;
    case IR_XMITTING:
        IR_CAPTURE_DISABLE();
        IR_COMPARE_DISABLE();
        IrCtrl.txIndex = 0;
        break;
    }
    IrCtrl.state = nextState;
}

void IR_initialize (void)
{
    IR_INIT_TIMER();
    IR_INIT_XMIT();

    IR_state( IR_IDLE );
}
