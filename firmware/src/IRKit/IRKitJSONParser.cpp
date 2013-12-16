#include "Arduino.h"
#include "pins.h"
#include "IRKitJSONParser.h"
#include "IrCtrl.h"
#include "FullColorLed.h"

extern FullColorLed color;
extern uint32_t newest_message_id;

// simple, specialized, JSON like string parser
// well, I'm fighting with 100bytes of program memory

typedef void (*IrJsonParserStartEnd)();
typedef void (*IrJsonParserData)(uint8_t key, uint32_t value);

static void jsonDetectedStart() {
    Serial.println("json<<");

    IR_state( IR_WRITING );
}

static void jsonDetectedData( uint8_t key, uint32_t value ) {
    if ( IrCtrl.state != IR_WRITING ) {
        return;
    }

    switch (key) {
    case IrJsonParserDataKeyId:
        newest_message_id = value;
        break;
    case IrJsonParserDataKeyFreq:
        IrCtrl.freq = value;
        break;
    case IrJsonParserDataKeyData:
        IR_put( value );
        break;
    default:
        break;
    }
}

static void jsonDetectedEnd() {
    Serial.println(">>json");

    if ( IrCtrl.state != IR_WRITING ) {
        Serial.println("!E5");
        IR_dump();
        return;
    }

    Serial.println(("xmit"));
    IR_xmit();
    color.setLedColor( 0, 0, 1, true, 1 ); // xmit: blue blink for 1sec
}

void irjson_parse (char letter,
                   IrJsonParserStartEnd onStart,
                   IrJsonParserData onData,
                   IrJsonParserStartEnd onEnd) {
    static uint8_t  current_token;
    static uint32_t data;
    static uint8_t  data_exists;
    static uint8_t  first_letter_of_key;
    static uint8_t  is_key;

    // special case only json parser
    // non-nested Object with following possible keys
    // (check only the first 2 letters to identify key)
    // - ID
    // - FOrmat
    // - FReq
    // - DAta
    switch (letter) {
    case '{':
        is_key = 0;
        onStart();
        break;
    case '}':
        if (data_exists) {
            onData(current_token, data);
        }
        onEnd();
        break;
    case '"':
        if ( ! is_key ) {
            // detected JSON Object's key
            is_key              = 1;
            first_letter_of_key = 0;
            current_token       = IrJsonParserDataKeyUnknown;
        }
        else {
            is_key              = 0;
        }
        break;
    case ':':
        data          = 0;
        data_exists   = 0;
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

    if (is_key && (letter != '"')) {
        if (! first_letter_of_key) {
            // save key's first letter
            first_letter_of_key = letter;
        }
        else if (current_token == IrJsonParserDataKeyUnknown) {
            // - id
            // - format
            // - freq
            // - data
            if (first_letter_of_key == 'i' && letter == 'd') {
                current_token = IrJsonParserDataKeyId;
            }
            else if (first_letter_of_key == 'f' && letter == 'o') {
                current_token = IrJsonParserDataKeyFormat;
            }
            else if (first_letter_of_key == 'f' && letter == 'r') {
                current_token = IrJsonParserDataKeyFreq;
            }
            else if (first_letter_of_key == 'd' && letter == 'a') {
                current_token = IrJsonParserDataKeyData;
            }
        }
    }
}

IRKitJSONParser::IRKitJSONParser() {
}

void IRKitJSONParser::parse( char letter ) {
    irjson_parse( letter,
                  &jsonDetectedStart,
                  &jsonDetectedData,
                  &jsonDetectedEnd );
}
