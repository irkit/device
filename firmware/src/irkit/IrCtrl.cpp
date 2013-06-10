/*----------------------------------------------------------------------------/
/  IR_CTRL - IR remote control module  R0.01                  (C)ChaN, 2008
/-----------------------------------------------------------------------------/
/ The IR_CTRL is a generic Transmisson/Reception control module for IR remote
/ control systems. This is a free software and is opened for education,
/ research and development under license policy of following trems.
/
/  Copyright (C) 2008, ChaN, all right reserved.
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
    0b10000000 = Input Capture Noise Canceler: Active
                 Setting this bit (to one) activates the Input Capture Noise Canceler.
                 When the noise canceler is activated, the input from the Input Capture pin (ICP1)
                 is filtered. The filter function requires four successive equal valued samples of
                 the ICP1 pin for changing its output. The Input Capture is therefore delayed by
                 four Oscillator cycles when the noise canceler is enabled.

  TCCR1A : Timer/Counter1 Control Register B
    0b00000000 = Normal port operation

*/

#define IR_INIT_XMIT()                          /* Tx: Initialize Transmitter (Timer2 for IR subcarrier: Fast PWM, clk/8) */ \
    OCR2B = 16; \
    TCCR2A = _BV(WGM21)|_BV(WGM20); \
    TCCR2B = _BV(WGM22)|0b010
/*
[sample(mega48 10MHz)]
  OCR2B  = 10
  TCCR2A = _BV(WGM21)|_BV(WGM20)
  TCCR2B = _BV(WGM22)|0b010
  IR_TX_38K() OCR2A = 32
  IR_TX_40K() OCR2A = 30

  WGM2[111] = fast PWM
  0b010     = clk/8
  Counting BOTTOM to TOP(=OCR2A), At BOTTOM set 1, At OCR2B set 0.
  Width of 1 is OCR2B=10, fixed.
  Width of 0 is OCR2A-OCR2B. Freq is defined by OCR2A.

   10MHz/8 = 1.25MHz
  OCR2A=33 : 1.25MHz/(33+1) =  36.76KHz
  OCR2A=32 : 1.25MHz/(32+1) =  37.87KHz * 38K
  OCR2A=31 : 1.25MHz/(31+1) =  39.06KHz
  OCR2A=30 : 1.25MHz/(30+1) =  40.32KHz * 40K
  OCR2A=29 : 1.25MHz/(29+1) =  41.66KHz

[Arduino2009+mega328 16MHz]
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
WGM20, WGM21 both 1 :
  Timer/Counter Mode of Operation : Fast PWM - p148
  TOP                             : 0xFF
  Update of OCRx at               : BOTTOM
  TOV Flag Set on                 : MAX (0xFF)

TCCR2B : Timer/Counter Control Register B
WGM22 : Waveform Geenration Mode
  0 : Normal Port Operation, OC2A Disconnected
  1 : Toggle OC2A on Copmare Match

CS21:0 : Clock Select
  0b010 : clk(T2S) / 8

*/

#define IR_TX_38K()     OCR2A = 52; TCNT2 = 0   /* Tx: Set IR burst frequency to 38kHz */
#define IR_TX_40K()     OCR2A = 49; TCNT2 = 0   /* Tx: Set IR burst frequency to 40kHz */
#define IR_TX_ON()      TCCR2A |=  _BV(COM2B1)  /* Tx: Start IR burst */
#define IR_TX_OFF()     TCCR2A &= ~_BV(COM2B1)  /* Tx: Stop IR burst */
#define IR_TX_TEST()    TCCR2A &   _BV(COM2B1)  /* Tx: Check if IR is being transmitted or not */

#define IR_CAPTURE_TEST()  TCCR1B &   _BV(ICES1)   /* Rx: Check which edge generated the capture interrupt */
/*
TCCR1B : Timer/Counter1 Control Register B
ICES1 : Input Capture Edge Select
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
  ICF1 can be cleared by writing a loginc one to its bit location
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

/*
can we read TCNT1(16-bit register) at once?
see p.114
 */
#define IR_COMPARE_ENABLE(n)  OCR1A = TCNT1 + (n); TIFR1 = _BV(OCF1A); TIMSK1 |= _BV(OCIE1A) /* Enable compare interrupt n count after now */
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

/*
 * 16MHz -> 500
 *  8MHz -> 1000
 */
// 1000_000_000
// #define T_CLK           (1000000000 / (F_CPU / 8))

/* Counter clock rate and register width */
#define T_CLK           500                    /* Timer tick period [ns] */
/*
[sample 10MHz]
  T_CLK = 800
  1s = 1000ms = 1000000us = 1000000000ns
  1M = 1000K  = 1000000
  10MHz   => 1000000000ns/ 10000000    = 100ns
  10MHz/8 => 1000000000ns/(10000000/8) = 800ns
[Arduino 16MHz]
  16MHz/8 => 1000000000ns/(16000000/8) = 500ns
*/
#define _timer_reg_t          uint16_t                /* Integer type of timer register */
/*---------------------------------------------------------------------------*/

/* IR control timings */
#define T_NEC   (562000/T_CLK)        /* Base time for NEC format (T=562us) */
#define T_AEHA  (425000/T_CLK)        /* Base time for AEHA format (T=425us) */
#define T_SONY  (600000/T_CLK)        /* Base time for SONY format (T=600us) */
#define T_TRAIL (6000000/T_CLK)        /* Trailer detection time (6ms) */


/* Working area for IR communication  */

volatile IR_STRUCT IrCtrl;


/* IR receiving interrupt on either edge of input */
#if IR_USE_RCVR
ISR_CAPTURE()
{
    static _timer_reg_t last_interrupt, low_width;
    _timer_reg_t counter, high_width;
    static uint8_t bit_counter;        /* Bit counter */
    uint8_t received_bytes, format, data;

    counter = IR_CAPTURE_REG();

    if (IR_CAPTURE_TEST()) {
        /* On stop of burst (rising edge) */
        IR_CAPTURE_FALL();            /* Next is start of carrier (falling edge on input) */
        IR_COMPARE_ENABLE(T_TRAIL);   /* Enable trailer timer */

        // TODO does counter overflow?
        low_width      = counter - last_interrupt;
        last_interrupt = counter;

        if (IR_USE_SONY &&
            IrCtrl.format == SONY &&
            low_width >= (uint16_t)(T_SONY * 0.8) &&
            low_width <= (uint16_t)(T_SONY * 2.5))
        {
            received_bytes = IrCtrl.phase / 8;
            if (received_bytes >= sizeof(IrCtrl.buff)) return;

            // save each bit in buff[ received_bytes ] using bit operation
            data = IrCtrl.buff[ received_bytes ];
            IrCtrl.buff[ received_bytes ] = (low_width >= (uint16_t)(T_SONY * 1.5))
                ? data | bit_counter
                : data & ~bit_counter;
            if ((bit_counter <<= 1) == 0) bit_counter = 1;
            IrCtrl.phase++;
        }
        return;
    }

    /* On start of burst (falling edge) */
    IR_CAPTURE_RISE();                        /* Next is stop of carrier (rising edge on input) */
    IR_COMPARE_DISABLE();                     /* Disable trailer timer */

    high_width     = counter - last_interrupt;
    last_interrupt = counter;

    if (IrCtrl.state >= IR_RECVED) return;    /* Reject if not ready to receive */

    format = 0;

    // detect leader pattern (on the falling edge of the first customer/data code)

    if (IR_USE_NEC &&
        low_width >= T_NEC * 13 &&
        low_width <= T_NEC * 19)
    {
        /* Is NEC leader pattern? */
        if (high_width >= T_NEC * 6 &&
            high_width <= T_NEC * 10)
            format = NEC;

        if (high_width >= T_NEC * 3 &&
            high_width <= T_NEC * 5)
            format = NEC|REPT;
    }
    if (IR_USE_AEHA &&
        low_width >= T_AEHA * 5 &&
        low_width <= T_AEHA * 12)
    {
        /* Is AEHA leader pattern? */
        if (high_width >= (uint16_t)(T_AEHA * 2.5) &&
            high_width <= (uint16_t)(T_AEHA * 5.5))
            format = AEHA;

        if (high_width >= T_AEHA * 5 &&
            high_width <= T_AEHA * 11)
            format = AEHA|REPT;
    }
    if (IR_USE_SONY &&
        low_width >= T_SONY * 3 &&
        low_width <= T_SONY * 5) {
        /* Is SONY leader pattern? */
        if (high_width >= (uint16_t)(T_SONY * 0.75) &&
            high_width <= (uint16_t)(T_SONY * 1.25))
            format = SONY;
    }
    if (format) {    /* A leader pattern is detected */
        IrCtrl.format = format;
        IrCtrl.phase  = 0;
        bit_counter   = 1;
        IrCtrl.state  = IR_RECVING;
        return;
    }

    // detect custom, data pattern

    if (IrCtrl.state == IR_RECVING) {
        received_bytes = IrCtrl.phase / 8;
        if (received_bytes >= sizeof(IrCtrl.buff)) return;

        data   = IrCtrl.buff[ received_bytes ];
        format = IrCtrl.format;
        if (IR_USE_NEC &&
            format == NEC &&
            low_width  <= (uint16_t)(T_NEC * 1.5) &&
            high_width <= (uint16_t)(T_NEC * 3 * 1.5))
        {
            /* Is NEC data mark? */
            IrCtrl.buff[ received_bytes ] = (high_width >= T_NEC * 2)
                ? data | bit_counter
                : data & ~bit_counter;
            if ((bit_counter <<= 1) == 0) bit_counter = 1;
            IrCtrl.phase++;
            return;
        }
        if (IR_USE_AEHA &&
            format == AEHA &&
            low_width  <= (uint16_t)(T_AEHA * 1.5) &&
            high_width <= (uint16_t)(T_AEHA * 3 * 1.5))
        {
            /* Is AEHA data mark? */
            IrCtrl.buff[ received_bytes ] = (high_width >= T_AEHA * 2)
                ? data | bit_counter
                : data & ~bit_counter;
            if ((bit_counter <<= 1) == 0) bit_counter = 1;
            IrCtrl.phase++;
            return;
        }
        if (IR_USE_SONY &&
            format == SONY &&
            high_width <= (uint16_t)(T_SONY * 1.5))
        {
            /* Is SONY data mark? */
            return;        /* Nothing to do at start of carrier */
        }
    }

    // When an invalid mark width is detected, abort and return idle state
    // We come here on the very 1st falling edge of the leader code too
    IrCtrl.state = IR_IDLE;
}
#endif /* IR_USE_RCVR */



/* Transmission timing and Trailer detection */

ISR_COMPARE()
{
    uint8_t state = IrCtrl.state;

#if IR_USE_XMIT
    uint8_t i, data, format = IrCtrl.format;
    uint16_t width;

    if (state == IR_XMITING) {
        if (IR_TX_TEST()) {             /* End of mark? */
            IR_TX_OFF();                /* Stop burst */
            i = IrCtrl.phase;
            if (i < IrCtrl.len) {        /* Is there a bit to be sent? */
                if (IR_USE_SONY && (format & SONY)) {
                    width = T_SONY;
                } else {
                    i /= 8;
                    data = IrCtrl.buff[i];
                    if (IR_USE_AEHA && (format & AEHA))
                        width = (data & 1) ? T_AEHA * 3 : T_AEHA;
                    else
                        width = (data & 1) ? T_NEC * 3 : T_NEC;
                    IrCtrl.buff[i] = data >> 1;
                }
                IR_COMPARE_NEXT(width);
                return;
            }
        } else {
            IR_TX_ON();                    /* Start burst */
            i = ++IrCtrl.phase / 8;
            if (IR_USE_SONY && (format & SONY)) {
                data = IrCtrl.buff[i];
                width = (data & 1) ? T_SONY * 2 : T_SONY;
                IrCtrl.buff[i] = data >> 1;
            } else {
                width = (format & NEC) ? T_NEC : T_AEHA;
            }
            IR_COMPARE_NEXT(width);
            return;
        }
    }

    if (state == IR_XMIT) {
        IR_TX_OFF();                    /* Stop carrier */
        switch (format) {                    /* Set next transition time */
#if IR_USE_SONY
        case SONY:
            width = T_SONY;
            break;
#endif
#if IR_USE_AEHA
        case AEHA:
            width = IrCtrl.len ? T_AEHA * 4 : T_AEHA * 8;
            break;
#endif
        default:    /* NEC */
            width = IrCtrl.len ? T_NEC * 8 : T_NEC * 4;
            break;
        }
        IR_COMPARE_NEXT(width);
        IrCtrl.state = IR_XMITING;
        IrCtrl.phase = 0xFF;
        return;
    }
#endif /* IR_USE_XMIT */

    IR_COMPARE_DISABLE();                    /* Disable compare */

#if IR_USE_RCVR
#if IR_USE_XMIT
    IR_CAPTURE_ENABLE();                    /* Re-enable receiving */
#endif
    if (state == IR_RECVING) {            /* Trailer detected */
        IrCtrl.len   = IrCtrl.phase;
        IrCtrl.state = IR_RECVED;
        return;
    }
#endif

    IrCtrl.state = IR_IDLE;
}




/*---------------------------*/
/* Data Transmission Request */
/*---------------------------*/

#if IR_USE_XMIT
int IR_xmit (
    uint8_t        format,      /* Frame format: NEC, AEHA or SONY */
    const uint8_t* data,        /* Pointer to the data to be sent */
    uint8_t        len          /* Data length [bit]. 0 for a repeat frame */
)
{
    _timer_reg_t leader_width;
    uint8_t i;

    // TODO errcode
    if (len / 8 > sizeof(IrCtrl.buff)) return 0; /* Too long data */
    if (IrCtrl.state != IR_IDLE)       return 0; /* Abort when collision detected */

    switch (format) {
#if IR_USE_NEC
    case NEC:    /* NEC frame */
        if (len != 0 && len != 32) return 0;        /* Must be 32 bit data */
        leader_width = T_NEC * 16;    /* Leader burst time */
        IR_TX_38K();
        break;
#endif
#if IR_USE_AEHA
    case AEHA:    /* AEHA frame */
        if ((len > 0 && len < 48) || len % 8) return 0;    /* Must be 48 bit or longer data */
        leader_width = T_AEHA * 8;    /* Leader burst time */
        IR_TX_38K();
        break;
#endif
#if IR_USE_SONY
    case SONY:    /* SONY frame */
        if (len != 12 && len != 15 && len != 20) return 0;    /* Must be 12, 15 or 20 bit data */
        leader_width = T_SONY * 4;    /* Leader burst time */
        IR_TX_40K();
        break;
#endif
    default:
        return 0;
    }

#if IR_USE_RCVR
    IR_CAPTURE_DISABLE();
#endif
    IR_COMPARE_DISABLE();
    IrCtrl.format = format;
    IrCtrl.len = (IR_USE_SONY && (format == SONY)) ? len - 1 : len;
    len = (len + 7) / 8;
    for (i = 0; i < len; i++) IrCtrl.buff[i] = data[i];

    /* Start transmission sequense */
    IrCtrl.state = IR_XMIT;
    IR_TX_ON();
    IR_COMPARE_ENABLE(leader_width);

    return 1;
}
#endif /* IR_USE_XMIT */



/*---------------------------*/
/* Initialize IR functions   */
/*---------------------------*/

void IR_initialize (void)
{
    /* Initialize timer and port functions for IR communication */
    IR_INIT_TIMER();
#if IR_USE_XMIT
    IR_INIT_XMIT();
#endif

    IrCtrl.state = IR_IDLE;

    /* Enable receiving */
#if IR_USE_RCVR
    IR_CAPTURE_FALL();
    IR_CAPTURE_ENABLE();
#endif
}
