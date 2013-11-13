#include "ringbuffer.h"

void ring_init (struct RingBuffer *ring, char *area, uint8_t size) {
    ring->buf    = area;
    ring->size   = size;
    ring->addr_w = 0;
    ring->addr_r = 0;
}

int8_t ring_put (struct RingBuffer *ring, char dat) {
    if (ring_isfull(ring)) {
        return -1;
    }
    ring->buf[ ring->addr_w ] = dat;
    ring->addr_w = (ring->addr_w + 1) % ring->size;
    return 0;
}

uint8_t ring_get (struct RingBuffer *ring, char *dat, uint8_t len) {
    uint8_t i;

    for (i = 0; i < len; i ++) {
        if (ring->addr_r == ring->addr_w) {
            break;
        }
        dat[i] = ring->buf[ring->addr_r];
        ring->addr_r = (ring->addr_r + 1) % ring->size;
    }
    return i;
}

uint8_t ring_used (struct RingBuffer *ring) {
    if (ring->addr_r <= ring->addr_w) {
        return ring->addr_w - ring->addr_r;
    }
    else {
        return ring->size - ring->addr_r + ring->addr_w;
    }
}

uint8_t ring_isfull (struct RingBuffer *ring) {
    return ring_used( ring ) == (ring->size - 1);
}

uint8_t ring_isempty (struct RingBuffer *ring) {
    return ring_used( ring ) == 0;
}

void ring_clear (struct RingBuffer *ring) {
    ring->addr_w = 0;
    ring->addr_r = 0;
}
