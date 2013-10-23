#include "ringbuffer.h"
#include "nanotap.h"

int main() {
    ok( 1, "ok" );

    {
        struct RingBuffer *buf = ring_new(10);
        ok( ring_available(buf) == 10, "available" );

        ring_put(buf, 'a');
        ok( ring_available(buf) == 9, "available after put" );

        char buf2[10];
        ring_get(buf, &buf2[0], 1);
        ok( buf2[0] == 'a', "get" );
        ok( ring_available(buf) == 10, "available after get" );

        ring_put(buf, 'b');
        ring_clear(buf);
        ok( ring_available(buf) == 10, "available after clear" );
    }

    done_testing();
}
