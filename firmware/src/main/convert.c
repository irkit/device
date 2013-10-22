#include "Arduino.h"
#include "convert.h"

int from_hex (int ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

int to_hex (int code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

int x2i (char c) {
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

char i2x (int i) {
    if (i >= 0 && i <= 9) {
        return i + '0';
    } else
    if (i >= 10 && i <= 15) {
        return i - 10 + 'A';
    }
    return '0';
}
