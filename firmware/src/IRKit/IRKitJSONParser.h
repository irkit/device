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
#ifndef __IRKITJSONPARSER_H__
#define __IRKITJSONPARSER_H__

#include <inttypes.h>

#define IrJsonParserDataKeyId      0x01
#define IrJsonParserDataKeyFormat  0x02
#define IrJsonParserDataKeyFreq    0x03
#define IrJsonParserDataKeyData    0x04
#define IrJsonParserDataKeyPass    0x05
#define IrJsonParserDataKeyUnknown 0xFF

typedef void (*JSONParserStartEnd)();
typedef void (*JSONParserData)(uint8_t key, uint32_t value, char *pass);

#ifdef __cplusplus
extern "C" {
#endif

extern void irkit_json_parse (char letter,
                              JSONParserStartEnd on_start,
                              JSONParserData on_data,
                              JSONParserStartEnd on_end);

#ifdef __cplusplus
}
#endif

#endif // __IRKITJSONPARSER_H__
