//#define DEBUG
//#define DEBUG_VIEW

#ifdef DEBUG
#define DBG(...) printf("" __VA_ARGS__)
#else
#define DBG(...)
#endif
