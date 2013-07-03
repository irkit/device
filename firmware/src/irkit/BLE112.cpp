#include "Arduino.h"
#include "pins.h"
#include "BLE112.h"
#include "pgmStrToRAM.h"
#include "AuthSwitch.h"
#include "IrCtrl.h"
#include "MemoryFree.h"

// gatt.xml allows max: 255 for service.characteristic.value.length
// but BGLib I2C fails (waiting for attributes_write response timeouts) sending 255Bytes
// looks like 254Bytes is the longest length allowed
#define BLE112_MAX_CHARACTERISTIC_VALUE_LENGTH 254

// I don't like this but..
extern BLE112 ble112;
extern AuthSwitch auth;
extern volatile IR_STRUCT IrCtrl;
extern void IR_xmit();

const uint8_t data[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
    0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
    0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
    0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
    0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
    0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
    0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, /* gatt database size max: 255 */
};

// ================================================================
// INTERNAL BGLIB CLASS CALLBACK FUNCTIONS
// ================================================================

void onBusy() {
    // turn LED on when we're busy
    digitalWrite(BUSY_LED, HIGH);
}

void onIdle() {
    // turn LED off when we're no longer busy
    digitalWrite(BUSY_LED, LOW);
}

void onTimeout() {
    Serial.println(P("!!!\tTimeout occurred!"));
}

void onBeforeTXCommand() {
}

void onTXCommandComplete() {
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

void my_rsp_connection_get_rssi(const ble_msg_connection_get_rssi_rsp_t *msg) {
    Serial.print(P("<--\tconnection_get_rssi: { "));
    Serial.print(P("conn: "));   Serial.print((uint8)msg -> connection, HEX);
    Serial.print(P(", rssi: ")); Serial.print((uint8)msg -> rssi);
    Serial.println(P(" }"));
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

void my_rsp_sm_passkey_entry(const ble_msg_sm_passkey_entry_rsp_t *msg) {
    Serial.print(P("<--\tsm_passkey_entry: { "));
    // 0x0181: Device in Wrong State Device is in wrong state to receive command
    Serial.print(P("result: "));   Serial.print((uint16)msg -> result, HEX);
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

void my_evt_connection_status_evt_t(const ble_msg_connection_status_evt_t *msg) {
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
        // we'll encrypt after client read authentication attribute
        // auto enrypt
        // this shows bonding dialog on iOS
        // ble112.encryptStart();
    }
    else if ( (msg->bonding != INVALID_BOND_HANDLE) &&
         (msg->flags | BGLIB_CONNECTION_ENCRYPTED) ) {
        auth.setCurrentBondHandle(msg->bonding);
    }
}

void my_evt_connection_disconnected(const ble_msg_connection_disconnected_evt_t *msg) {
    Serial.println( P("###\tdisconnected") );

    auth.setCurrentBondHandle(INVALID_BOND_HANDLE);

    ble112.startAdvertising();
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

void my_evt_attributes_user_read_request(const struct ble_msg_attributes_user_read_request_evt_t* msg) {
    Serial.print( P("###\tattributes_user_read_request: { ") );
    Serial.print(P("conn: "));  Serial.print((uint8)msg -> connection, HEX);
    Serial.print(P(", att.handle: ")); Serial.print((uint16)msg -> handle, HEX);
    Serial.print(P(", offset: ")); Serial.print((uint16)msg -> offset, HEX);
    Serial.print(P(", maxsize: ")); Serial.print((uint8)msg -> maxsize, HEX);
    Serial.println(P(" }"));

    switch (msg->handle) {
    case ATTRIBUTE_HANDLE_IR_DATA:
        {
            bool authorized = auth.isAuthorized();
            if ( ! authorized ) {
                // TODO
                break;
            }
            if ( (IrCtrl.state != IR_IDLE) && (IrCtrl.state != IR_RECVED_IDLE) ) {
                // must be idle
                // TODO error response
                break;
            }
            if ( IrCtrl.len * 2 <= msg->offset ) {
                // range error
                // TODO
                break;
            }
            uint8 value_len = msg->maxsize;
            if ( IrCtrl.len * 2 < msg->offset + (uint16_t)value_len ) {
                // last partial
                value_len = IrCtrl.len * 2 - msg->offset;
            }
            uint8* buffWithOffset = (uint8*)IrCtrl.buff + msg->offset;

            ble112.attributesUserReadResponseData( 0, // att_error
                                                   value_len, // value_len
                                                   buffWithOffset // value_data
                                                   );
        }
        break;
    case ATTRIBUTE_HANDLE_IR_UNREAD_STATUS:
        {
            bool unread = 0;
            if ( (IrCtrl.len > 0) &&
                 ((IrCtrl.state == IR_IDLE) || (IrCtrl.state == IR_RECVED_IDLE)) ) {
                unread = 1;
            }
            ble112.attributesUserReadResponseUnread( unread );
        }
        break;
    // case ATTRIBUTE_HANDLE_IR_CONTROL_POINT:
    //     break;

    // TODO
    // case ATTRIBUTE_HANDLE_IR_CARRIER_FREQUENCY:
    //     break;
    case ATTRIBUTE_HANDLE_IR_AUTH_STATUS:
        {
            bool authorized = auth.isAuthorized();
            ble112.attributesUserReadResponseAuthorized( authorized );
            if ( ! authorized ) {
                ble112.nextCommand = NEXT_COMMAND_ID_ENCRYPT_START;
            }
        }
        break;
    default:
        Serial.println(P("unknown attribute !"));
        break;
    }
}

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
    ble112.nextCommand = NEXT_COMMAND_ID_USER_WRITE_RESPONSE_SUCCESS;

    if (msg->reason != BGLIB_ATTRIBUTES_ATTRIBUTE_CHANGE_REASON_WRITE_REQUEST_USER) {
        return;
    }
    bool authorized = auth.isAuthorized();
    if ( ! authorized ) {
        // writes always require authz
        // TODO respond with error
        return;
    }
    if ( (IrCtrl.state != IR_IDLE) && (IrCtrl.state != IR_RECVED_IDLE) ) {
        // must be idle
        // TODO error response
        return;
    }

    uint8 *buffWithOffset;
    switch (msg->handle) {
    case ATTRIBUTE_HANDLE_IR_DATA:
        {
            Serial.print(P("free:"));
            Serial.println( freeMemory() );

            // valid offset and length?
            if (IR_BUFF_SIZE * 2 < msg->offset + msg->value.len) {
                // overflow
                // TODO error response
                Serial.println(P("!overflow!"));
                return;
            }
            // ready to fill IR data
            if (msg->offset == 0) {
                IR_state( IR_IDLE ); // clear IrCtrl.buff
            }

            buffWithOffset = (uint8*)IrCtrl.buff + msg->offset;
            memcpy( buffWithOffset, (const void*)(msg->value.data), msg->value.len );
            IrCtrl.len = (msg->offset + msg->value.len) / 2;
        }
        break;
    case ATTRIBUTE_HANDLE_IR_CONTROL_POINT:
        {
            if (msg->value.data[0] != 0) {
                // unknown control point value
                // TODO error response
                return;
            }
            if ( IrCtrl.len == 0 ) {
                // TODO error response
                return;
            }
            Serial.println(P("will send"));
            IR_xmit();
            Serial.println(P("sent"));
        }
        break;
    default:
        // not expected
        // TODO error response
        break;
    }
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

void my_evt_sm_passkey_display(const struct ble_msg_sm_passkey_display_evt_t *msg) {
    Serial.print( P("###\tsm_passkey_display: { ") );
    Serial.print(P("handle: "));   Serial.print((uint8)msg -> handle, HEX);
    Serial.print(P(", passkey: ")); Serial.print((uint32)msg -> passkey, HEX);
    Serial.println(P(" }"));
}

void my_evt_sm_passkey_request(const struct ble_msg_sm_passkey_request_evt_t *msg) {
    Serial.print( P("###\tsm_passkey_request: { ") );
    Serial.print(P("handle: "));   Serial.print((uint8)msg -> handle, HEX);
    Serial.println(P(" }"));
}

BLE112::BLE112(HardwareSerial *module) :
    bglib(module, 0, 1),
    nextCommand(0xFF)
{

}

void BLE112::setup()
{
    // set up internal status handlers
    // (these are technically optional)
    bglib.onBusy = onBusy;
    bglib.onIdle = onIdle;
    bglib.onTimeout = onTimeout;

    // ONLY enable these if you are using the <wakeup_pin> parameter in your firmware's hardware.xml file
    bglib.onBeforeTXCommand = onBeforeTXCommand;
    bglib.onTXCommandComplete = onTXCommandComplete;

    // set up BGLib response handlers (called almost immediately after sending commands)
    // (these are also technicaly optional)
    bglib.ble_rsp_system_hello                  = my_rsp_system_hello;
    bglib.ble_rsp_gap_set_scan_parameters       = my_rsp_gap_set_scan_parameters;
    bglib.ble_rsp_gap_discover                  = my_rsp_gap_discover;
    bglib.ble_rsp_gap_end_procedure             = my_rsp_gap_end_procedure;
    bglib.ble_rsp_gap_set_adv_data              = my_rsp_gap_set_adv_data;
    bglib.ble_rsp_gap_set_mode                  = my_rsp_gap_set_mode;
    bglib.ble_rsp_attributes_read               = my_rsp_attributes_read;
    bglib.ble_rsp_attributes_user_read_response = my_rsp_attributes_user_read_response;
    bglib.ble_rsp_attributes_write              = my_rsp_attributes_write;
    bglib.ble_rsp_attributes_user_write_response = my_rsp_attributes_user_write_response;
    bglib.ble_rsp_connection_disconnect         = my_rsp_connection_disconnect;
    bglib.ble_rsp_connection_get_rssi           = my_rsp_connection_get_rssi;
    bglib.ble_rsp_sm_encrypt_start              = my_rsp_sm_encrypt_start;
    bglib.ble_rsp_sm_get_bonds                  = my_rsp_sm_get_bonds;
    bglib.ble_rsp_sm_passkey_entry              = my_rsp_sm_passkey_entry;
    bglib.ble_rsp_sm_set_bondable_mode          = my_rsp_sm_set_bondable_mode;
    bglib.ble_rsp_sm_set_oob_data               = my_rsp_sm_set_oob_data;
    bglib.ble_rsp_sm_set_parameters             = my_rsp_sm_set_parameters;
    bglib.ble_rsp_sm_delete_bonding             = my_rsp_sm_delete_bonding;

    // set up BGLib event handlers (called at unknown times)
    bglib.ble_evt_system_boot                   = my_evt_system_boot;
    bglib.ble_evt_gap_scan_response             = my_evt_gap_scan_response;
    bglib.ble_evt_connection_status             = my_evt_connection_status_evt_t;
    bglib.ble_evt_connection_disconnected       = my_evt_connection_disconnected;
    bglib.ble_evt_attributes_status             = my_evt_attributes_status;
    bglib.ble_evt_attributes_user_read_request  = my_evt_attributes_user_read_request;
    bglib.ble_evt_attributes_value              = my_evt_attributes_value;
    bglib.ble_evt_attclient_attribute_value     = my_evt_attclient_attribute_value;
    bglib.ble_evt_attclient_indicated           = my_evt_attclient_indicated;
    bglib.ble_evt_attclient_procedure_completed = my_evt_attclient_procedure_completed;
    bglib.ble_evt_sm_bonding_fail               = my_evt_sm_bonding_fail;
    bglib.ble_evt_sm_bond_status                = my_evt_sm_bond_status;
    bglib.ble_evt_sm_passkey_display            = my_evt_sm_passkey_display;
    bglib.ble_evt_sm_passkey_request            = my_evt_sm_passkey_request;
}

void BLE112::loop()
{
    bglib.checkActivity();

    switch (nextCommand) {
    case NEXT_COMMAND_ID_ENCRYPT_START:
        nextCommand = NEXT_COMMAND_ID_EMPTY;
        encryptStart();
        break;
    case NEXT_COMMAND_ID_USER_WRITE_RESPONSE_SUCCESS:
        nextCommand = NEXT_COMMAND_ID_EMPTY;
        attributesUserWriteResponse( 0,   // conn_handle
                                     0 ); // att_error
        break;
    case NEXT_COMMAND_ID_EMPTY:
        break;
    }
}

void BLE112::reset()
{
    Serial.println(P("-->\tsystem_reset: { boot_in_dfu: 0 }"));
    bglib.ble_cmd_system_reset(0);

    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
    // system_reset doesn't have a response, but this BGLib
    // implementation allows the system_boot event specially to
    // set the P("busy") flag to false for this particular case
}

void BLE112::hello()
{
    Serial.println(P("-->\tsystem_hello"));
    bglib.ble_cmd_system_hello();

    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
    // response should come back within milliseconds
}

void BLE112::startAdvertising()
{
    setBondableMode();
    setParameters();

    // can't initialize uint8array directly....
    uint8 data[27] = {
        // length
        0x1A,
        // AD Format: Flags
        0x02, 0x01, 0x06,
        // AD Format: 128bit Service UUID
        0x11, 0x07, 0xE4, 0xBA, 0x94, 0xC3,
                    0xC9, 0xB7, 0xCD, 0xB0,
                    0x9B, 0x48, 0x7A, 0x43,
                    0x8A, 0xE5, 0x5A, 0x19,
        // AD Format: Manufacturer Specific Data
        0x04, 0xFF, 0x01, 0x02, 0x03
    };
    setAdvData( 0, (uint8*)&data[0] );

    // TODO: set discoverable mode to limited,
    // and after 30sec, set it to general
    // limited: ad interval 250-500ms, only 30sec
    // general: ad interval 1.28-2.56s, forever
    // BGLIB_GAP_GENERAL_DISCOVERABLE
    setMode( BGLIB_GAP_USER_DATA, BGLIB_GAP_UNDIRECTED_CONNECTABLE );
}

void BLE112::setAdvData( uint8 set_scanrsp, uint8 *data )
{
    Serial.print(P("-->\tgap_set_adv_data: { "));
    Serial.print(P("set_scanrsp: "));  Serial.print(set_scanrsp, HEX);
    Serial.print(P(", data: "));
    uint8array *array = (uint8array*)data;
    for (uint8_t i = 0; i < array->len; i++) {
        if (array->data[i] < 16)
            Serial.write('0');
        Serial.print(array->data[i], HEX);
    }
    Serial.println(P(" }"));

    bglib.ble_cmd_gap_set_adv_data( set_scanrsp, array->len, array->data );

    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
}

void BLE112::setMode( uint8 discoverable, uint8 connectable )
{
    Serial.print(P("-->\tgap_set_mode: { "));
    Serial.print(P("discover: "));  Serial.print(discoverable, HEX);
    Serial.print(P(", connect: ")); Serial.print(connectable, HEX);
    Serial.println(P(" }"));

    bglib.ble_cmd_gap_set_mode( discoverable, connectable );

    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
}

void BLE112::getRSSI()
{
    Serial.println(P("-->\tconnection_get_rssi"));
    bglib.ble_cmd_connection_get_rssi( 0x00 ); // connection handle

    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
}

void BLE112::writeAttribute()
{
    uint8_t status;
    // maximum payload: 20bytes
    // see p26 of Bluetooth_Smart_API_11_11032013.pdf
    uint8 totalSize = BLE112_MAX_CHARACTERISTIC_VALUE_LENGTH;
    // Serial.print(P("totalSize:")); Serial.println(totalSize);

    for (uint8 i=0; i<(totalSize/20)+1; i++) {
        uint8 sendSize = totalSize - (i * 20);
        if (sendSize > 20) {
            sendSize = 20;
        }

        Serial.println(P("-->\tattributes_write"));
        /* Serial.print(P("sendSize:")); Serial.println(sendSize); */
        /* Serial.print(P("i:"));        Serial.println(i); */

        // handle value:
        // When the project is compiled with the BGBuild compiler
        // a text file called attributes.txt is generated.
        // This files contains the ids and corresponding handle values.
        bglib.ble_cmd_attributes_write( (uint16)0x0014,       // handle value
                                        (uint8)(i*20),        // offset
                                        sendSize,             // value_len
                                        (const uint8*)&data[i*20] // value_data
                                        );
        while ((status = bglib.checkActivity(1000)));
    }

}

void BLE112::writeAttributeAuthenticationStatus(bool authenticated)
{
    Serial.print(P("-->\tattributes_write auth status: "));
    Serial.println(authenticated, BIN);

    bglib.ble_cmd_attributes_write( (uint16)ATTRIBUTE_HANDLE_IR_AUTH_STATUS, // handle value
                                    0,                                       // offset
                                    1,                                       // value_len
                                    (const uint8*)&authenticated             // value_data
                                    );
    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
}

// BLE112 sends, and iOS device receives a notification
// even if unread value doesnt change
void BLE112::writeAttributeUnreadStatus(bool unread)
{
    Serial.print(P("-->\tattributes_write unread status: "));
    Serial.println(unread, BIN);

    bglib.ble_cmd_attributes_write( (uint16)ATTRIBUTE_HANDLE_IR_UNREAD_STATUS, // handle value
                                    0,                                         // offset
                                    1,                                         // value_len
                                    (const uint8*)&unread                      // value_data
                                    );
    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
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
        bglib.ble_cmd_attributes_read( (uint16)0x0011,       // handle value
                                        (uint8)(i*20) );      // offset
        while ((status = bglib.checkActivity(1000)));
    }
}

void BLE112::disconnect() {
    Serial.println(P("-->\tdisconnect"));
    bglib.ble_cmd_connection_disconnect( (uint8)0 ); // connection handle

    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
}

void BLE112::encryptStart()
{
    Serial.println(P("-->\tsm_encrypt_start"));
    bglib.ble_cmd_sm_encrypt_start( (uint8)0, // connection handle
                                    (uint8)1  // create bonding if devices are not already bonded
                                    );
    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
}

void BLE112::getBonds()
{
    Serial.println(P("-->\tsm_get_bonds"));
    bglib.ble_cmd_sm_get_bonds();

    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
}

void BLE112::passkeyEntry()
{
    Serial.println(P("-->\tsm_passkey_entry"));
    bglib.ble_cmd_sm_passkey_entry( (uint8)0,  // connection handle
                                    (uint32)0  // passkey
                                    );

    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
}

void BLE112::setBondableMode()
{
    Serial.println(P("-->\tsm_set_bondable_mode"));
    bglib.ble_cmd_sm_set_bondable_mode( 1 ); // this device is bondable

    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
}

void BLE112::setOobData()
{
    Serial.println(P("-->\tsm_set_oob_data"));
    bglib.ble_cmd_sm_set_oob_data( (uint8)2, // oob_len
                                   data // oob_data
                                   );

    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
}

void BLE112::setParameters()
{
    Serial.println(P("-->\tsm_set_parameters"));
    // can't enable man-in-the-middle protection without having any keyboard nor display
    bglib.ble_cmd_sm_set_parameters( (uint8)0, // man-in-the-middle protection NOT required
                                     (uint8)16, // minimum key size in bytes range 7-16
                                     (uint8)3   // SMP IO Capabilities (No input, No output)
                                     );

    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
}

void BLE112::deleteBonding(uint8 connectionHandle)
{
    Serial.println(P("-->\tsm_delete_bonding"));
    bglib.ble_cmd_sm_delete_bonding( connectionHandle );

    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
}

void BLE112::attributesUserReadResponse()
{
    Serial.println(P("-->\tattributes_user_read_response"));
    bglib.ble_cmd_attributes_user_read_response( (uint8)0, // connection handle
                                                 (uint8)0, // att_error
                                                 (uint8)22, // value_len,
                                                 data      // value_data
                                                 );
    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
}

void BLE112::attributesUserReadResponseData(uint8 att_error, uint8 value_len, uint8* value_data)
{
    Serial.println(P("-->\tattributes_user_read_response irdata"));

    bglib.ble_cmd_attributes_user_read_response( (uint8)0,   // connection handle
                                                 (uint8)att_error,   // att_error
                                                 (uint8)value_len,   // value_len,
                                                 (uint8*)value_data // value_data
                                                 );
    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
}

void BLE112::attributesUserReadResponseAuthorized(bool authorized)
{
    Serial.print(P("-->\tattributes_user_read_response authorized: "));
    Serial.println(authorized, BIN);

    bglib.ble_cmd_attributes_user_read_response( (uint8)0,   // connection handle
                                                 (uint8)0,   // att_error
                                                 (uint8)1,   // value_len,
                                                 (uint8*)&authorized // value_data
                                                 );
    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
}

void BLE112::attributesUserReadResponseUnread(bool unread)
{
    Serial.print(P("-->\tattributes_user_read_response unread: "));
    Serial.println(unread, BIN);

    bglib.ble_cmd_attributes_user_read_response( (uint8)0,   // connection handle
                                                 (uint8)0,   // att_error
                                                 (uint8)1,   // value_len,
                                                 (uint8*)&unread // value_data
                                                 );
    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
}

void BLE112::attributesUserWriteResponse( uint8 conn_handle, uint8 att_error )
{
    Serial.println(P("-->\tattributes_user_write_response"));
    bglib.ble_cmd_attributes_user_write_response( conn_handle,
                                                  att_error
                                                  );

    uint8_t status;
    while ((status = bglib.checkActivity(1000)));
}
