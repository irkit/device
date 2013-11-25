#include "ringbuffer.h"

inline void ring_init (volatile struct RingBuffer *ring, volatile char *area, uint8_t size) {
    ring->buf    = area;
    ring->size   = size;
    ring->addr_w = 0;
    ring->addr_r = 0;
}

inline int8_t ring_put (volatile struct RingBuffer *ring, char dat) {
    ring->buf[ ring->addr_w ] = dat;
    ring->addr_w = (ring->addr_w + 1) % ring->size;
    return 0;
}

inline uint8_t ring_get (volatile struct RingBuffer *ring, char *dat, uint8_t len) {
    uint8_t i;

    for (i = 0; i < len; i ++) {
        if (ring_isempty(ring)) {
            break;
        }
        dat[i] = ring->buf[ring->addr_r];
        ring->addr_r = (ring->addr_r + 1) % ring->size;
    }
    return i;
}

inline uint8_t ring_isfull (volatile struct RingBuffer *ring) {
    return (ring->addr_w + 1) % ring->size == ring->addr_r;
}

inline uint8_t ring_isempty (volatile struct RingBuffer *ring) {
    return ring->addr_r == ring->addr_w;
}

inline uint8_t ring_used (volatile struct RingBuffer *ring) {
    if (ring->addr_r <= ring->addr_w) {
        return ring->addr_w - ring->addr_r;
    }
    else {
        return ring->size - ring->addr_r + ring->addr_w;
    }
}

inline void ring_clear (volatile struct RingBuffer *ring) {
    ring->addr_w = 0;
    ring->addr_r = 0;
}
