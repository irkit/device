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

extern inline void    ring_init    (volatile struct RingBuffer *ring, volatile char *area, uint8_t size);
extern inline int8_t  ring_put     (volatile struct RingBuffer *ring, char dat);
extern inline uint8_t ring_get     (volatile struct RingBuffer *ring, char *dat, uint8_t len);
extern inline uint8_t ring_isfull  (volatile struct RingBuffer *ring);
extern inline uint8_t ring_isempty (volatile struct RingBuffer *ring);
extern inline uint8_t ring_used    (volatile struct RingBuffer *ring);
extern inline void    ring_clear   (volatile struct RingBuffer *ring);

#ifdef __cplusplus
}
#endif

#endif // __RINGBUFFER_H__
