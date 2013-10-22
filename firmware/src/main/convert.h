#ifndef __CONVERT_H__
#define __CONVERT_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int  x2i (char c);
extern char i2x (int i);
extern int  from_hex (int ch);
extern int  to_hex (int code);

#ifdef  __cplusplus
}
#endif

#endif // __CONVERT_H__
