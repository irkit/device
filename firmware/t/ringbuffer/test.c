#include "ringbuffer.h"
#include "nanotap.h"

int main() {
    ok( 1, "ok" );

    {
        struct RingBuffer buf_;
        struct RingBuffer *buf = &buf_;
        char data[65];
        ring_init( buf, data, 65 );
        ok( ring_used(buf) == 0, "0 used" );
        ok( ring_isempty(buf) == 1, "is empty" );

        ring_put(buf, 'a');
        ok( ring_used(buf) == 1, "1 used after put" );
        ok( ring_isfull(buf) == 0, "not full" );
        ok( ring_isempty(buf) == 0, "is empty" );

        char buf2[64];
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

        ring_put(buf, '0');        ring_put(buf, '1');        ring_put(buf, '2');        ring_put(buf, '3');
        ring_put(buf, '4');        ring_put(buf, '5');        ring_put(buf, '6');        ring_put(buf, '7');
        ring_put(buf, '8');        ring_put(buf, '9');        ring_put(buf, 'a');        ring_put(buf, 'b');
        ring_put(buf, 'c');        ring_put(buf, 'd');        ring_put(buf, 'e');        ring_put(buf, 'f');
        ring_put(buf, '0');        ring_put(buf, '1');        ring_put(buf, '2');        ring_put(buf, '3');
        ring_put(buf, '4');        ring_put(buf, '5');        ring_put(buf, '6');        ring_put(buf, '7');
        ring_put(buf, '8');        ring_put(buf, '9');        ring_put(buf, 'a');        ring_put(buf, 'b');
        ring_put(buf, 'c');        ring_put(buf, 'd');        ring_put(buf, 'e');        ring_put(buf, 'f');
        ring_put(buf, '0');        ring_put(buf, '1');        ring_put(buf, '2');        ring_put(buf, '3');
        ring_put(buf, '4');        ring_put(buf, '5');        ring_put(buf, '6');        ring_put(buf, '7');
        ring_put(buf, '8');        ring_put(buf, '9');        ring_put(buf, 'a');        ring_put(buf, 'b');
        ring_put(buf, 'c');        ring_put(buf, 'd');        ring_put(buf, 'e');        ring_put(buf, 'f');
        ring_put(buf, '0');        ring_put(buf, '1');        ring_put(buf, '2');        ring_put(buf, '3');
        ring_put(buf, '4');        ring_put(buf, '5');        ring_put(buf, '6');        ring_put(buf, '7');
        ring_put(buf, '8');        ring_put(buf, '9');        ring_put(buf, 'a');        ring_put(buf, 'b');
        ring_put(buf, 'c');        ring_put(buf, 'd');        ring_put(buf, 'e');        ring_put(buf, 'f');
        ok( ring_used(buf) == 64, "64 used after 64 puts" );
        ok( ring_isfull(buf) == 1, "is full" );
        ok( ring_isempty(buf) == 0, "is empty" );

        ok( ring_put(buf, 'x') == -1, "can't put into full" );

        uint8_t fetched = ring_get(buf, &buf2[0], 64);
        ok( buf2[0] == '0', "get 1" );
        ok( buf2[1] == '1', "get 2" );
        ok( buf2[2] == '2', "get 3" );
        ok( buf2[3] == '3', "get 4" );
        ok( fetched == 64, "fetched all" );

        ok( ring_used(buf) == 0, "0 used after all get" );
        ok( ring_isfull(buf) == 0, "not full again" );
        ok( ring_isempty(buf) == 1, "is empty" );
    }

    done_testing();
}
