#ifndef __BLE112_H__
#define __BLE112_H__

#include "BGLib.h"

// imported from firmware/ble112/attributes.txt
#define ATTRIBUTE_HANDLE_SOFTWARE_VERSION     22 // 0x16
#define ATTRIBUTE_HANDLE_IR_DATA              26 // 0x1a
#define ATTRIBUTE_HANDLE_IR_UNREAD_STATUS     29 // 0x1d
#define ATTRIBUTE_HANDLE_IR_CONTROL_POINT     33 // 0x21
#define ATTRIBUTE_HANDLE_IR_CARRIER_FREQUENCY 36 // 0x24
#define ATTRIBUTE_HANDLE_IR_AUTH_STATUS       39 // 0x27

#define NEXT_COMMAND_ID_ENCRYPT_START                          0x01
#define NEXT_COMMAND_ID_USER_WRITE_RESPONSE_SUCCESS            0x02
#define NEXT_COMMAND_ID_USER_WRITE_RESPONSE_ERROR_UNAUTHORIZED 0x03
#define NEXT_COMMAND_ID_USER_WRITE_RESPONSE_ERROR_STATE        0x04
#define NEXT_COMMAND_ID_USER_WRITE_RESPONSE_ERROR_UNEXPECTED   0x05
#define NEXT_COMMAND_ID_EMPTY                                  0xFF

#define INVALID_BOND_HANDLE    0xFF

class BLE112 {
    public:
        BLE112(HardwareSerial*, uint8_t);
        uint8 next_command;
        uint8 current_bond_handle;
        bool (*isAuthorizedCallback)(uint8);

        void setup();
        void loop();
        void softwareReset();
        void hardwareReset();
        void hello();
        void startAdvertising();
        void getRSSI();
        void writeAttribute();
        void writeAttributeAuthorizationStatus(bool);
        void writeAttributeUnreadStatus(bool);
        void readAttribute();
        void disconnect();
        void encryptStart();
        void getBonds();
        void passkeyEntry();
        void setOobData();
        void deleteBonding(uint8);
        void attributesUserReadResponse();
        void attributesUserReadResponseData(uint8, uint8, uint8*);
        void attributesUserReadResponseAuthorized(bool);
        void attributesUserReadResponseFrequency(uint16);
        void attributesUserReadResponseSoftwareVersion();
        void attributesUserWriteResponse(uint8, uint8);

    private:
        // create BGLib object:
        //  - use SoftwareSerial por for module comms
        //  - use nothing for passthrough comms (0 = null pointer)
        //  - enable packet mode on API protocol since flow control is unavailable
        BGLib   bglib_;
        uint8_t reset_pin_;

        void setAdvData(uint8, uint8*);
        void gapSetMode(uint8, uint8);
        void gapSetAdvParameters(uint16, uint16, uint8);
        void smSetBondableMode();
        void smSetParameters();
};

#endif
