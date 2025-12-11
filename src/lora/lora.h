#ifndef _LORAMAC_H_
#define _LORAMAC_H_

#include <stdint.h>

typedef struct LoRaConfig_s
{
    uint8_t deveui[8];
    uint8_t joineui[8];
    uint8_t appkey[16];
    uint8_t join_dr;
    uint8_t adr;
} LoRaConfig_t;

typedef struct LoRaData_s
{
    uint8_t data[256];
    uint8_t data_len;
    uint8_t is_confirmed;
    uint8_t port;
} LoRadata_t;

int lora_init();
int lora_join_network(LoRaConfig_t *config);
int lora_send_data(LoRadata_t *data);
int lora_get_downlink(LoRadata_t *data);

#endif