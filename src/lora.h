#ifndef H_LORA_
#define H_LORA_

#include <stdint.h>

int lora_init(uint8_t deveui[8], uint8_t joineui[8], uint8_t appkey[16]);
int lora_send(uint8_t *buffer, uint8_t len);

#endif