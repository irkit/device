#ifndef __BASE64ENCODER_H__
#define __BASE64ENCODER_H__

#include <inttypes.h>

typedef void (*Base64EncodeCallback)(char);
extern uint16_t base64_length(uint16_t input_length);
extern void base64_encode(const uint8_t *input,
                          uint16_t input_length,
                          Base64EncodeCallback callback);

#endif
