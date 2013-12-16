#include "Arduino.h"
#include "pgmStrToRAM.h"
#include "IRKitHTTPClient.h"
#include "Keys.h"
#include "CommandQueue.h"
#include "IRKitJSONParser.h"
#include "IrCtrl.h"
#include "timer.h"

static IRKitJSONParser parser;

extern GSwifi gs;
extern Keys keys;
extern uint32_t newest_message_id;
extern CommandQueue commands;
extern IRKitHTTPClient client;
extern volatile uint8_t recently_posted_timer;
uint8_t post_keys_cid;

// if we have recently received POST /messages request,
// delay our next request to API server for a while
// to avoid concurrently processing receiving requests and requesting.
// if user can access us via local wifi, no requests arrive at our server anyway
// (except another person's trying to send from outside, at the same time)
volatile uint8_t recently_posted_timer = TIMER_OFF;

uint32_t newest_message_id = 0; // on memory only should be fine

#define POST_DOOR_BODY_LENGTH 61
#define POST_KEYS_BODY_LENGTH 42

int8_t onPostDoorResponse(uint8_t cid, uint16_t status_code, GSwifi::GSREQUESTSTATE state) {
    Serial.print(P("P /d RS ")); Serial.println(status_code);

    ring_clear(gs._buf_cmd);

    if (state != GSwifi::GSREQUESTSTATE_RECEIVED) {
        return 0;
    }

    switch (status_code) {
    case 200:
        keys.setKeyValid(true);
        // save only independent area, since global.buffer might be populated by IR or so.
        keys.save2();

        commands.put( COMMAND_CLOSE );
        commands.put( cid );
        commands.put( COMMAND_START );

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
        client.postDoor();
        break;
    }

    return 0;
}

int8_t onGetMessagesResponse(uint8_t cid, uint16_t status_code, GSwifi::GSREQUESTSTATE state) {
    Serial.print(P("G /m RS ")); Serial.println(status_code);

    if (status_code != 200) {
        ring_clear(gs._buf_cmd);
    }

    switch (status_code) {
    case 200:
        while (!ring_isempty(gs._buf_cmd)) {
            char letter;
            ring_get(gs._buf_cmd, &letter, 1);

            parser.parse( letter );
        }

        if (state == GSwifi::GSREQUESTSTATE_RECEIVED) {
            // should not be WRITING here, should be XMITTING or IDLE (xmit finished)
            if (IrCtrl.state == IR_WRITING) {
                // prevent from locking in WRITING state forever
                IR_state( IR_IDLE );
            }

            commands.put( COMMAND_CLOSE );
            commands.put( cid );
            commands.put( COMMAND_START );
        }
        break;
    case HTTP_STATUSCODE_CLIENT_TIMEOUT:
        gs.close(cid);
        client.startTimer(5);
        break;
    // heroku responds with 503 if longer than 30sec,
    // or when deploy occurs
    case 503:
    default:
        if (state == GSwifi::GSREQUESTSTATE_RECEIVED) {
            gs.close(cid);
            client.startTimer(5);
        }
        break;
    }

    return 0;
}

int8_t onPostKeysResponse(uint8_t cid, uint16_t status_code, GSwifi::GSREQUESTSTATE state) {
    Serial.print(P("P /k RS ")); Serial.println(status_code);

    if (status_code != 200) {
        ring_clear(gs._buf_cmd);
    }

    if (state != GSwifi::GSREQUESTSTATE_RECEIVED) {
        return 0;
    }

    gs.writeHead( post_keys_cid, status_code );

    switch (status_code) {
    case 200:
        while (!ring_isempty(gs._buf_cmd)) {
            char letter;
            ring_get(gs._buf_cmd, &letter, 1);
            gs.write( letter );
        }
        gs.writeEnd();
        break;
    default:
        gs.writeEnd();
        break;
    }

    commands.put( COMMAND_CLOSE );
    commands.put( cid );
    commands.put( COMMAND_CLOSE );
    if (commands.is_full()) {
        Serial.println(("!E8"));
        return -1;
    }
    commands.put( post_keys_cid );

    return 0;
}

int8_t onPostMessagesResponse(uint8_t cid, uint16_t status_code, GSwifi::GSREQUESTSTATE state) {
    Serial.print(P("P /m RS ")); Serial.println(status_code);

    if (status_code != 200) {
        ring_clear(gs._buf_cmd);
    }

    if (state != GSwifi::GSREQUESTSTATE_RECEIVED) {
        return 0;
    }

    commands.put( COMMAND_CLOSE );
    commands.put( cid );

    return 0;
}


int8_t onPostMessagesRequest(uint8_t cid, GSwifi::GSREQUESTSTATE state) {
    while (!ring_isempty(gs._buf_cmd)) {
        char letter;
        ring_get(gs._buf_cmd, &letter, 1);

        parser.parse( letter );
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
        commands.put( COMMAND_CLOSE );
        commands.put( cid );

        TIMER_START( recently_posted_timer, SUSPEND_GET_MESSAGES_INTERVAL );
    }

    return 0;
}

int8_t onPostKeysRequest(uint8_t cid, GSwifi::GSREQUESTSTATE state) {
    if (state == GSwifi::GSREQUESTSTATE_RECEIVED) {
        // don't close other client requests, we can handle multiple concurrent client requests
        // and "close" and it's response mixing up makes things difficult

        // respond to this cid, when we get a new key
        post_keys_cid = cid;

        // delay execution to next tick (we get clean stack)
        // POST /keys to server
        commands.put( COMMAND_POST_KEYS );
    }
}

int8_t onRequest(uint8_t cid, int8_t routeid, GSwifi::GSREQUESTSTATE state) {
    switch (routeid) {
    case 0: // POST /messages
        return onPostMessagesRequest(cid, state);

    case 1: // POST /keys
        // when client requests for a new key,
        // we request server for one, and respond to client with the result from server
        return onPostKeysRequest(cid, state);

    default:
        break;
    }
    return -1;
}

IRKitHTTPClient::IRKitHTTPClient(GSwifi *gs) :
    gs_(gs),
    timer_(TIMER_OFF)
{
}

int8_t IRKitHTTPClient::postDoor() {
    // devicekey=[0-9A-F]{32}&hostname=IRKit%%%%
    char body[POST_DOOR_BODY_LENGTH+1];
    sprintf(body, "devicekey=%s&hostname=%s", keys.getKey(), gs_->hostname());
    return gs_->post( "/d", body, POST_DOOR_BODY_LENGTH, &onPostDoorResponse, 50 );
}

int8_t IRKitHTTPClient::getMessages() {
    // /m?devicekey=C7363FDA0F06406AB11C29BA41272AE3&newer_than=4294967295
    char path[70];
    sprintf(path, P("/m?devicekey=%s&newer_than=%ld"), keys.getKey(), newest_message_id);
    return gs_->get(path, &onGetMessagesResponse, 50);
}

int8_t IRKitHTTPClient::postMessages() {
    // post body is IR data, move devicekey parameter to query, for implementation simplicity
    // /p?devicekey=C7363FDA0F06406AB11C29BA41272AE3&freq=38
    char path[54];
    sprintf(path, P("/p?devicekey=%s&freq=%d"), keys.getKey(), IrCtrl.freq);
    return gs_->postBinary( path,
                          (const char*)global.buffer, IR_packedlength(),
                          &onPostMessagesResponse,
                          10 );
}

int8_t IRKitHTTPClient::postKeys() {
    // devicekey=[0-9A-F]{32}
    char body[POST_KEYS_BODY_LENGTH+1];
    sprintf(body, "devicekey=%s", keys.getKey());
    int8_t result = gs_->post( "/k",
                               body, POST_KEYS_BODY_LENGTH,
                               &onPostKeysResponse,
                               10 );
    if ( result < 0 ) {
        gs.writeHead( post_keys_cid, 500 );
        gs.writeEnd();
        gs.close( post_keys_cid );
    }
}

void IRKitHTTPClient::registerRequestHandler() {
    // 0
    gs_->registerRoute( GSwifi::GSMETHOD_POST, P("/messages") );
    // 1
    gs_->registerRoute( GSwifi::GSMETHOD_POST, P("/keys") );

    gs_->setRequestHandler( &onRequest );
}

void IRKitHTTPClient::startTimer(uint8_t time) {
    TIMER_START(timer_, time);
}

void IRKitHTTPClient::onTimer() {
    TIMER_TICK(timer_);

    TIMER_TICK( recently_posted_timer );
}

void IRKitHTTPClient::loop() {
    // long poll
    if (TIMER_FIRED(timer_)) {
        TIMER_STOP(timer_);
        Serial.println("message timeout");

        if (TIMER_RUNNING(recently_posted_timer)) {
            // suspend GET /m for a while if we have received a POST /messages request from client
            // client is in wifi, we can ignore our server for a while
            TIMER_START(timer_, SUSPEND_GET_MESSAGES_INTERVAL);
        }
        else {
            int8_t result = getMessages();
            if ( result != 0 ) {
                Serial.println(("!E3"));
                // maybe time cures GS?
                TIMER_START(timer_, 5);
            }
        }
    }

    if (TIMER_FIRED(recently_posted_timer)) {
        TIMER_STOP(recently_posted_timer);
        Serial.println("recently timeout");
    }
}
