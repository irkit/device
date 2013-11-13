#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

// variable sized ring buffer without malloc

struct RingBuffer {
    char    *buf;
    uint8_t size;
    uint8_t addr_w, addr_r;
};

extern void    ring_init    (struct RingBuffer *ring, char *area, uint8_t size);
extern int8_t  ring_put     (struct RingBuffer *ring, char dat);
extern uint8_t ring_get     (struct RingBuffer *ring, char *dat, uint8_t len);
extern uint8_t ring_used    (struct RingBuffer *ring);
extern uint8_t ring_isfull  (struct RingBuffer *ring);
extern uint8_t ring_isempty (struct RingBuffer *ring);
extern void    ring_clear   (struct RingBuffer *ring);

#ifdef __cplusplus
}
#endif

#endif // __RINGBUFFER_H__
