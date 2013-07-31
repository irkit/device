// MemoryFree library based on code posted here:
// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1213583720/15
//
// Extended by Matthew Murdoch to include walking of the free list.

#ifndef __MEMORY_FREE_H__
#define __MEMORY_FREE_H__

#ifdef __cplusplus
extern "C" {
#endif

int freeMemory();

#ifdef  __cplusplus
}
#endif

#endif // __MEMORY_FREE_H__
