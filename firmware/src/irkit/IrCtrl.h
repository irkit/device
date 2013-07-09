/*----------------------------------------------------------------------------/
/  IR_CTRL - IR remote control module                         (C)ChaN, 2008
/-----------------------------------------------------------------------------/
/  Common include file for IR_CTRL module and application
/----------------------------------------------------------------------------*/

#include <stdint.h>

/* Put hardware dependent include files here */
#include <avr/io.h>
#include <avr/interrupt.h>

/* Enable/Disable transmission/reception functions <1/0> */
#define IR_USE_XMIT     1
#define IR_USE_RCVR     1
#define IR_USE_NEC      1
#define IR_USE_AEHA     1
#define IR_USE_SONY     1

// Structure of IR function work area
// buff size must be at least 259 to store 16byte of ir data
#define IR_BUFF_SIZE       512
#define IR_DEFAULT_CARRIER 38
typedef struct _irstruct {
    uint8_t state;               // Communication state
    uint8_t trailerCount;        // Number of T_TRAIL time to wait to determine signal ended
    uint8_t freq;                // carrier wave freq in kHz
    uint16_t len;                // Size of buff used
    uint16_t txIndex;            // 0 < txIndex < len
    uint16_t buff[IR_BUFF_SIZE]; // Data buffer 16Byte x 8bit/Byte x 2(HIGH and LOW) x uint16_t
} IR_STRUCT;

/* The work area for IR_CTRL is defined in ir_ctrl.c */
extern
volatile IR_STRUCT IrCtrl;

/* IR control state (state) */
#define IR_IDLE        0    /* In idle state, ready to receive/transmit */
#define IR_RECVING     1    /* An IR frame is being received */
#define IR_RECVED      2    /* An IR frame has been received and data is valid */
#define IR_XMITTING    3    /* IR transmission is in progress */
#define IR_RECVED_IDLE 4    /* Received IR frame is on memory til next receive and transmit */

/* Prototypes */
void IR_initialize (void);
int IR_xmit (uint8_t, const uint8_t*, uint8_t);
void IR_state (uint8_t);
