#include "Arduino.h"
#include "convert.h"

uint8_t x2i (char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return 0;
}

char i2x (uint8_t x) {
    if (x >= 0 && x <= 9) {
        return x + '0';
    }
    else if (x >= 10 && x <= 15) {
        return x - 10 + 'a';
    }
    return 0;
}
