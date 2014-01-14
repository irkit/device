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
#ifndef __BASE64ENCODER_H__
#define __BASE64ENCODER_H__

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*Base64EncodeCallback)(char);
extern uint16_t base64_length(uint16_t input_length);
extern void base64_encode(const uint8_t *input,
                          uint16_t input_length,
                          Base64EncodeCallback callback);

#ifdef __cplusplus
}
#endif

#endif
