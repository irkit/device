#ifndef __IRKITJSONPARSER_H__
#define __IRKITJSONPARSER_H__

#include <inttypes.h>

#define IrJsonParserDataKeyId      0x01
#define IrJsonParserDataKeyFormat  0x02
#define IrJsonParserDataKeyFreq    0x03
#define IrJsonParserDataKeyData    0x04
#define IrJsonParserDataKeyUnknown 0xFF

typedef void (*JSONParserStartEnd)();
typedef void (*JSONParserData)(uint8_t key, uint32_t value);

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
