struct RingBuffer {
    char *buf;
    int size;
    int addr_w, addr_r;
} RingBuffer;
struct RingBuffer *ring_new (int p_size);
extern int  ring_put (struct RingBuffer *ring, char dat);
extern int  ring_get (struct RingBuffer *ring, char *dat, int len);
extern void ring_clear (struct RingBuffer *ring);
extern int  ring_available (struct RingBuffer *ring);
