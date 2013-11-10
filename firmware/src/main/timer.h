#ifndef __TIMER_H__
#define __TIMER_H__

// 255: off, 0: dispatch, 0<t<255: timer running
#define TIMER_OFF          255
#define TIMER_FIRE         0

#define TIMER_RUNNING(a)   ((a != TIMER_OFF) && (a>0))
#define TIMER_COUNTDOWN(a) (a --)
#define TIMER_RUN(a)       if (TIMER_RUNNING(a)) { TIMER_COUNTDOWN(a); }
#define TIMER_FIRED(a)     (a == TIMER_FIRE)
#define TIMER_STOP(a)      (a = TIMER_OFF)

#endif // __TIMER_H__
