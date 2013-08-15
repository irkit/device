#include "pins.h"
#include "BLE112.h"
#include "pgmStrToRAM.h"
#include "IrCtrl.h"
#include "MemoryFree.h"
#include "version.h"

// gatt.xml allows max: 255 for service.characteristic.value.length
// but BGLib I2C fails (waiting for attributes_write response timeouts) sending 255Bytes
// looks like 254Bytes is the longest length allowed
#define BLE112_MAX_CHARACTERISTIC_VALUE_LENGTH 254

// wait for this interval for ble112's response
// we have to send commands sequentially
#define BLE112_RESPONSE_TIMEOUT              100
#define BLE112_RESPONSE_TIMEOUT_AFTER_RESET 1000

// if RSSI is heigher than (including) this value,
// authenticate the connected handle
// at -38dBm the BLE112 receiver is saturated.
// by p81 of Bluetooth_Smart_API_11_11032013.pdf
// TODO adjust this when our production case is ready
#define NEAR_RSSI_MIN  -40

// TODO remove IrCtrl dependency here
extern BLE112 ble112;
extern volatile IR_STRUCT IrCtrl;
extern int IR_xmit();

// ================================================================
// INTERNAL BGLIB CLASS CALLBACK FUNCTIONS
// ================================================================

void onTimeout() {
    if (ble112.didTimeoutCallback) {
        ble112.didTimeoutCallback();
    }
}

// ================================================================
// USER-DEFINED BGLIB RESPONSE CALLBACKS
// ================================================================

void my_rsp_system_hello(const ble_msg_system_hello_rsp_t *msg) {
    Serial.println(P("<--\tsystem_hello"));
}

void my_rsp_gap_set_scan_parameters(const ble_msg_gap_set_scan_parameters_rsp_t *msg) {
    Serial.print(P("<--\tgap_set_scan_parameters: { "));
    Serial.print(P("result: ")); Serial.print((uint16_t)msg -> result, HEX);
    Serial.println(P(" }"));
}

void my_rsp_gap_discover(const ble_msg_gap_discover_rsp_t *msg) {
    Serial.print(P("<--\tgap_discover: { "));
    Serial.print(P("result: ")); Serial.print((uint16_t)msg -> result, HEX);
    Serial.println(P(" }"));
}

void my_rsp_gap_end_procedure(const ble_msg_gap_end_procedure_rsp_t *msg) {
    Serial.print(P("<--\tgap_end_procedure: { "));
    Serial.print(P("result: ")); Serial.print((uint16_t)msg -> result, HEX);
    Serial.println(P(" }"));
}

void my_rsp_gap_set_adv_data(const ble_msg_gap_set_adv_data_rsp_t *msg) {
    Serial.print(P("<--\tgap_set_adv_data: { "));
    Serial.print(P("result: ")); Serial.print((uint16_t)msg -> result, HEX);
    Serial.println(P(" }"));
}

void my_rsp_gap_set_mode(const ble_msg_gap_set_mode_rsp_t *msg) {
    Serial.print(P("<--\tgap_set_mode: { "));

    // 0x020C: Command Disallowed
    Serial.print(P("result: ")); Serial.print((uint16_t)msg -> result, HEX);

    Serial.println(P(" }"));
}

void my_rsp_gap_set_adv_parameters(const ble_msg_gap_set_adv_parameters_rsp_t *msg) {
    Serial.print(P("<--\tgap_set_parameters: { "));
    Serial.print(P("result: ")); Serial.print((uint16_t)msg -> result, HEX);
    Serial.println(P(" }"));
}

void my_rsp_attributes_read(const struct ble_msg_attributes_read_rsp_t * msg ) {
    Serial.print(P("<--\tattributes_read: { "));
    Serial.print(P("handle: "));   Serial.print((uint16_t)msg -> handle, HEX);
    Serial.print(P(", offset: ")); Serial.print((uint16_t)msg -> offset, HEX);

    // 407: Invalid Offset
    Serial.print(P(", result: "));
    for (uint8_t i = 0; i < msg -> value.len; i++) {
        if (msg -> value.data[i] < 16) Serial.write('0');
        Serial.print(msg -> value.data[i], HEX);
    }
    Serial.println(P(" }"));
}

void my_rsp_attributes_user_read_response(const struct ble_msg_attributes_user_read_response_rsp_t * msg ) {
    Serial.println(P("<--\tattributes_user_read_response: {}"));
}

void my_rsp_attributes_write(const ble_msg_attributes_write_rsp_t *msg) {
    Serial.print(P("<--\tattributes_write: { "));
    Serial.print(P("result: ")); Serial.print((uint16_t)msg -> result, HEX);
    Serial.println(P(" }"));
}

void my_rsp_attributes_user_write_response(const struct ble_msg_attributes_user_write_response_rsp_t * msg ) {
    Serial.println(P("<--\tattributes_user_write_response: {}"));
}

void my_rsp_connection_disconnect(const struct ble_msg_connection_disconnect_rsp_t *msg) {
    Serial.print(P("<--\tconnection_disconnect: { "));
    Serial.print(P("conn: "));     Serial.print((uint8)msg -> connection, HEX);
    Serial.print(P(", result: ")); Serial.print((uint8)msg -> result,     HEX);
    Serial.println(P(" }"));
}

// respond to ATTRIBUTE_HANDLE_AUTH_CONTROL_POINT write event
void my_rsp_connection_get_rssi(const ble_msg_connection_get_rssi_rsp_t *msg) {
    int8 rssi = msg->rssi;
    Serial.print(P("<--\tconnection_get_rssi: { "));
    Serial.print(P("conn: "));   Serial.print((uint8)msg -> connection, HEX);
    Serial.print(P(", rssi: ")); Serial.print(rssi);
    Serial.println(P(" }"));

    // rssi can be > 0 if not connected

    if ( (rssi < 0) && (rssi >= NEAR_RSSI_MIN) ) {
        // ok
        if (ble112.didAuthenticateCallback) {
            ble112.didAuthenticateCallback();
        }

        ble112.attributesUserReadResponseAuthenticated( 1 );
    }
    else {
        // too far
        ble112.attributesUserReadResponseAuthenticated( 0 );
    }
}

void my_rsp_sm_encrypt_start(const ble_msg_sm_encrypt_start_rsp_t *msg) {
    Serial.print(P("<--\tsm_encrypt_start: { "));
    Serial.print(P("handle: "));   Serial.print((uint8)msg -> handle, HEX);
    Serial.print(P(", result: ")); Serial.print((uint16)msg -> result, HEX);
    Serial.println(P(" }"));
}

void my_rsp_sm_get_bonds(const ble_msg_sm_get_bonds_rsp_t *msg) {
    Serial.print(P("<--\tsm_get_bonds: { "));
    // bonds: num of currently bonded devices
    Serial.print(P("bonds: "));   Serial.print((uint8)msg -> bonds, HEX);
    Serial.println(P(" }"));
}

void my_rsp_sm_set_bondable_mode(const ble_msg_sm_set_bondable_mode_rsp_t *msg) {
    Serial.println(P("<--\tsm_set_bondable_mode: {}"));
}

void my_rsp_sm_set_oob_data(const ble_msg_sm_set_oob_data_rsp_t *msg) {
    Serial.println(P("<--\tsm_set_oob_data: {}"));
}

void my_rsp_sm_set_parameters(const ble_msg_sm_set_parameters_rsp_t *msg) {
    Serial.println(P("<--\tsm_set_parameters: {}"));
}

void my_rsp_sm_delete_bonding(const ble_msg_sm_delete_bonding_rsp_t *msg) {
    Serial.print(P("<--\tsm_delete_bonding: { "));
    // 0x0180: Invalid Parameter - Command contained invalid parameter
    Serial.print(P("result: ")); Serial.print((uint16)msg -> result, HEX);
    Serial.println(P(" }"));
}


// ================================================================
// USER-DEFINED BGLIB EVENT CALLBACKS
// ================================================================

void my_evt_system_boot(const ble_msg_system_boot_evt_t *msg) {
    Serial.print(P("###\tsystem_boot: { "));
    Serial.print(P("major: ")); Serial.print(msg -> major, HEX);
    Serial.print(P(", minor: ")); Serial.print(msg -> minor, HEX);
    Serial.print(P(", patch: ")); Serial.print(msg -> patch, HEX);
    Serial.print(P(", build: ")); Serial.print(msg -> build, HEX);
    Serial.print(P(", ll_version: ")); Serial.print(msg -> ll_version, HEX);
    Serial.print(P(", protocol_version: ")); Serial.print(msg -> protocol_version, HEX);
    Serial.print(P(", hw: ")); Serial.print(msg -> hw, HEX);
    Serial.println(P(" }"));
}

void my_evt_gap_scan_response(const ble_msg_gap_scan_response_evt_t *msg) {
    Serial.print(P("###\tgap_scan_response: { "));
    Serial.print(P("rssi: ")); Serial.print(msg -> rssi);
    Serial.print(P(", packet_type: ")); Serial.print((uint8_t)msg -> packet_type, HEX);
    Serial.print(P(", sender: "));
    // this is a bd_addr data type, which is a 6-byte uint8_t array
    for (uint8_t i = 0; i < 6; i++) {
        if (msg -> sender.addr[i] < 16) Serial.write('0');
        Serial.print(msg -> sender.addr[i], HEX);
    }
    Serial.print(P(", address_type: ")); Serial.print(msg -> address_type, HEX);
    Serial.print(P(", bond: ")); Serial.print(msg -> bond, HEX);
    Serial.print(P(", data: "));
    // this is a uint8array data type, which is a length byte and a uint8_t* pointer
    for (uint8_t i = 0; i < msg -> data.len; i++) {
        if (msg -> data.data[i] < 16) Serial.write('0');
        Serial.print(msg -> data.data[i], HEX);
    }
    Serial.println(P(" }"));
}

void my_evt_connection_status(const ble_msg_connection_status_evt_t *msg) {
    Serial.print(P("###\tconnection_status: { "));
    Serial.print(P("conn: "));    Serial.print(msg -> connection, HEX);

    // flags
    // bit 0 connection_connected | This status flag tells the connection
    //                              exists to a remote device.
    // bit 1 connection_encrypted | This flag tells the connection is encrypted.
    // bit 2 connection_completed | Connection completed flag,
    //                              which is used to tell a new connection has been created.
    // bit 3 connection_parameters_change | This flag tells that connection parameters
    // have changed and. It is set when connection parameters have changed
    // due to a link layer operation.
    Serial.print(P(", flags: ")); Serial.print(msg -> flags, BIN);
    Serial.print(P(", address: "));
    // this is a bd_addr data type, which is a 6-byte uint8_t array
    for (uint8_t i = 0; i < 6; i++) {
        if (msg -> address.addr[i] < 16) Serial.write('0');
        Serial.print(msg -> address.addr[i], HEX);
    }
    // address_type
    // 0: public address
    // 1: random address
    Serial.print(P(", address_type: ")); Serial.print(msg -> address_type, HEX);
    // Current connection interval (units of 1.25ms)
    Serial.print(P(", intvl: "));        Serial.print(msg -> conn_interval, HEX);
    // Current supervision timeout (units of 10ms)
    Serial.print(P(", timeout: "));      Serial.print(msg -> timeout, HEX);
    // Slave latency (how many connection intervals the slave may skip)
    Serial.print(P(", latency: "));      Serial.print(msg -> latency, HEX);
    // bonding handle if there is stored bonding for this device
    // 0xff otherwise
    Serial.print(P(", bonding: "));      Serial.print(msg -> bonding, HEX);
    Serial.println(P(" }"));

    if (msg->bonding == INVALID_BOND_HANDLE) {
        // DON'T DO THIS, fails because of timing?
        // we'll encrypt after client read auth attribute
        // auto enrypt
        // this shows bonding dialog on iOS
        // ble112.encryptStart();
    }
    else if ( (msg->bonding != INVALID_BOND_HANDLE) &&
         (msg->flags | BGLIB_CONNECTION_ENCRYPTED) ) {
        ble112.current_bond_handle = msg->bonding;
    }

    if (ble112.didConnectCallback) {
        ble112.didConnectCallback();
    }
}

void my_evt_connection_disconnected(const ble_msg_connection_disconnected_evt_t *msg) {
    Serial.println( P("###\tdisconnected") );
    Serial.print(P("free:")); Serial.println( freeMemory() );

    ble112.current_bond_handle = INVALID_BOND_HANDLE;
    ble112.startAdvertising();

    if (ble112.didDisconnectCallback) {
        ble112.didDisconnectCallback();
    }
}

void my_evt_attributes_status(const ble_msg_attributes_status_evt_t *msg) {
    Serial.print( P("###\tattributes_status: { ") );
    Serial.print(P("handle: "));  Serial.print((uint16)msg -> handle, HEX);
    // flags:
    // 1: Notifications are enabled
    // 2: Indications are enabled
    Serial.print(P(", flags: ")); Serial.print((uint8)msg -> flags, HEX);
    Serial.println(P(" }"));
}

// central reads characteristic
void my_evt_attributes_user_read_request(const struct ble_msg_attributes_user_read_request_evt_t* msg) {
    Serial.print(P("free:")); Serial.println( freeMemory() );
    Serial.print( P("###\tattributes_user_read_request: { ") );
    Serial.print(P("conn: "));  Serial.print((uint8)msg -> connection, HEX);
    Serial.print(P(", att.handle: ")); Serial.print((uint16)msg -> handle, HEX);
    Serial.print(P(", offset: ")); Serial.print((uint16)msg -> offset, HEX);
    Serial.print(P(", maxsize: ")); Serial.print((uint8)msg -> maxsize, HEX);
    Serial.println(P(" }"));

    switch (msg->handle) {
    case ATTRIBUTE_HANDLE_IR_DATA:
        {
            bool authenticated = ble112.isAuthenticatedCallback(ble112.current_bond_handle);
            if ( ! authenticated ) {
                Serial.println(P("!!! unauthenticated read"));
                ble112.attributesUserReadResponseData( ATT_ERROR_UNAUTHENTICATED, // att_error
                                                       0, // value_len
                                                       NULL // value_data
                                                       );
                break;
            }
            if ( (IrCtrl.state != IR_RECVED_IDLE) &&
                 (IrCtrl.state != IR_READING) ) {
                // you can read data only when we're IR_RECVED_IDLE state
                Serial.print(P("!!! user_read_request unexpected state: ")); Serial.println(IrCtrl.state, HEX);
                ble112.attributesUserReadResponseData( ATT_ERROR_STATE, // att_error
                                                       0, // value_len
                                                       NULL // value_data
                                                       );
                break;
            }
            if ( IrCtrl.len * 2 < msg->offset ) {
                // range error
                Serial.print(P("!!! range error IrCtrl.len: ")); Serial.println(IrCtrl.len, HEX);
                ble112.attributesUserReadResponseData( ATT_ERROR_UNEXPECTED, // att_error
                                                       0, // value_len
                                                       NULL // value_data
                                                       );
                break;
            }
            if ( msg->offset == 0 ) {
                // 1st read
                IR_state( IR_READING );
            }
            uint8 value_len = msg->maxsize;
            bool is_last_slice = 0;
            if ( IrCtrl.len * 2 <= msg->offset + (uint16_t)value_len ) {
                // if IrCtrl.len * 2 == msg->offset, value_len is 0
                // last partial
                value_len = IrCtrl.len * 2 - msg->offset;
                is_last_slice = 1;
            }
            uint8* buffWithOffset = (uint8*)IrCtrl.buff + msg->offset;

            ble112.attributesUserReadResponseData( 0, // att_error
                                                   value_len, // value_len
                                                   buffWithOffset // value_data
                                                   );

            if ( is_last_slice && ble112.afterBTCallback ) {
                IR_state( IR_IDLE );
                ble112.afterBTCallback();
            }
        }
        break;
    case ATTRIBUTE_HANDLE_IR_CARRIER_FREQUENCY:
        {
            ble112.attributesUserReadResponseFrequency( IrCtrl.freq );
        }
        break;
    case ATTRIBUTE_HANDLE_AUTH_STATUS:
        {
            if (ble112.current_bond_handle == INVALID_BOND_HANDLE) {
                // we can't determine if we're authenticated or not
                // if we aren't bonded yet
                Serial.println(P("!!! not bonded yet !!!"));
                ble112.attributesUserReadResponseAuthenticated( 0 );
                ble112.next_command = NEXT_COMMAND_ID_ENCRYPT_START;
                return;
            }

            bool authenticated = ble112.isAuthenticatedCallback( ble112.current_bond_handle );
            if ( authenticated ) {
                ble112.attributesUserReadResponseAuthenticated( 1 );
                return;
            }
            // get RSSI, and if we're close enough,
            // we determine that we've successfully authenticated
            ble112.getRSSI();

            // next: my_rsp_connection_get_rssi
        }
        break;
    case ATTRIBUTE_HANDLE_SOFTWARE_VERSION:
        {
            ble112.attributesUserReadResponseSoftwareVersion();
        }
        break;
    default:
        Serial.println(P("unknown attribute !"));
        break;
    }
}

// central writes characteristic
void my_evt_attributes_value(const struct ble_msg_attributes_value_evt_t * msg ) {
    Serial.print( P("###\tattributes_value: { ") );
    Serial.print(P("conn: "));  Serial.print((uint8)msg -> connection, HEX);

    // 0: attributes_attribute_change_reason_write_request
    // 2: attributes_attribute_change_reason_write_request_user
    //    Value was written by remote end,
    //    stack is waiting for write response to be sent to other end.
    //    Use User Write Response to send response.
    Serial.print(P(", reason: ")); Serial.print((uint8)msg -> reason, HEX);
    Serial.print(P(", handle: ")); Serial.print((uint16)msg -> handle, HEX);
    Serial.print(P(", offset: ")); Serial.print((uint16)msg -> offset, HEX);
    Serial.print(P(", value: "));
    for (uint8_t i = 0; i < msg -> value.len; i++) {
        if (msg -> value.data[i] < 16) Serial.write('0');
        Serial.print(msg -> value.data[i], HEX);
    }
    Serial.println(P(" }"));
    Serial.print(P("free:"));
    Serial.println( freeMemory() );

    if (msg->reason != BGLIB_ATTRIBUTES_ATTRIBUTE_CHANGE_REASON_WRITE_REQUEST_USER) {
        Serial.println(P("!!! unexpected reason"));
        ble112.next_command = NEXT_COMMAND_ID_USER_WRITE_RESPONSE_ERROR_UNEXPECTED;
        return;
    }
    bool authenticated = ble112.isAuthenticatedCallback( ble112.current_bond_handle );
    if ( ! authenticated ) {
        // writes to characteristics other than auth control point
        // always require authz
        Serial.println(P("!!! unauthenticated write"));
        ble112.next_command = NEXT_COMMAND_ID_USER_WRITE_RESPONSE_ERROR_UNAUTHENTICATED;
        return;
    }
    if ( (IrCtrl.state != IR_IDLE) &&
         (IrCtrl.state != IR_RECVED_IDLE) &&
         (IrCtrl.state != IR_WRITING) ) {
        // must be idle or writing
        IR_state( IR_IDLE );
        Serial.print(P("!!! write_request not idle state: ")); Serial.println(IrCtrl.state, HEX);
        ble112.next_command = NEXT_COMMAND_ID_USER_WRITE_RESPONSE_ERROR_STATE;
        return;
    }

    uint8 *buffWithOffset;
    switch (msg->handle) {
    case ATTRIBUTE_HANDLE_IR_DATA:
        {
            Serial.print(P("free:")); Serial.println( freeMemory() );

            // valid offset and length?
            if (IR_BUFF_SIZE * 2 < msg->offset + msg->value.len) {
                // overflow
                IR_state( IR_IDLE );
                Serial.println(P("!!! write overflow"));
                ble112.next_command = NEXT_COMMAND_ID_USER_WRITE_RESPONSE_ERROR_UNEXPECTED;
                return;
            }
            // ready to fill IR data
            if (msg->offset == 0) {
                if (ble112.beforeIRCallback) {
                    ble112.beforeIRCallback();
                }
                IR_state( IR_WRITING );
            }

            buffWithOffset = (uint8*)IrCtrl.buff + msg->offset;
            memcpy( buffWithOffset, (const void*)(msg->value.data), msg->value.len );
            IrCtrl.len = (msg->offset + msg->value.len) / 2;
        }
        break;
    case ATTRIBUTE_HANDLE_IR_CARRIER_FREQUENCY:
        {
            uint8_t freq = (uint8_t)(msg->value.data[0]);
            IrCtrl.freq  = freq;
        }
        break;
    case ATTRIBUTE_HANDLE_IR_CONTROL_POINT:
        {
            if (msg->value.data[0] != 0) {
                Serial.println(P("!!! unknown ir control point value"));
                ble112.next_command = NEXT_COMMAND_ID_USER_WRITE_RESPONSE_ERROR_UNEXPECTED;
                return;
            }
            if ( IrCtrl.len == 0 ) {
                Serial.println(P("!!! invalid data"));
                ble112.next_command = NEXT_COMMAND_ID_USER_WRITE_RESPONSE_ERROR_UNEXPECTED;
                return;
            }

            IR_xmit();
            IR_dump();
            Serial.println(P("xmitting"));

            // delay response til xmit complete
            ble112.next_command = NEXT_COMMAND_ID_USER_WRITE_RESPONSE_SUCCESS_XMIT;
            return;
        }
        break;
    default:
        // not expected
        Serial.println(P("!!! write to unknown att"));
        ble112.next_command = NEXT_COMMAND_ID_USER_WRITE_RESPONSE_ERROR_UNEXPECTED;
        return;
    }

    ble112.next_command = NEXT_COMMAND_ID_USER_WRITE_RESPONSE_SUCCESS;
}

void my_evt_attclient_attribute_value(const struct ble_msg_attclient_attribute_value_evt_t *msg) {
    Serial.print( P("###\tattclient_attribute_value: { ") );
    Serial.print(P("conn: "));         Serial.print((uint8)msg -> connection, HEX);
    Serial.print(P(", atthandle: "));  Serial.print((uint16)msg -> atthandle, HEX);
    Serial.print(P(", type: "));       Serial.print((uint8)msg -> type, HEX);
    Serial.print(P(", value: "));
    for (uint8_t i = 0; i < msg -> value.len; i++) {
        if (msg -> value.data[i] < 16) Serial.write('0');
        Serial.print(msg -> value.data[i], HEX);
    }
    Serial.println(P(" }"));
}

void my_evt_attclient_indicated(const struct ble_msg_attclient_indicated_evt_t *msg) {
    Serial.print( P("###\tattclient_indicated: { ") );
    Serial.print(P("conn: "));   Serial.print((uint8)msg -> connection, HEX);
    Serial.print(P(", attrhandle: ")); Serial.print((uint16)msg -> attrhandle, HEX);
    Serial.println(P(" }"));
}

void my_evt_attclient_procedure_completed(const struct ble_msg_attclient_procedure_completed_evt_t *msg) {
    Serial.print( P("###\tattclient_procedure_completed: { ") );
    Serial.print(P("conn: "));   Serial.print((uint8)msg -> connection, HEX);
    Serial.print(P(", result: ")); Serial.print((uint16)msg -> result, HEX);
    Serial.print(P(", chrhandle: ")); Serial.print((uint16)msg -> chrhandle, HEX);
    Serial.println(P(" }"));
}

void my_evt_sm_bonding_fail(const struct ble_msg_sm_bonding_fail_evt_t *msg) {
    Serial.print( P("###\tsm_bonding_fail: { ") );
    Serial.print(P("handle: "));   Serial.print((uint8)msg -> handle, HEX);
    // Encryption status, describes error that occurred during bonding
    // 0x0185: Timeout
    //         Command or Procedure failed due to timeout
    // 0x0186: Not Connected
    //         Connection handle passed is to command is not a valid handle
    // 0x0301: Passkey Entry Failed
    //         The user input of passkey failed, for example, the user cancelled the operation
    // 0x0302: OOB Data is not available
    //         Out of Band data is not available for authentication
    // 0x0303: Authentication Requirements
    //         The pairing procedure cannot be performed as authentication requirements
    //         cannot be met due to IO capabilities of one or both devices
    // 0x0305: Pairing Not Supported
    //         Pairing is not supported by the device
    // 0x0308: Unspecified Reason
    //         Pairing failed due to an unspecified reason
    Serial.print(P(", result: ")); Serial.print((uint16)msg -> result, HEX);
    Serial.println(P(" }"));
}

void my_evt_sm_bond_status(const struct ble_msg_sm_bond_status_evt_t *msg) {
    Serial.print( P("###\tsm_bond_status: { ") );
    Serial.print(P("bond: "));   Serial.print((uint8)msg -> bond, HEX);
    Serial.print(P(", keysize: ")); Serial.print((uint8)msg -> keysize, HEX);
    Serial.print(P(", mitm: ")); Serial.print((uint8)msg -> mitm, HEX);
    Serial.print(P(", keys: ")); Serial.print((uint8)msg -> keys, HEX);
    Serial.println(P(" }"));
}

BLE112::BLE112(HardwareSerial *module, uint8_t reset_pin) :
    bglib_(module, 0, 1),
    next_command(0xFF),
    reset_pin_(reset_pin),
    isAuthenticatedCallback(0),
    didTimeoutCallback(0),
    didConnectCallback(0),
    didDisconnectCallback(0),
    beforeIRCallback(0),
    afterIRCallback(0),
    beforeBTCallback(0),
    afterBTCallback(0),
    current_bond_handle(INVALID_BOND_HANDLE)
{
    pinMode(reset_pin,      OUTPUT);
    digitalWrite(reset_pin, HIGH);
}

void BLE112::setup()
{
    bglib_.onTimeout = onTimeout;

    // set up BGLib response handlers (called almost immediately after sending commands)
    // (these are also technicaly optional)
    bglib_.ble_rsp_system_hello                   = my_rsp_system_hello;
    bglib_.ble_rsp_gap_set_scan_parameters        = my_rsp_gap_set_scan_parameters;
    bglib_.ble_rsp_gap_discover                   = my_rsp_gap_discover;
    bglib_.ble_rsp_gap_end_procedure              = my_rsp_gap_end_procedure;
    bglib_.ble_rsp_gap_set_adv_data               = my_rsp_gap_set_adv_data;
    bglib_.ble_rsp_gap_set_mode                   = my_rsp_gap_set_mode;
    bglib_.ble_rsp_gap_set_adv_parameters         = my_rsp_gap_set_adv_parameters;
    bglib_.ble_rsp_attributes_read                = my_rsp_attributes_read;
    bglib_.ble_rsp_attributes_user_read_response  = my_rsp_attributes_user_read_response;
    bglib_.ble_rsp_attributes_write               = my_rsp_attributes_write;
    bglib_.ble_rsp_attributes_user_write_response = my_rsp_attributes_user_write_response;
    bglib_.ble_rsp_connection_disconnect          = my_rsp_connection_disconnect;
    bglib_.ble_rsp_connection_get_rssi            = my_rsp_connection_get_rssi;
    bglib_.ble_rsp_sm_encrypt_start               = my_rsp_sm_encrypt_start;
    bglib_.ble_rsp_sm_get_bonds                   = my_rsp_sm_get_bonds;
    bglib_.ble_rsp_sm_set_bondable_mode           = my_rsp_sm_set_bondable_mode;
    bglib_.ble_rsp_sm_set_oob_data                = my_rsp_sm_set_oob_data;
    bglib_.ble_rsp_sm_set_parameters              = my_rsp_sm_set_parameters;
    bglib_.ble_rsp_sm_delete_bonding              = my_rsp_sm_delete_bonding;

    // set up BGLib event handlers (called at unknown times)
    bglib_.ble_evt_system_boot                    = my_evt_system_boot;
    bglib_.ble_evt_gap_scan_response              = my_evt_gap_scan_response;
    bglib_.ble_evt_connection_status              = my_evt_connection_status;
    bglib_.ble_evt_connection_disconnected        = my_evt_connection_disconnected;
    bglib_.ble_evt_attributes_status              = my_evt_attributes_status;
    bglib_.ble_evt_attributes_user_read_request   = my_evt_attributes_user_read_request;
    bglib_.ble_evt_attributes_value               = my_evt_attributes_value;
    bglib_.ble_evt_attclient_attribute_value      = my_evt_attclient_attribute_value;
    bglib_.ble_evt_attclient_indicated            = my_evt_attclient_indicated;
    bglib_.ble_evt_attclient_procedure_completed  = my_evt_attclient_procedure_completed;
    bglib_.ble_evt_sm_bonding_fail                = my_evt_sm_bonding_fail;
    bglib_.ble_evt_sm_bond_status                 = my_evt_sm_bond_status;
}

void BLE112::loop()
{
    bglib_.checkActivity();

    switch (next_command) {
    case NEXT_COMMAND_ID_ENCRYPT_START:
        next_command = NEXT_COMMAND_ID_EMPTY;
        encryptStart();
        break;
    case NEXT_COMMAND_ID_USER_WRITE_RESPONSE_SUCCESS:
        next_command = NEXT_COMMAND_ID_EMPTY;
        attributesUserWriteResponse( 0,   // conn_handle
                                     0 ); // att_error
        break;
    case NEXT_COMMAND_ID_USER_WRITE_RESPONSE_SUCCESS_XMIT:
        if (IrCtrl.state == IR_IDLE) {
            // if xmit finished, respond with success
            next_command = NEXT_COMMAND_ID_EMPTY;
            attributesUserWriteResponse( 0,   // conn_handle
                                         0 ); // att_error

            if (ble112.afterIRCallback) {
                ble112.afterIRCallback();
            }
        }
        else if ( IRDidXmitTimeout() ) {
            IR_state( IR_IDLE );
            Serial.println(P("!!!\tIR xmit timeout"));
            // have been xmitting for more than ** milliseconds,
            // might be something wrong, but delay our decision, respond with success
            next_command = NEXT_COMMAND_ID_EMPTY;
            attributesUserWriteResponse( 0,   // conn_handle
                                         0 ); // att_error
        }
        break;
    case NEXT_COMMAND_ID_USER_WRITE_RESPONSE_ERROR_UNAUTHENTICATED:
        next_command = NEXT_COMMAND_ID_EMPTY;
        attributesUserWriteResponse( 0,   // conn_handle
                                     ATT_ERROR_UNAUTHENTICATED ); // att_error
        break;
    case NEXT_COMMAND_ID_USER_WRITE_RESPONSE_ERROR_STATE:
        next_command = NEXT_COMMAND_ID_EMPTY;
        attributesUserWriteResponse( 0,   // conn_handle
                                     ATT_ERROR_STATE ); // att_error
        break;
    case NEXT_COMMAND_ID_USER_WRITE_RESPONSE_ERROR_UNEXPECTED:
        next_command = NEXT_COMMAND_ID_EMPTY;
        attributesUserWriteResponse( 0,   // conn_handle
                                     ATT_ERROR_UNEXPECTED ); // att_error
        break;
    case NEXT_COMMAND_ID_EMPTY:
        break;
    }
}

void BLE112::softwareReset()
{
    Serial.println(P("-->\tsystem_reset: { boot_in_dfu: 0 }"));
    bglib_.ble_cmd_system_reset(0);

    uint8_t status;
    while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT_AFTER_RESET)));
    // system_reset doesn't have a response, but this BGLib
    // implementation allows the system_boot event specially to
    // set the P("busy") flag to false for this particular case
}

void BLE112::hardwareReset()
{
    Serial.println(P("-->\thardware_reset: {}"));

    // CC2540 RESET_N low duration min: 1us
    digitalWrite(reset_pin_, LOW);
    delay( 10 ); // [ms]
    digitalWrite(reset_pin_, HIGH);

    bglib_.setBusy(true); // checkActivity will wait until system boot event occured

    uint8_t status;
    while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT_AFTER_RESET)));
}

void BLE112::hello()
{
    Serial.println(P("-->\tsystem_hello"));
    bglib_.ble_cmd_system_hello();

    uint8_t status;
    while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT)));
    // response should come back within milliseconds
}

void BLE112::startAdvertising()
{
    smSetBondableMode();
    smSetParameters();

    // adv_interval_min default: 0x200 = 320ms
    // adv_interval_max default: 0x200 = 320ms
    // adv_channels     default: ?
    // https://developer.apple.com/hardwaredrivers/BluetoothDesignGuidelines.pdf
    // To be discovered by the Apple product, the Bluetooth accessory should first use the recommended advertising interval of 20 ms for at least 30 seconds. If it is not discovered within the initial 30 seconds, the accessory may choose to save battery power and increase its advertising interval. Apple recommends using one of the following longer intervals to increase chances of discovery by the Apple product:
    // *  645 ms
    // *  768 ms
    // *  961 ms
    // * 1065 ms
    // * 1294 ms
    // 20ms -> 0x20
    gapSetAdvParameters( 0x20, 0x40, 0x07 );

    // TODO: set discoverable mode to limited,
    // and after 30sec, set it to general
    // limited: ad interval 250-500ms, only 30sec
    // general: ad interval 1.28-2.56s, forever
    // BGLIB_GAP_GENERAL_DISCOVERABLE
    gapSetMode( BGLIB_GAP_GENERAL_DISCOVERABLE, BGLIB_GAP_UNDIRECTED_CONNECTABLE );
}

// this is how to change advertisement data
// void BLE112::updateAdvData()
// {
//     // can't initialize uint8array directly....
//     uint8 data[27] = {
//         // length
//         0x18,
//         // AD Format: Flags
//         0x02, 0x01, 0x06,
//         // AD Format: 128bit Service UUID
//         0x11, 0x07, 0xE4, 0xBA, 0x94, 0xC3,
//                     0xC9, 0xB7, 0xCD, 0xB0,
//                     0x9B, 0x48, 0x7A, 0x43,
//                     0x8A, 0xE5, 0x5A, 0x19,
//         // AD Format: Manufacturer Specific Data
//         0x02, 0xFF, receivedCount_
//     };
//     setAdvData( 0, (uint8*)&data[0] );
// }

// void BLE112::setAdvData( uint8 set_scanrsp, uint8 *data )
// {
//     Serial.print(P("-->\tgap_set_adv_data: { "));
//     Serial.print(P("set_scanrsp: "));  Serial.print(set_scanrsp, HEX);
//     Serial.print(P(", data: "));
//     uint8array *array = (uint8array*)data;
//     for (uint8_t i = 0; i < array->len; i++) {
//         if (array->data[i] < 16)
//             Serial.write('0');
//         Serial.print(array->data[i], HEX);
//     }
//     Serial.println(P(" }"));

//     bglib_.ble_cmd_gap_set_adv_data( set_scanrsp, array->len, array->data );

//     uint8_t status;
//     while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT)));
// }

void BLE112::gapSetMode( uint8 discoverable, uint8 connectable )
{
    Serial.print(P("-->\tgap_set_mode: { "));
    Serial.print(P("discover: "));  Serial.print(discoverable, HEX);
    Serial.print(P(", connect: ")); Serial.print(connectable, HEX);
    Serial.println(P(" }"));

    bglib_.ble_cmd_gap_set_mode( discoverable, connectable );

    uint8_t status;
    while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT)));
}

void BLE112::gapSetAdvParameters( uint16 interval_min, uint16 interval_max, uint8 channels )
{
    Serial.print(P("-->\tgap_set_adv_parameters: { "));
    Serial.print(P("interval_min: "));  Serial.print(interval_min, HEX);
    Serial.print(P(", interval_max: ")); Serial.print(interval_max, HEX);
    Serial.print(P(", channels: ")); Serial.print(channels, HEX);
    Serial.println(P(" }"));

    bglib_.ble_cmd_gap_set_adv_parameters( interval_min, interval_max, channels );

    uint8_t status;
    while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT)));
}

void BLE112::getRSSI()
{
    Serial.println(P("-->\tconnection_get_rssi"));
    bglib_.ble_cmd_connection_get_rssi( 0x00 ); // connection handle

    uint8_t status;
    while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT)));
}

void BLE112::writeAttributeAuthStatus(bool authenticated)
{
    Serial.print(P("-->\tattributes_write auth status: "));
    Serial.println(authenticated, BIN);

    bglib_.ble_cmd_attributes_write( (uint16)ATTRIBUTE_HANDLE_AUTH_STATUS, // handle value
                                    0,                                     // offset
                                    1,                                     // value_len
                                    (const uint8*)&authenticated           // value_data
                                    );
    uint8_t status;
    while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT)));
}

void BLE112::writeAttributeUnreadStatus(bool unread)
{
    Serial.print(P("-->\tattributes_write unread status: "));
    Serial.println(unread, BIN);

    if (beforeBTCallback) {
        beforeBTCallback();
    }

    bglib_.ble_cmd_attributes_write( (uint16)ATTRIBUTE_HANDLE_IR_UNREAD_STATUS, // handle value
                                    0,                                       // offset
                                    1,                                       // value_len
                                    (const uint8*)&unread                    // value_data
                                    );
    uint8_t status;
    while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT)));
}

void BLE112::readAttribute()
{
    uint8_t status;
    // maximum payload: 22bytes
    // see p26 of Bluetooth_Smart_API_11_11032013.pdf
    uint8 totalSize = BLE112_MAX_CHARACTERISTIC_VALUE_LENGTH;
    // Serial.print(P("totalSize:")); Serial.println(totalSize);

    // read can deliver 22Bytes,
    // but let's use the same number with writes for simplicity
    for (uint8 i=0; i<(totalSize/20)+1; i++) {
        uint8 sendSize = totalSize - (i * 20);
        if (sendSize > 20) {
            sendSize = 20;
        }

        Serial.println(P("-->\tattributes_read"));
        /* Serial.print(P("sendSize:")); Serial.println(sendSize); */
        /* Serial.print(P("i:"));        Serial.println(i); */

        // handle value:
        // When the project is compiled with the BGBuild compiler
        // a text file called attributes.txt is generated.
        // This files contains the ids and corresponding handle values.
        bglib_.ble_cmd_attributes_read( (uint16)0x0011,       // handle value
                                        (uint8)(i*20) );      // offset
        while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT)));
    }
}

void BLE112::disconnect() {
    Serial.println(P("-->\tdisconnect"));
    bglib_.ble_cmd_connection_disconnect( (uint8)0 ); // connection handle

    uint8_t status;
    while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT)));
}

void BLE112::encryptStart()
{
    Serial.println(P("-->\tsm_encrypt_start"));
    bglib_.ble_cmd_sm_encrypt_start( (uint8)0, // connection handle
                                    (uint8)1  // create bonding if devices are not already bonded
                                    );
    uint8_t status;
    while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT)));
}

void BLE112::getBonds()
{
    Serial.println(P("-->\tsm_get_bonds"));
    bglib_.ble_cmd_sm_get_bonds();

    uint8_t status;
    while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT)));
}

void BLE112::smSetBondableMode()
{
    Serial.println(P("-->\tsm_set_bondable_mode"));
    bglib_.ble_cmd_sm_set_bondable_mode( 1 ); // this device is bondable

    uint8_t status;
    while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT)));
}

void BLE112::smSetParameters()
{
    Serial.println(P("-->\tsm_set_parameters"));
    // can't enable man-in-the-middle protection without having any keyboard nor display
    bglib_.ble_cmd_sm_set_parameters( (uint8)0, // man-in-the-middle protection NOT required
                                     (uint8)16, // minimum key size in bytes range 7-16
                                     (uint8)3   // SMP IO Capabilities (No input, No output)
                                     );

    uint8_t status;
    while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT)));
}

void BLE112::deleteBonding(uint8 connectionHandle)
{
    Serial.println(P("-->\tsm_delete_bonding"));
    bglib_.ble_cmd_sm_delete_bonding( connectionHandle );

    uint8_t status;
    while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT)));
}

void BLE112::attributesUserReadResponseData(uint8 att_error, uint8 value_len, uint8* value_data)
{
    Serial.print(P("-->\tattributes_user_read_response irdata: { "));
    for (uint8 i=0; i<value_len; i++) {
        if (value_data[i] < 16) Serial.write('0');
        Serial.print( value_data[i], HEX );
    }
    Serial.println(P(" }"));

    bglib_.ble_cmd_attributes_user_read_response( (uint8)0,   // connection handle
                                                 (uint8)att_error,   // att_error
                                                 (uint8)value_len,   // value_len,
                                                 (uint8*)value_data  // value_data
                                                 );
    uint8_t status;
    while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT)));
}

void BLE112::attributesUserReadResponseAuthenticated(bool authenticated)
{
    Serial.print(P("-->\tattributes_user_read_response authenticated: "));
    Serial.println(authenticated, BIN);

    bglib_.ble_cmd_attributes_user_read_response( (uint8)0,   // connection handle
                                                 (uint8)0,   // att_error
                                                 (uint8)1,   // value_len,
                                                 (uint8*)&authenticated // value_data
                                                 );
    uint8_t status;
    while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT)));
}

void BLE112::attributesUserReadResponseFrequency(uint16 freq)
{
    Serial.print(P("-->\tattributes_user_read_response freq: "));
    Serial.println(freq, HEX);

    bglib_.ble_cmd_attributes_user_read_response( (uint8)0,   // connection handle
                                                 (uint8)0,   // att_error
                                                 (uint8)2,   // value_len,
                                                 (uint8*)&freq // value_data
                                                 );
    uint8_t status;
    while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT)));
}

void BLE112::attributesUserReadResponseSoftwareVersion()
{
    Serial.print(P("-->\tattributes_user_read_response software version: "));
    Serial.print(P("version: ")); Serial.println(version);

    bglib_.ble_cmd_attributes_user_read_response( (uint8)0,   // connection handle
                                                 (uint8)0,   // att_error
                                                 (uint8)strlen(version), // value_len,
                                                 (uint8*)version // value_data
                                                 );
    uint8_t status;
    while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT)));
}

void BLE112::attributesUserWriteResponse( uint8 conn_handle, uint8 att_error )
{
    Serial.println(P("-->\tattributes_user_write_response"));
    bglib_.ble_cmd_attributes_user_write_response( conn_handle,
                                                  att_error
                                                  );

    uint8_t status;
    while ((status = bglib_.checkActivity(BLE112_RESPONSE_TIMEOUT)));
}
