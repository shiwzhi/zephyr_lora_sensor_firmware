#ifndef H_LORA_
#define H_LORA_

int lora_init();
int lora_join();
int lora_send(uint8_t port, uint8_t *buffer, uint8_t len);

#endif