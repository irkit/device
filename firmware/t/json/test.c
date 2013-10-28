#include "IrJsonParser.h"
#include "nanotap.h"
#include <string.h>

uint8_t start_called = 0;
uint8_t end_called   = 0;
uint16_t data_called_freq[10];
uint16_t data_called_data[10];
uint8_t data_called_freq_index;
uint8_t data_called_data_index;

void start () {
    start_called = 1;
}

void end () {
    end_called = 1;
}

void data (uint8_t key, uint16_t value) {
    if (key == IrJsonParserDataKeyFreq) {
        data_called_freq[ data_called_freq_index ++ ] = value;
    }
    else if (key == IrJsonParserDataKeyData) {
        data_called_data[ data_called_data_index ++ ] = value;
    }
}

void reset () {
    start_called           = 0;
    end_called             = 0;
    data_called_freq_index = 0;
    data_called_data_index = 0;
    memset( data_called_freq, 0, sizeof(data_called_freq) );
    memset( data_called_data, 0, sizeof(data_called_data) );
}

int main() {
    {
        reset();

        char buf[] = "{}";
        for (int i=0; i<strlen(buf); i++) {
            irjson_parse(buf[i], &start, &data, &end);
        }

        ok( start_called           == 1, "start called" );
        ok( end_called             == 1, "end called" );
        ok( data_called_freq_index == 0, "freq not called" );
        ok( data_called_data_index == 0, "data not called" );
        uint16_t expected[1] = { 0 };
        ok( memcmp( data_called_freq, expected, 1 ) == 0, "freq is same" );
        ok( memcmp( data_called_data, expected, 1 ) == 0, "data is same" );
    }

    {
        reset();

        char buf[] = "{\"format\":\"raw\",\"freq\":38,\"data\":[50489,9039,1205,1127]}";
        for (int i=0; i<strlen(buf); i++) {
            irjson_parse(buf[i], &start, &data, &end);
        }

        ok( start_called           == 1, "start called" );
        ok( end_called             == 1, "end called" );
        ok( data_called_freq_index == 1, "freq called" );
        ok( data_called_data_index == 4, "data called" );
        uint16_t expected1[1] = { 38 };
        ok( memcmp( data_called_freq, expected1, 1 ) == 0, "freq is same" );
        uint16_t expected2[4] = { 50489,9039,1205,1127 };
        ok( memcmp( data_called_data, expected2, 4 ) == 0, "data is same" );
    }

    done_testing();
}
