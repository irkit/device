#include "ringbuffer.h"
#include <stdlib.h> // for malloc

struct RingBuffer *ring_new (int p_size) {
    struct RingBuffer *ring;
    ring         = (struct RingBuffer *)malloc(sizeof(struct RingBuffer));
    ring->size   = p_size + 1;
    ring->buf    = (char*)malloc(ring->size);
    ring->addr_w = 0;
    ring->addr_r = 0;
    return ring;
}

int ring_put (struct RingBuffer *ring, char dat) {
    int next;

    next = (ring->addr_w + 1) % ring->size;
    if (next == ring->addr_r) {
        return -1;
    }
    ring->buf[ring->addr_w] = dat;
    ring->addr_w = next;
    return -1;
}

int ring_get (struct RingBuffer *ring, char *dat, int len) {
    int i;

    for (i = 0; i < len; i ++) {
        if (ring->addr_r == ring->addr_w) {
            break;
        }
        dat[i] = ring->buf[ring->addr_r];
        ring->addr_r = (ring->addr_r + 1) % ring->size;
    }
    return i;
}

int ring_available (struct RingBuffer *ring) {
    if (ring->addr_w < ring->addr_r) {
        return ring->addr_r - ring->addr_w - 1;
    } else {
        return (ring->size - ring->addr_w) + ring->addr_r - 1;
    }
}

void ring_clear (struct RingBuffer *ring) {
    ring->addr_w = 0;
    ring->addr_r = 0;
}
