#include "ringbuffer.h"
#include "nanotap.h"

int main() {
    ok( 1, "ok" );

    {
        struct RingBuffer *buf = ring_new(4);
        ok( ring_used(buf) == 0, "0 used" );
        ok( ring_isempty(buf) == 1, "is empty" );

        ring_put(buf, 'a');
        ok( ring_used(buf) == 1, "1 used after put" );
        ok( ring_isfull(buf) == 0, "not full" );
        ok( ring_isempty(buf) == 0, "is empty" );

        char buf2[10];
        ring_get(buf, &buf2[0], 1);
        ok( buf2[0] == 'a', "get" );
        ok( ring_used(buf) == 0, "0 used after get" );
        ok( ring_isfull(buf) == 0, "not full" );
        ok( ring_isempty(buf) == 1, "is empty" );

        ring_put(buf, 'b');
        ring_clear(buf);
        ok( ring_used(buf) == 0, "0 used after clear" );
        ok( ring_isfull(buf) == 0, "not full" );
        ok( ring_isempty(buf) == 1, "is empty" );

        ring_put(buf, 'a');
        ring_put(buf, 'b');
        ring_put(buf, 'c');
        ring_put(buf, 'd');
        ok( ring_used(buf) == 4, "4 used after 4 puts" );
        ok( ring_isfull(buf) == 1, "is full" );
        ok( ring_isempty(buf) == 0, "is empty" );

        ring_get(buf, &buf2[0], 4);
        ok( buf2[0] == 'a', "get 1" );
        ok( buf2[1] == 'b', "get 2" );
        ok( buf2[2] == 'c', "get 3" );
        ok( buf2[3] == 'd', "get 4" );
        ok( ring_used(buf) == 0, "0 used after all get" );
        ok( ring_isfull(buf) == 0, "not full again" );
        ok( ring_isempty(buf) == 1, "is empty" );
    }

    done_testing();
}
