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
#ifndef __CRC8_H__
#define __CRC8_H__

#include <inttypes.h>

#define CRC8INIT 0x00
#define CRC8POLY 0x31 // = X^8+X^5+X^4+X^0

#ifdef __cplusplus
extern "C" {
#endif

uint8_t crc8 (uint8_t* data, uint16_t size, uint8_t init);

#ifdef __cplusplus
}
#endif

#endif
