#ifndef __IRJSONPARSER_H__
#define __IRJSONPARSER_H__

#include <inttypes.h>

#define IrJsonParserDataKeyId      0x01
#define IrJsonParserDataKeyFormat  0x02
#define IrJsonParserDataKeyFreq    0x03
#define IrJsonParserDataKeyData    0x04
#define IrJsonParserDataKeyUnknown 0xFF

class IRKitJSONParser {
 public:
    IRKitJSONParser();
    void parse( char );

 private:
};

#endif // __IRJSONPARSER_H__
