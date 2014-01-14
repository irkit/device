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
#ifndef __TIMER_H__
#define __TIMER_H__

#define TIMER_INTERVAL     200 // [ms]

// be careful about overflow
#define TIMER_START(a,sec) (a = sec * 5)
#define TIMER_STOP(a)      (a = TIMER_OFF)

// 255: off, 0: dispatch, 0<t<255: timer running
#define TIMER_OFF          255
#define TIMER_FIRE         0

#define TIMER_RUNNING(a)   ((a != TIMER_OFF) && (a>0))
#define TIMER_COUNTDOWN(a) (a --)
#define TIMER_TICK(a)      if (TIMER_RUNNING(a)) { TIMER_COUNTDOWN(a); }
#define TIMER_FIRED(a)     (a == TIMER_FIRE)
#define TIMER_STOPPED(a)   (a == TIMER_OFF)

#ifdef __cplusplus
extern "C" {
#endif

void timer_init( void (*callback)() );
void timer_start( uint16_t interval_ms );
void timer_stop( void );

#ifdef __cplusplus
}
#endif

#endif // __TIMER_H__
