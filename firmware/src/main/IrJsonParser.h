#ifndef __IRJSONPARSER_H__
#define __IRJSONPARSER_H__

#include <inttypes.h>

#define IrJsonParserDataKeyId      0x01
#define IrJsonParserDataKeyFormat  0x02
#define IrJsonParserDataKeyFreq    0x03
#define IrJsonParserDataKeyData    0x04
#define IrJsonParserDataKeyUnknown 0xFF

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*IrJsonParserStartEnd)();
typedef void (*IrJsonParserData)(uint8_t key, uint32_t value);

extern void irjson_parse (char letter,
                          IrJsonParserStartEnd start,
                          IrJsonParserData data,
                          IrJsonParserStartEnd end);

#ifdef __cplusplus
}
#endif

#endif // __IRJSONPARSER_H__
