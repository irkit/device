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
#ifndef __IRPACKER_H__
#define __IRPACKER_H__

#include <inttypes.h>
#include <stdbool.h>

#define TREE_SIZE 168

struct irpacker_t {
    volatile uint8_t *buff;
    uint16_t  length;

    // bitpack
    uint8_t  val0;
    uint8_t  val1;
    uint8_t  bit_index;
    uint16_t byte_index;
    uint8_t  bitpack_length;
};

typedef void (*IrPackerUnpackCallback)(uint16_t);

#ifdef __cplusplus
extern "C" {
#endif

extern void irpacker_init( volatile struct irpacker_t *state, volatile uint8_t *buff );
extern void irpacker_clear( volatile struct irpacker_t *state );
extern void irpacker_reset( volatile struct irpacker_t *state );
extern void irpacker_pack( volatile struct irpacker_t *state, uint16_t data );
extern void irpacker_packend( volatile struct irpacker_t *state );
extern uint16_t irpacker_safelength( const volatile struct irpacker_t *state );
extern uint16_t irpacker_length( const volatile struct irpacker_t *state );
extern void irpacker_unpack_start( volatile struct irpacker_t *state );
extern uint16_t irpacker_unpack( volatile struct irpacker_t *state );
extern void irpacker_unpack_sequence( volatile struct irpacker_t *state, uint8_t *in, uint16_t length, IrPackerUnpackCallback callback );

#ifdef __cplusplus
}
#endif

#endif // __IRPACKER_H__
