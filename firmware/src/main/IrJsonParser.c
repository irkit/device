#include "IrJsonParser.h"

// simple, specialized, JSON like string parser
// well, I'm fighting with 100bytes of program memory

void irjson_parse (char letter,
                   IrJsonParserStartEnd onStart,
                   IrJsonParserData onData,
                   IrJsonParserStartEnd onEnd) {
    static uint8_t  current_token;
    static uint32_t data;
    static uint8_t  data_exists;

    // special case only json parser
    // non-nested Object with following possible keys
    // (capital letter is unique to that key -> use that to identify key)
    // - Id
    // - fOrMat
    // - frEQ
    // - Data
    switch (letter) {
    case '{':
        current_token = IrJsonParserDataKeyUnknown;
        data          = 0;
        data_exists   = 0;
        onStart();
        break;
    case '}':
        if (data_exists) {
            onData(current_token, data);
            data        = 0;
            data_exists = 0;
        }
        onEnd();
        break;
    case ':':
        data          = 0;
        data_exists   = 0;
        break;
    case 'i':
        current_token = IrJsonParserDataKeyId;
    case 'o':
    case 'm':
        // format
        current_token = IrJsonParserDataKeyFormat;
        break;
    case 'e':
    case 'q':
        // freq
        current_token = IrJsonParserDataKeyFreq;
        break;
    case 'd':
        // data
        current_token = IrJsonParserDataKeyData;
        break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        if ( (current_token == IrJsonParserDataKeyId)   ||
             (current_token == IrJsonParserDataKeyFreq) ||
             (current_token == IrJsonParserDataKeyData) ) {
            if (data_exists) {
                data    *= 10;
            }
            data        += (letter - '0');
            data_exists  = 1;
        }
        break;
    case ',':
    case ']':
        if (data_exists) {
            onData(current_token, data);
            data        = 0;
            data_exists = 0;
        }
        break;
    default:
        break;
    }
}
