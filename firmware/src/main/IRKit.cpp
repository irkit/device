#include "pins.h"
#include "MemoryFree.h"
#include "pgmStrToRAM.h"
#include "IrCtrl.h"
#include "FullColorLed.h"
#include "version.h"
#include "GSwifi.h"
#include "Keys.h"
#include "FlexiTimer2.h"
#include "Global.h"
#include "MorseListener.h"
#include "IrJsonParser.h"
#include "timer.h"
#include "LongPressButton.h"

// Serial1(RX=D0,TX=D1) is Wifi module's UART interface
GSwifi gs(&Serial1);

FullColorLed color( FULLCOLOR_LED_R, FULLCOLOR_LED_G, FULLCOLOR_LED_B );

MorseListener listener(MICROPHONE,13);

LongPressButton clear_button(RESET_SWITCH, 5);

Keys keys;
volatile static uint8_t message_timer   = TIMER_OFF;
volatile static uint8_t reconnect_timer = TIMER_OFF;
static uint32_t newest_message_id = 0; // on memory only should be fine
static bool     morse_error       = 0;
static uint8_t  post_keys_cid;

#define NEXT_TICK_POST_KEYS 1
#define NEXT_TICK_NOP       0

static uint8_t on_next_tick = NEXT_TICK_NOP;

//--- declaration

void   reset3V3();
void   longPressed();
void   timerLoop();
void   onTimer();
int8_t onReset();
int8_t onDisconnect();
int8_t onGetMessagesRequest();
void   jsonDetectedStart();
void   jsonDetectedData( uint8_t key, uint16_t value );
void   jsonDetectedEnd();
void   onIRXmitComplete();
int8_t onPostMessagesRequest();
int8_t onPostKeysRequest();
int8_t onRequest();
int8_t onPostDoorResponse();
int8_t onGetMessagesResponse();
int8_t onPostKeysResponse();
void   postDoor();
int8_t getMessages();
void   postKeys();
void   connect();
void   startNormalOperation();
void   letterCallback( char letter );
void   wordCallback();
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

void longPressed() {
    Serial.println(P("long pressed"));
    keys.clear();
    reset3V3();
}

void timerLoop() {
    // long poll
    if (TIMER_FIRED(message_timer)) {
        TIMER_STOP(message_timer);
        int8_t result = getMessages();
        if ( result != 0 ) {
            // reset if any error happens
            Serial.println(P("!!!E3"));
            reset3V3();
        }
    }

    // reconnect
    if (TIMER_FIRED(reconnect_timer)) {
        TIMER_STOP(reconnect_timer);
        connect();
    }

    switch (on_next_tick) {
    case NEXT_TICK_POST_KEYS:
        postKeys();
        break;
    case NEXT_TICK_NOP:
    default:
        break;
    }
    on_next_tick = NEXT_TICK_NOP;
}

// inside ISR, be careful
void onTimer() {
    color.toggleBlink(); // 200msec blink

    TIMER_TICK( message_timer );

    TIMER_TICK( reconnect_timer );

    gs.onTimer();

    IR_timer();

    clear_button.onTimer();
}

int8_t onReset() {
    Serial.println(P("!!! onReset"));

    gs.setup( &onDisconnect, &onReset );

    connect();
    return 0;
}

int8_t onDisconnect() {
    Serial.println(P("!!! onDisconnect"));

    connect();
    return 0;
}

int8_t onGetMessagesRequest() {
    if (gs.serverRequest.state != GSwifi::GSREQUESTSTATE_RECEIVED) {
        Serial.println(P("!!! GET with body??"));
        return -1;
    }
    gs.writeHead(200);

    if ( (global.buffer_mode == GBufferModeWifiCredentials) ||
         (IrCtrl.len <= 0) ||
         (IrCtrl.state != IR_RECVED) ) {
        // if no data
        gs.end();
        return 0;
    }

    IR_state( IR_READING );

    gs.write(P("{\"format\":\"raw\",\"freq\":")); // format fixed to "raw" for now
    gs.write(IrCtrl.freq);
    gs.write(P(",\"data\":["));
    for (uint16_t i=0; i<IrCtrl.len; i++) {
        gs.write(IrCtrl.buff[i]);
        if (i != IrCtrl.len - 1) {
            gs.write(",");
        }
    }
    gs.write(P("]}"));
    gs.end();

    IR_state( IR_IDLE );

    return 0;
}

void jsonDetectedStart() {
    Serial.println(P("json start"));

    if (global.buffer_mode != GBufferModeWifiCredentials) {
        IR_state( IR_WRITING );
    }
}

void jsonDetectedData( uint8_t key, uint32_t value ) {
    Serial.print(P("json data: ")); Serial.print(key); Serial.print(","); Serial.println(value);

    if ( (IrCtrl.state != IR_WRITING) ||
         (global.buffer_mode != GBufferModeIR) ) {
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

void jsonDetectedEnd() {
    Serial.println(P("json end"));

    if ( (IrCtrl.state != IR_WRITING) ||
         (global.buffer_mode != GBufferModeIR) ) {
        Serial.print("buffer_mode:"); Serial.println(global.buffer_mode);
        IR_dump();
        return;
    }

    Serial.println(P("xmit"));
    IR_xmit();
}

int8_t onPostMessagesRequest() {
    while (!ring_isempty(gs._buf_cmd)) {
        char letter;
        ring_get(gs._buf_cmd, &letter, 1);

        irjson_parse( letter,
                      &jsonDetectedStart,
                      &jsonDetectedData,
                      &jsonDetectedEnd );
    }

    if (gs.serverRequest.state == GSwifi::GSREQUESTSTATE_RECEIVED) {
        // should be xmitting or idle (xmit finished)
        if (IrCtrl.state == IR_WRITING) {
            // invalid json
            gs.writeHead(400);
            gs.end();
            return 0;
        }
        gs.writeHead(200);
        gs.end();
    }

    return 0;
}

int8_t onPostKeysRequest() {
    if (gs.serverRequest.state == GSwifi::GSREQUESTSTATE_RECEIVED) {
        // don't close other client requests, we can handle multiple concurrent client requests
        // and "close" and it's response mixing up makes things difficult

        // respond to this cid, when we get a new key
        post_keys_cid = gs.serverRequest.cid;

        // delay execution to next tick (we get clean stack)
        // POST /keys to server
        on_next_tick = NEXT_TICK_POST_KEYS;
    }
}

int8_t onRequest() {
    Serial.println(P("onRequest"));

    switch (gs.serverRequest.routeid) {
    case 0: // GET /messages
        return onGetMessagesRequest();

    case 1: // POST /messages
        return onPostMessagesRequest();

    case 2: // POST /keys
        // when client requests for a new key,
        // we request server for one, and respond to client with the result from server
        return onPostKeysRequest();

    default:
        break;
    }
    return -1;
}

int8_t onPostDoorResponse() {
    Serial.println(P("onPostDoorResponse"));
    uint16_t status = gs.clientRequest.status_code;

    switch (status) {
    case 200:
        keys.setKeyValid(true);
        // save only independent area, since global.buffer might be populated by IR or so.
        keys.save2();
        startNormalOperation();
        break;
    case 401:
    case HTTP_STATUSCODE_CLIENT_TIMEOUT:
        // keys have expired, we have to start from morse sequence again
        keys.clear();
        break;
    case 408:
    case 503: // heroku responds with 503 if longer than 30sec
    default:
        // try again
        postDoor();
        break;
    }

    return 0;
}

int8_t onGetMessagesResponse() {
    uint16_t status = gs.clientRequest.status_code;

    Serial.print(P("onGetMessagesResponse ")); Serial.println(status);

    switch (status) {
    case 200:
        while (!ring_isempty(gs._buf_cmd)) {
            char letter;
            ring_get(gs._buf_cmd, &letter, 1);

            irjson_parse( letter,
                          &jsonDetectedStart,
                          &jsonDetectedData,
                          &jsonDetectedEnd );
        }

        if (gs.clientRequest.state == GSwifi::GSRESPONSESTATE_RECEIVED) {
            // should not be WRITING here, should be XMITTING or IDLE (xmit finished)
            if (IrCtrl.state == IR_WRITING) {
                // prevent from locking in WRITING state forever
                IR_state( IR_IDLE );
            }

            TIMER_START(message_timer, 0);
        }
        break;
    case HTTP_STATUSCODE_CLIENT_TIMEOUT:
        TIMER_START(message_timer, 5);
        break;
    case 503: // heroku responds with 503 if longer than 30sec
    default:
        if (gs.clientRequest.state == GSwifi::GSRESPONSESTATE_RECEIVED) {
            TIMER_START(message_timer, 5);
        }
        break;
    }

    return 0;
}

int8_t onPostKeysResponse() {
    uint16_t status = gs.clientRequest.status_code;

    Serial.print(P("onPostKeysResponse ")); Serial.println(status);

    if (gs.clientRequest.state != GSwifi::GSRESPONSESTATE_RECEIVED) {
        return 0;
    }

    if ( (gs.serverRequest.cid == CID_UNDEFINED) ||
         (gs.serverRequest.cid != post_keys_cid) ) {
        return 0;
    }

    gs.writeHead( status );

    switch (status) {
    case 200:
        while (!ring_isempty(gs._buf_cmd)) {
            char letter;
            ring_get(gs._buf_cmd, &letter, 1);
            gs.write( letter );
        }
        gs.end();
        break;
    default:
        gs.end();
        break;
    }

    return 0;
}

void postDoor() {
    char body[41]; // 4 + 36 + 1
    sprintf(body, "key=%s", keys.getKey());
    gs.post( PB("/door",1), body, 40, &onPostDoorResponse, 50 );
}

int8_t getMessages() {
    // /messages?key=5bd38a24-77e3-46ea-954f-571071055dac&newer_than=%s
    char path[80];
    sprintf(path, P("/messages?key=%s&newer_than=%ld"), keys.getKey(), newest_message_id);
    return gs.get(path, &onGetMessagesResponse, 50);
}

void postKeys() {
    char body[41]; // 4 + 36 + 1
    sprintf(body, "key=%s", keys.getKey());
    gs.post( PB("/keys",1), body, 40, &onPostKeysResponse, 10 );
}

void connect() {
    global.buffer_mode = GBufferModeWifiCredentials;
    IR_state( IR_DISABLED );

    // load wifi credentials from EEPROM
    keys.load();

    if (keys.isWifiCredentialsSet()) {
        color.setLedColor( 1, 1, 0, true ); // yellow blink if we have valid keys

        gs.join(keys.getSecurity(),
                keys.getSSID(),
                keys.getPassword());
    }

    if (gs.isJoined()) {
        keys.setWifiWasValid(true);
        keys.save();

        color.setLedColor( 0, 1, 0, true );

        // 0
        gs.registerRoute( GSwifi::GSMETHOD_GET,  P("/messages") );
        // 1
        gs.registerRoute( GSwifi::GSMETHOD_POST, P("/messages") );
        // 2
        gs.registerRoute( GSwifi::GSMETHOD_POST, P("/keys") );

        gs.setRequestHandler( &onRequest );

        // start http server
        gs.listen(80);

        // start mDNS
        gs.setupMDNS();
    }
    else {
        Serial.println(P("!!!join failed"));
        keys.dump();

        if (keys.wasWifiValid()) {
            // retry
            color.setLedColor( 1, 0, 0, false );
            TIMER_START(reconnect_timer, 5);
        }
        else {
            keys.clear();
            color.setLedColor( 1, 0, 0, true );
            listener.enable(true);
        }
    }

    if (gs.isListening()) {
        color.setLedColor( 0, 1, 0, false );

        if (keys.isAPIKeySet() && ! keys.isValid()) {
            postDoor();
        }
        else if (keys.isValid()) {
            startNormalOperation();
        }
    }
}

void startNormalOperation() {
    Serial.println(P("startNormal"));

    TIMER_START(message_timer, 0);

    global.buffer_mode = GBufferModeIR;

    IR_state( IR_IDLE );
}

void letterCallback( char letter ) {
    Serial.print(P("letter: ")); Serial.write(letter); Serial.println();

    if (morse_error) {
        return;
    }

    uint8_t result = keys.put( letter );
    if (result != 0) {
        // postpone til this "word" ends
        morse_error = true;
    }
}

void wordCallback() {
    Serial.println(P("word"));

    if (morse_error) {
        morse_error = false;
        keys.clear();
        return;
    }

    int8_t result = keys.putDone();
    if ( result != 0 ) {
        keys.clear();
        Serial.println(P("cleared"));
    }
    else {
        keys.dump();
        keys.save();
        connect();
    }
}

void IRKit_setup() {
    //--- initialize LED

    FlexiTimer2::set( TIMER_INTERVAL, &onTimer );
    FlexiTimer2::start();
    color.setLedColor( 1, 0, 0, false );

    //--- initialize long press button

    clear_button.callback = &longPressed;

    //--- initialize morse listener

    pinMode(MICROPHONE,  INPUT);

    listener.letterCallback = &letterCallback;
    listener.wordCallback   = &wordCallback;
    listener.setup();

    //--- initialize IR

    pinMode(IR_OUT,           OUTPUT);

    // pull-up
    pinMode(IR_IN,            INPUT);
    digitalWrite(IR_IN,       HIGH);

    IR_initialize();

    //--- initialize Wifi

    reset3V3();

    gs.setup( &onDisconnect, &onReset );

    connect();
}

void IRKit_loop() {
    global.loop(); // always run first

    listener.loop();

    timerLoop();

    clear_button.loop();

    // wifi
    gs.loop();

    // Wifi UART interface test
    if (Serial.available()) {
        static uint8_t last_character = '0';
        last_character = Serial.read();

        Serial.write(last_character);
        Serial.println();
        Serial.print(P("free memory: 0x")); Serial.println( freeMemory(), HEX );

        if (last_character == 'd') {
            keys.load();

            Serial.print("buffer_mode: "); Serial.println(global.buffer_mode);

            Serial.println(P("---keys---"));
            keys.dump();
            Serial.println();

            Serial.println(P("---wifi---"));
            // gs.dump();
            Serial.println();

            Serial.println(P("---ir---"));
            IR_dump();
            Serial.println();
        }
        else if (last_character == 'r') {
            Serial.print(P("reset"));
            reset3V3();
        }
        else if (last_character == 's') {
            Serial.println(P("setting keys in EEPROM"));
            keys.set(GSSECURITY_WPA2_PSK,
                     PB("Rhodos",1),
                     PB("aaaaaaaaaaaaa",2));
            keys.setKey(P("5bd38a24-77e3-46ea-954f-571071055dac"));
            keys.save();
        }
        else if (last_character == 'v') {
            Serial.print(P("version: "));
            Serial.println(version);
        }
    }
}
