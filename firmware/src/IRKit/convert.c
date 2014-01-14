/*
 Copyright (C) 2013-2014 Masakazu Ohtsuka
  
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
  
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
  
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
