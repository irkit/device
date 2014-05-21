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
#include "CRC8.h"

uint8_t crc8 ( uint8_t *data, uint16_t size, uint8_t init )
{
    uint8_t crc, i;

    crc = init;

    while (size--) {
        crc ^= *data++;

        for (i=0; i<8; i++) {
            if (crc & 0x80) {
                crc = (crc<<1) ^ CRC8POLY;
            }
            else {
                crc <<= 1;
            }
        }
    }

    return crc;
}
