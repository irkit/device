#ifndef __IRPACKER_H__
#define __IRPACKER_H__

#include <inttypes.h>
#include <stdbool.h>

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
extern void irpacker_pack( volatile struct irpacker_t *state, uint16_t data );
extern void irpacker_packend( volatile struct irpacker_t *state );
extern uint16_t irpacker_safelength( const volatile struct irpacker_t *state );
extern uint16_t irpacker_length( const volatile struct irpacker_t *state );
extern void irpacker_unpack_start( volatile struct irpacker_t *state );
extern uint16_t irpacker_unpack( volatile struct irpacker_t *state );
extern void irpacker_load( void *offset );

#ifdef __cplusplus
}
#endif

#endif // __IRPACKER_H__
