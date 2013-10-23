#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char uint8_t;

extern struct RingBuffer *ring_new (uint8_t p_size);
extern void    ring_put (struct RingBuffer *ring, char dat);
extern uint8_t ring_get (struct RingBuffer *ring, char *dat, uint8_t len);
extern uint8_t ring_used (struct RingBuffer *ring);
extern uint8_t ring_isfull  (struct RingBuffer *ring);
extern void    ring_clear (struct RingBuffer *ring);

#ifdef __cplusplus
}
#endif

#endif // __RINGBUFFER_H__
