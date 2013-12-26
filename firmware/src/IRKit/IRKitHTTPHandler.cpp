#include "Arduino.h"
#include "pgmStrToRAM.h"
#include "IRKitHTTPHandler.h"
#include "Keys.h"
#include "IRKitJSONParser.h"
#include "IrCtrl.h"
#include "timer.h"
#include "commands.h"

extern GSwifi gs;
extern Keys keys;
extern struct RingBuffer commands;
extern void on_ir_xmit();
extern volatile char sharedbuffer[];

// if we have recently received GET or POST /messages request,
// delay our next request to API server for a while
// to avoid concurrently processing receiving requests and requesting.
// if user can access us via local wifi, no requests arrive at our server anyway
// (except another person's trying to send from outside, at the same time)
static volatile uint8_t suspend_polling_timer = TIMER_OFF;
static volatile uint8_t polling_timer         = TIMER_OFF;

static uint32_t newest_message_id = 0; // on memory only should be fine
static uint8_t post_keys_cid;

#define POST_DOOR_BODY_LENGTH 61
#define POST_KEYS_BODY_LENGTH 42

// simple, specialized, JSON like string parser
// well, I'm fighting with 100bytes of program memory

static void on_json_start() {
    Serial.println("json<<");

    IR_state( IR_WRITING );
}

static void on_json_data( uint8_t key, uint32_t value ) {
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

static void on_json_end() {
    Serial.println(">>json");

    if ( IrCtrl.state != IR_WRITING ) {
        Serial.println("!E5");
        IR_dump();
        return;
    }

    IR_xmit();
    on_ir_xmit();
}

static void parse_json( char letter ) {
    irkit_json_parse( letter,
                      &on_json_start,
                      &on_json_data,
                      &on_json_end );
}

static int8_t on_post_door_response(uint8_t cid, uint16_t status_code, GSwifi::GSREQUESTSTATE state) {
    Serial.print(P("P /d RS ")); Serial.println(status_code);

    gs.bufferClear();

    if (state != GSwifi::GSREQUESTSTATE_RECEIVED) {
        return 0;
    }

    switch (status_code) {
    case 200:
        keys.setKeyValid(true);
        // save only independent area, since sharedbuffer might be populated by IR or so.
        keys.save2();
        IR_state( IR_IDLE );

        ring_put( &commands, COMMAND_CLOSE );
        ring_put( &commands, cid );
        ring_put( &commands, COMMAND_START_POLLING );

        break;
    case 401:
    case HTTP_STATUSCODE_CLIENT_TIMEOUT:
        // keys have expired, we have to start from morse sequence again
        gs.close(cid);
        keys.clear();
        keys.save();
        break;
    case 408:
    case 503: // heroku responds with 503 if longer than 30sec
    default:
        // try again
        gs.close(cid);
        irkit_httpclient_post_door();
        break;
    }

    return 0;
}

static int8_t on_get_messages_response(uint8_t cid, uint16_t status_code, GSwifi::GSREQUESTSTATE state) {
    Serial.print(P("G /m RS ")); Serial.println(status_code);

    if (status_code != 200) {
        gs.bufferClear();
    }

    switch (status_code) {
    case 200:
        while (! gs.bufferEmpty()) {
            char letter = gs.bufferGet();

            parse_json( letter );
        }

        if (state == GSwifi::GSREQUESTSTATE_RECEIVED) {
            // should not be WRITING here, should be XMITTING or IDLE (xmit finished)
            if (IrCtrl.state == IR_WRITING) {
                // prevent from locking in WRITING state forever
                IR_state( IR_IDLE );
            }

            ring_put( &commands, COMMAND_CLOSE );
            ring_put( &commands, cid );
            ring_put( &commands, COMMAND_START_POLLING );
        }
        break;
    case HTTP_STATUSCODE_CLIENT_TIMEOUT:
        gs.close(cid);
        irkit_httpclient_start_polling( 5 );
        break;
    // heroku responds with 503 if longer than 30sec,
    // or when deploy occurs
    case 503:
    default:
        if (state == GSwifi::GSREQUESTSTATE_RECEIVED) {
            gs.close(cid);
            irkit_httpclient_start_polling( 5 );
        }
        break;
    }

    return 0;
}

static int8_t on_post_keys_response(uint8_t cid, uint16_t status_code, GSwifi::GSREQUESTSTATE state) {
    Serial.print(P("P /k RS ")); Serial.println(status_code);

    if (status_code != 200) {
        gs.bufferClear();
    }

    if (state != GSwifi::GSREQUESTSTATE_RECEIVED) {
        return 0;
    }

    gs.writeHead( post_keys_cid, status_code );

    switch (status_code) {
    case 200:
        while (! gs.bufferEmpty()) {
            char letter = gs.bufferGet();
            gs.write( letter );
        }
        gs.writeEnd();
        break;
    default:
        gs.writeEnd();
        break;
    }

    ring_put( &commands, COMMAND_CLOSE );
    ring_put( &commands, cid );
    ring_put( &commands, COMMAND_CLOSE );
    if (ring_isfull( &commands )) {
        Serial.println(("!E8"));
        return -1;
    }
    ring_put( &commands, post_keys_cid );

    return 0;
}

static int8_t on_post_messages_response(uint8_t cid, uint16_t status_code, GSwifi::GSREQUESTSTATE state) {
    Serial.print(P("P /m RS ")); Serial.println(status_code);

    if (status_code != 200) {
        gs.bufferClear();
    }

    if (state != GSwifi::GSREQUESTSTATE_RECEIVED) {
        return 0;
    }

    ring_put( &commands, COMMAND_CLOSE );
    ring_put( &commands, cid );

    return 0;
}

static int8_t on_get_messages_request(uint8_t cid, GSwifi::GSREQUESTSTATE state) {
    if (state != GSwifi::GSREQUESTSTATE_RECEIVED) {
        return -1;
    }

    gs.writeHead(cid, 200);

    if ( (IrCtrl.len <= 0) ||
         (IrCtrl.state != IR_RECVED_IDLE) ) {
        // if no data
        gs.writeEnd();
        ring_put( &commands, COMMAND_CLOSE );
        ring_put( &commands, cid );
        return 0;
    }

    IR_state( IR_READING );

    gs.write(P("{\"format\":\"raw\",\"freq\":")); // format fixed to "raw" for now
    gs.write(IrCtrl.freq);
    gs.write(P(",\"data\":["));
    for (uint16_t i=0; i<IrCtrl.len; i++) {
        gs.write( IR_get() );
        if (i != IrCtrl.len - 1) {
            gs.write(",");
        }
    }
    gs.write("]}");
    gs.writeEnd();
    ring_put( &commands, COMMAND_CLOSE );
    ring_put( &commands, cid );

    IR_state( IR_IDLE );

    TIMER_START( suspend_polling_timer, SUSPEND_GET_MESSAGES_INTERVAL );

    return 0;
}

static int8_t on_post_messages_request(uint8_t cid, GSwifi::GSREQUESTSTATE state) {
    while (! gs.bufferEmpty()) {
        char letter = gs.bufferGet();
        parse_json( letter );
    }

    if (state == GSwifi::GSREQUESTSTATE_RECEIVED) {
        // should be xmitting or idle (xmit finished)
        if (IrCtrl.state == IR_WRITING) {
            Serial.println(("!E7"));
            // invalid json
            gs.writeHead(cid, 400);
            gs.writeEnd();
        }
        else {
            gs.writeHead(cid, 200);
            gs.writeEnd();
        }
        ring_put( &commands, COMMAND_CLOSE );
        ring_put( &commands, cid );

        TIMER_START( suspend_polling_timer, SUSPEND_GET_MESSAGES_INTERVAL );
    }

    return 0;
}

static int8_t on_post_keys_request(uint8_t cid, GSwifi::GSREQUESTSTATE state) {
    if (state == GSwifi::GSREQUESTSTATE_RECEIVED) {
        // don't close other client requests, we can handle multiple concurrent client requests
        // and "close" and it's response mixing up makes things difficult

        // respond to this cid, when we get a new key
        post_keys_cid = cid;

        // delay execution to next tick (we get clean stack)
        // POST /keys to server
        ring_put( &commands, COMMAND_POST_KEYS );
    }
}

static int8_t on_request(uint8_t cid, int8_t routeid, GSwifi::GSREQUESTSTATE state) {
    switch (routeid) {
    case 0: // POST /messages
        return on_post_messages_request(cid, state);

    case 1: // POST /keys
        // when client requests for a new key,
        // we request server for one, and respond to client with the result from server
        return on_post_keys_request(cid, state);

    case 2: // GET /messages
        return on_get_messages_request(cid, state);

    default:
        break;
    }
    return -1;
}

int8_t irkit_httpclient_post_door() {
    // devicekey=[0-9A-F]{32}&hostname=IRKit%%%%
    char body[POST_DOOR_BODY_LENGTH+1];
    sprintf(body, "devicekey=%s&hostname=%s", keys.getKey(), gs.hostname());
    return gs.post( "/d", body, POST_DOOR_BODY_LENGTH, &on_post_door_response, 50 );
}

int8_t irkit_httpclient_get_messages() {
    // /m?devicekey=C7363FDA0F06406AB11C29BA41272AE3&newer_than=4294967295
    char path[70];
    sprintf(path, P("/m?devicekey=%s&newer_than=%ld"), keys.getKey(), newest_message_id);
    return gs.get(path, &on_get_messages_response, 50);
}

int8_t irkit_httpclient_post_messages() {
    // post body is IR data, move devicekey parameter to query, for implementation simplicity
    // /p?devicekey=C7363FDA0F06406AB11C29BA41272AE3&freq=38
    char path[54];
    sprintf(path, P("/p?devicekey=%s&freq=%d"), keys.getKey(), IrCtrl.freq);
    return gs.postBinary( path,
                          (const char*)sharedbuffer, IR_packedlength(),
                          &on_post_messages_response,
                           10 );
}

int8_t irkit_httpclient_post_keys() {
    // devicekey=[0-9A-F]{32}
    char body[POST_KEYS_BODY_LENGTH+1];
    sprintf(body, "devicekey=%s", keys.getKey());
    int8_t result = gs.post( "/k",
                             body, POST_KEYS_BODY_LENGTH,
                             &on_post_keys_response,
                             10 );
    if ( result < 0 ) {
        gs.writeHead( post_keys_cid, 500 );
        gs.writeEnd();
        gs.close( post_keys_cid );
    }
}

void irkit_httpclient_start_polling(uint8_t delay) {
    TIMER_START(polling_timer, delay);
}

void irkit_httpserver_register_handler() {
    // 0
    gs.registerRoute( GSwifi::GSMETHOD_POST, P("/messages") );
    // 1
    gs.registerRoute( GSwifi::GSMETHOD_POST, P("/keys") );
    // 2
    gs.registerRoute( GSwifi::GSMETHOD_GET, P("/messages") );

    gs.setRequestHandler( &on_request );
}

void irkit_http_on_timer() {
    TIMER_TICK(polling_timer);

    TIMER_TICK(suspend_polling_timer);
}

void irkit_http_loop() {
    // long poll
    if (TIMER_FIRED(polling_timer)) {
        TIMER_STOP(polling_timer);
        Serial.println("message timeout");

        if (TIMER_RUNNING(suspend_polling_timer)) {
            // suspend GET /m for a while if we have received a POST /messages request from client
            // client is in wifi, we can ignore our server for a while
            TIMER_START(polling_timer, SUSPEND_GET_MESSAGES_INTERVAL);
        }
        else {
            int8_t result = irkit_httpclient_get_messages();
            if ( result != 0 ) {
                Serial.println(("!E3"));
                // maybe time cures GS?
                TIMER_START(polling_timer, 5);
            }
        }
    }

    if (TIMER_FIRED(suspend_polling_timer)) {
        TIMER_STOP(suspend_polling_timer);
        Serial.println("suspend timeout");
    }
}
