/*
  HardwareSerialX.cpp - Hardware serial library for Wiring
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Modified 23 November 2006 by David A. Mellis
  Modified 28 September 2010 by Mark Sproul
  Modified 14 August 2012 by Alarus
  Modified Nov 2013 by mash
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "Arduino.h"
#include "wiring_private.h"
#include "HardwareSerialX.h"

/*
 * on ATmega8, the uart and its bits are not numbered, so there is no "TXC0"
 * definition.
 */
#if !defined(TXC0)
#if defined(TXC)
#define TXC0 TXC
#elif defined(TXC1)
// Some devices have uart1 but no uart0
#define TXC0 TXC1
#else
#error TXC0 not definable in HardwareSerial.h
#endif
#endif

#define RX_BUFFER_SIZE     64
#define TX_BUFFER_SIZE     64
#define XONOFF_HYSTERESIS   4
volatile static struct RingBuffer rx_buffer1;
volatile static char rx_buffer1_data[RX_BUFFER_SIZE + 1];
volatile static struct RingBuffer tx_buffer1;
volatile static char tx_buffer1_data[TX_BUFFER_SIZE + 1];

#define XON  0x11
#define XOFF 0x13
volatile static bool is_limiting = false;
volatile static bool send_xon    = false;
volatile static bool send_xoff   = false;

ISR(USART1_RX_vect)
{
  if (bit_is_clear(UCSR1A, UPE1)) {
    unsigned char c = UDR1;
    if ( !ring_isfull( &rx_buffer1 ) ) {
      ring_put( &rx_buffer1, c );
    }

    if (! is_limiting &&
        ring_used( &rx_buffer1 ) > (RX_BUFFER_SIZE - XONOFF_HYSTERESIS)) {
      // limit if we're going to reach full soon
      is_limiting = true;
      if (bit_is_set(UCSR1A, UDRE1)) {
        // if USART Data Register Empty is 1, send immediately
        UDR1 = XOFF;
      }
      else {
        // if not empty, send on next interrupt
        send_xoff = true;
        sbi(UCSR1B, UDRIE1);
      }
    }

  } else {
    unsigned char c = UDR1;
  }
}

ISR(USART1_UDRE_vect)
{
  // actually send xon/xoff
  if (send_xoff) {
    send_xoff = false;
    UDR1      = XOFF;
    return;
  }
  if (send_xon) {
    send_xon = false;
    UDR1     = XON;
    return;
  }
  if (ring_isempty( &tx_buffer1 )) {
    // Buffer empty, so disable interrupts
    cbi(UCSR1B, UDRIE1);
  }
  else {
    // There is more data in the output buffer. Send the next byte
    char c;
    ring_get( &tx_buffer1, &c, 1 );
    UDR1 = c;
  }
}

// Constructors ////////////////////////////////////////////////////////////////

HardwareSerialX::HardwareSerialX(volatile RingBuffer *rx_buffer, volatile RingBuffer *tx_buffer,
  volatile uint8_t *ubrrh, volatile uint8_t *ubrrl,
  volatile uint8_t *ucsra, volatile uint8_t *ucsrb,
  volatile uint8_t *ucsrc, volatile uint8_t *udr,
  uint8_t rxen, uint8_t txen, uint8_t rxcie, uint8_t udrie, uint8_t u2x)
{
  ring_init( &rx_buffer1, rx_buffer1_data, RX_BUFFER_SIZE + 1 );
  ring_init( &tx_buffer1, tx_buffer1_data, TX_BUFFER_SIZE + 1 );

  _rx_buffer = rx_buffer;
  _tx_buffer = tx_buffer;
  _ubrrh = ubrrh;
  _ubrrl = ubrrl;
  _ucsra = ucsra;
  _ucsrb = ucsrb;
  _ucsrc = ucsrc;
  _udr = udr;
  _rxen = rxen;
  _txen = txen;
  _rxcie = rxcie;
  _udrie = udrie;
  _u2x = u2x;
}

// Public Methods //////////////////////////////////////////////////////////////

void HardwareSerialX::begin(unsigned long baud)
{
  uint16_t baud_setting;
  bool use_u2x = true;

#if F_CPU == 16000000UL
  // hardcoded exception for compatibility with the bootloader shipped
  // with the Duemilanove and previous boards and the firmware on the 8U2
  // on the Uno and Mega 2560.
  if (baud == 57600) {
    use_u2x = false;
  }
#endif

try_again:

  if (use_u2x) {
    *_ucsra = 1 << _u2x;
    baud_setting = (F_CPU / 4 / baud - 1) / 2;
  } else {
    *_ucsra = 0;
    baud_setting = (F_CPU / 8 / baud - 1) / 2;
  }

  if ((baud_setting > 4095) && use_u2x)
  {
    use_u2x = false;
    goto try_again;
  }

  // assign the baud_setting, a.k.a. ubbr (USART Baud Rate Register)
  *_ubrrh = baud_setting >> 8;
  *_ubrrl = baud_setting;

  transmitting = false;

  sbi(*_ucsrb, _rxen);
  sbi(*_ucsrb, _txen);
  sbi(*_ucsrb, _rxcie);
  cbi(*_ucsrb, _udrie);
}

void HardwareSerialX::begin(unsigned long baud, byte config)
{
  uint16_t baud_setting;
  uint8_t current_config;
  bool use_u2x = true;

#if F_CPU == 16000000UL
  // hardcoded exception for compatibility with the bootloader shipped
  // with the Duemilanove and previous boards and the firmware on the 8U2
  // on the Uno and Mega 2560.
  if (baud == 57600) {
    use_u2x = false;
  }
#endif

try_again:

  if (use_u2x) {
    *_ucsra = 1 << _u2x;
    baud_setting = (F_CPU / 4 / baud - 1) / 2;
  } else {
    *_ucsra = 0;
    baud_setting = (F_CPU / 8 / baud - 1) / 2;
  }

  if ((baud_setting > 4095) && use_u2x)
  {
    use_u2x = false;
    goto try_again;
  }

  // assign the baud_setting, a.k.a. ubbr (USART Baud Rate Register)
  *_ubrrh = baud_setting >> 8;
  *_ubrrl = baud_setting;

  //set the data bits, parity, and stop bits
#if defined(__AVR_ATmega8__)
  config |= 0x80; // select UCSRC register (shared with UBRRH)
#endif
  *_ucsrc = config;

  sbi(*_ucsrb, _rxen);
  sbi(*_ucsrb, _txen);
  sbi(*_ucsrb, _rxcie);
  cbi(*_ucsrb, _udrie);
}

void HardwareSerialX::end()
{
  // wait for transmission of outgoing data
  while ( ! ring_isempty( _tx_buffer ) ) ;

  cbi(*_ucsrb, _rxen);
  cbi(*_ucsrb, _txen);
  cbi(*_ucsrb, _rxcie);
  cbi(*_ucsrb, _udrie);

  // clear any received data
  ring_clear( _rx_buffer );
}

int HardwareSerialX::available(void)
{
  return ! ring_isempty( _rx_buffer );
}

int HardwareSerialX::peek(void)
{
  return -1;
}

int HardwareSerialX::read(void)
{
  // if the head isn't ahead of the tail, we don't have any characters
  if (ring_isempty( _rx_buffer )) {
    return -1;
  }
  else {
    char c;
    ring_get( _rx_buffer, &c, 1 );

    if (is_limiting &&
        ring_used(_rx_buffer) < XONOFF_HYSTERESIS) {
        send_xon    = true;
        is_limiting = false;
        sbi(UCSR1B, UDRIE1);
    }

    return c;
  }
}

void HardwareSerialX::flush()
{
  // UDR is kept full while the buffer is not empty, so TXC triggers when EMPTY && SENT
  while (transmitting && ! (*_ucsra & _BV(TXC0)));
  transmitting = false;
}

size_t HardwareSerialX::write(uint8_t c)
{
  // If the output buffer is full, there's nothing for it other than to
  // wait for the interrupt handler to empty it a bit
  // ???: return 0 here instead?
  while (ring_isfull( _tx_buffer )) ;

  ring_put( _tx_buffer, c );

  sbi(*_ucsrb, _udrie);
  // clear the TXC bit -- "can be cleared by writing a one to its bit location"
  transmitting = true;
  sbi(*_ucsra, TXC0);

  return 1;
}

HardwareSerialX::operator bool() {
  return true;
}

// Preinstantiate Objects //////////////////////////////////////////////////////

HardwareSerialX Serial1X(&rx_buffer1, &tx_buffer1, &UBRR1H, &UBRR1L, &UCSR1A, &UCSR1B, &UCSR1C, &UDR1, RXEN1, TXEN1, RXCIE1, UDRIE1, U2X1);
