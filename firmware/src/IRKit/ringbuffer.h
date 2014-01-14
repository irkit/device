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
#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

// variable sized ring buffer without malloc

struct RingBuffer {
    char    *buf;
    uint8_t size;
    uint8_t addr_w, addr_r;
};

extern inline void    ring_init    (volatile struct RingBuffer *ring, volatile char *area, uint8_t size);
extern inline int8_t  ring_put     (volatile struct RingBuffer *ring, char dat);
extern inline uint8_t ring_get     (volatile struct RingBuffer *ring, char *dat, uint8_t len);
extern inline uint8_t ring_isfull  (volatile struct RingBuffer *ring);
extern inline uint8_t ring_isempty (volatile struct RingBuffer *ring);
extern inline uint8_t ring_used    (volatile struct RingBuffer *ring);
extern inline void    ring_clear   (volatile struct RingBuffer *ring);

#ifdef __cplusplus
}
#endif

#endif // __RINGBUFFER_H__
