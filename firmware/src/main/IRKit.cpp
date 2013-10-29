#include "pins.h"
#include "MemoryFree.h"
#include "pgmStrToRAM.h"
#include "IrCtrl.h"
#include "FullColorLed.h"
#include "version.h"
#include "GSwifi.h"
#include "WifiCredentials.h"
#include "FlexiTimer2.h"
#include "Global.h"
#include "MorseListener.h"
#include "IrJsonParser.h"

#define LED_BLINK_INTERVAL 200

// down -1- up -2- down -3- up -4- down -5- up
#define VALID_IR_LEN_MIN   5

// Serial1(RX=D0,TX=D1) is Wifi module's UART interface
GSwifi gs(&Serial1);

FullColorLed color( FULLCOLOR_LED_R, FULLCOLOR_LED_G, FULLCOLOR_LED_B );

MorseListener listener(MICROPHONE,13);

WifiCredentials credentials;

//--- declaration

void   reset3V3();
void   ir_recv_loop();
void   onTimer();
void   onDisconnect();
int8_t onGetRecent();
void   jsonDetectedStart();
void   jsonDetectedData( uint8_t key, uint16_t value );
void   jsonDetectedEnd();
int8_t onPostSend();
int8_t onRequest();
void   connect();
void   letterCallback( char letter );
void   wordCallback();
void   errorCallback();
void   printGuide(void);
void   IRKit_setup();
void   IRKit_loop();

//--- implementation

void reset3V3 () {
    Serial.println(P("hardware reset"));
    digitalWrite( LDO33_ENABLE, LOW );
    delay( 1000 );
    digitalWrite( LDO33_ENABLE, HIGH );

    // wait til gs wakes up
    delay( 1000 );
}

void ir_recv_loop(void) {
    char tmp[10];
    if ( IRDidRecvTimeout() ) {
        Serial.println(P("!!!\tIR recv timeout"));
        IR_state(IR_IDLE);
        return;
    }
    if (IrCtrl.state != IR_RECVED) {
        return;
    }
    if (IrCtrl.len < VALID_IR_LEN_MIN) {
        // data is too short = should be noise
        IR_state(IR_IDLE);
        return;
    }

    // can't receive here

    sprintf( tmp, P("%lu"), IrCtrl.overflowed );
    Serial.print(P("overflowed: "));  Serial.println( tmp );

    sprintf( tmp, P("%d"), freeMemory() );
    Serial.print(P("free:"));         Serial.println( tmp );

    sprintf( tmp, P("%x"), IrCtrl.len );
    Serial.print(P("received len:")); Serial.println( tmp );

    // start receiving again while leaving received data readable from central
    IR_state( IR_RECVED_IDLE );
}

void onTimer() {
    color.toggleBlink();
}

void onDisconnect() {
    Serial.println(P("onDisconnect"));
    connect();
}

int8_t onGetRecent() {
    if (gs._request.state != GSwifi::GSHTTPSTATE_RECEIVED) {
        Serial.println(P("GET with body??"));
        return -1;
    }
    gs.writeHead(200);
    if (IrCtrl.len <= 0) {
        // if no data
        gs.write(P("[]"));
        gs.end();
        return 0;
    }
    gs.write(P("[{"));
    gs.write(P("\"format\":\"raw\",")); // format fixed to "raw" for now
    gs.write(P("\"freq\":"));
    gs.write(IrCtrl.freq);
    gs.write(P(","));
    gs.write(P("\"data\":["));
    for (uint16_t i=0; i<IrCtrl.len; i++) {
        gs.write(IrCtrl.buff[i]);
        if (i != IrCtrl.len - 1) {
            gs.write(P(","));
        }
    }
    Serial.print(P(" "));
    gs.write(P("]}]"));
    gs.end();
    return 0;
}

void jsonDetectedStart() {
    Serial.println(P("json start"));

    gBufferMode = GBufferModeIR;
    IR_state( IR_WRITING );
}

void jsonDetectedData( uint8_t key, uint16_t value ) {
    Serial.print(P("json data: ")); Serial.println(value);
    switch (key) {
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

void jsonDetectedEnd() {
    Serial.println(P("json end, xmit"));
    IR_xmit();
}

int8_t onPostSend() {
    while (!ring_isempty(gs._buf_cmd)) {
        char letter;
        ring_get(gs._buf_cmd, &letter, 1);

        irjson_parse( letter,
                      &jsonDetectedStart,
                      &jsonDetectedData,
                      &jsonDetectedEnd );
    }
    return 0;
}

int8_t onRequest() {
    Serial.println(P("onRequest"));

    switch (gs._request.routeid) {
    case 0: // GET /recent
        return onGetRecent();

    case 1: // POST /send
        return onPostSend();

    default:
        break;
    }
    return -1;
}

void connect() {
    // load wifi credentials from EEPROM
    gBufferMode = GBufferModeWifiCredentials;
    credentials.load();

    if (credentials.isValid()) {
        color.setLedColor( 1, 1, 0, true ); // yellow blink if we have valid credentials

        gs.join(credentials.getSecurity(),
                credentials.getSSID(),
                credentials.getPassword());
    }
    else {
        Serial.println(P("!!! CLEAR EEPROM, ENABLE MORSE !!!"));
        credentials.dump();
        credentials.clear();

        color.setLedColor( 1, 0, 0, false );

        listener.enable(true);
    }

    if (gs.isJoined()) {
        color.setLedColor( 0, 1, 0, true );

        // 0
        gs.registerRoute( GSwifi::GSMETHOD_GET,  P("/recent") );
        // 1
        gs.registerRoute( GSwifi::GSMETHOD_POST, P("/send") );

        gs.setRequestHandler( &onRequest );

        // start http server
        gs.listen(80);
    }

    if (gs.isListening()) {
        color.setLedColor( 0, 1, 0, false );

        gBufferMode = GBufferModeUnused;
        IR_state( IR_IDLE );
    }

}

void letterCallback( char letter ) {
    Serial.print(P("letter: ")); Serial.write(letter); Serial.println();
    int8_t result = credentials.put( letter );
    if ( result != 0 ) {
        credentials.clear();
        Serial.println(P("cleared"));

        color.setLedColor( 1, 0, 0, false ); // red
    }
    else {
        color.setLedColor( 1, 0, 0, true ); // red blink
    }
}

void wordCallback() {
    Serial.println(P("word"));
    int8_t result = credentials.putDone();
    if ( result != 0 ) {
        credentials.clear();
        Serial.println(P("cleared"));

        color.setLedColor( 1, 0, 0, false ); // red
    }
    else {
        Serial.println(P("let's try connecting to wifi"));
        credentials.dump();
        credentials.save();
        connect();
    }
}

void errorCallback() {
    Serial.println(P("error"));
    credentials.clear();

    color.setLedColor( 1, 0, 0, false ); // red
}

void printGuide(void) {
    Serial.println(P("Operations Menu:"));
    Serial.println(P("h) Print this guide"));

    Serial.println(P("b) change baud rate to 9600"));
    Serial.println(P("B) change baud rate to 115200"));
    Serial.println(P("c) connect"));
    Serial.println(P("d) dump"));
    Serial.println(P("s) set credentials"));
    Serial.println(P("v) version"));

    Serial.println(P("Command?"));
}

void IRKit_setup() {
    //--- initialize LED

    FlexiTimer2::set( LED_BLINK_INTERVAL, &onTimer );
    FlexiTimer2::start();
    color.setLedColor( 1, 0, 0, false );

    //--- initialize morse listener

    pinMode(MICROPHONE,  INPUT);

    listener.letterCallback = &letterCallback;
    listener.wordCallback   = &wordCallback;
    listener.errorCallback  = &errorCallback;
    listener.setup();

    //--- initialize IR

    pinMode(IR_OUT,           OUTPUT);

    // pull-up
    pinMode(IR_IN,            INPUT);
    digitalWrite(IR_IN,       HIGH);

    IR_initialize();

    //--- initialize Wifi

    reset3V3();

    gs.setup( &onDisconnect );

    connect();

    printGuide();
}

void IRKit_loop() {
    static bool is_command_mode = false;

    global.loop(); // always run first

    listener.loop();

    // check if received
    ir_recv_loop();

    // wifi
    if ( ! is_command_mode ) {
        gs.loop();
    }
    else {
        if (Serial1.available()) {
            while (Serial1.available()) {
                uint8_t ch = Serial1.read();
                Serial.print(P("< 0x"));
                Serial.print(ch, HEX); Serial.print(P(" ")); Serial.write(ch); Serial.println();
            }
        }
    }

    // Wifi UART interface test
    if (Serial.available()) {
        char tmp[2];
        static uint8_t last_character = '0';
        last_character = Serial.read();

        Serial.print(P("> 0x"));
        sprintf( tmp, P("%x"), last_character );
        Serial.print(tmp);
        Serial.print(P(" "));
        Serial.write(last_character);
        Serial.println();
        Serial.print(P("free memory: 0x")); Serial.println( freeMemory(), HEX );

        if (is_command_mode) {
            Serial1.write(last_character);
            if ( last_character == 0x1B ) {
                is_command_mode = false;
                Serial.println(P("<< command mode finished !!!!"));
            }
        }
        else if (last_character == 0x1B) {
            is_command_mode = true;
            Serial.println(P(">> entered command mode !!!!"));
        }
        else if (last_character == 'h') {
            printGuide();
        }
        else if (last_character == 'b') {
            gs.setBaud(9600);
        }
        else if (last_character == 'B') {
            gs.setBaud(115200);
        }
        else if (last_character == 'c') {
            Serial.println(P("connect"));
            connect();
        }
        else if (last_character == 'd') {
            Serial.println(P("---credentials---"));
            credentials.dump();
            Serial.println();

            Serial.println(P("---wifi---"));
            gs.dump();
            Serial.println();

            Serial.println(P("---ir---"));
            IR_dump();
            Serial.println();
        }
        else if (last_character == 's') {
            Serial.println(P("setting credentials in EEPROM"));
            credentials.set(GSwifi::GSSECURITY_WPA2_PSK,
                            PB("Rhodos",2),
                            PB("aaaaaaaaaaaaa",3));
            credentials.save();
        }
        else if (last_character == 'v') {
            Serial.print(P("version: "));
            Serial.println(version);
        }
    }
}
