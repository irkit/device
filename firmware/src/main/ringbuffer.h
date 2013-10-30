#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define RING_SIZE 64

typedef unsigned char uint8_t;

struct RingBuffer {
    char    buf[RING_SIZE + 1];
    uint8_t size;
    uint8_t addr_w, addr_r;
};

extern void    ring_init    (struct RingBuffer *ring);
extern void    ring_put     (struct RingBuffer *ring, char dat);
extern uint8_t ring_get     (struct RingBuffer *ring, char *dat, uint8_t len);
extern uint8_t ring_used    (struct RingBuffer *ring);
extern uint8_t ring_isfull  (struct RingBuffer *ring);
extern uint8_t ring_isempty (struct RingBuffer *ring);
extern void    ring_clear   (struct RingBuffer *ring);

#ifdef __cplusplus
}
#endif

#endif // __RINGBUFFER_H__
