#include "lora.h"
#include "LoRaMac.h"
#include "board.h"
#include <stdio.h>
#include <string.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(LORA, LOG_LEVEL_DBG);

#define LORA_REGION LORAMAC_REGION_EU433
#define LORA_ADR true
#define MaxERROR 150

static LoRaMacCallback_t LoRaMacCallbacks;
static LoRaMacPrimitives_t LoRaMacPrimitives;
static LoRaMacStatus_t status;

static MibRequestConfirm_t mibReq;
static MlmeReq_t mlmeReq;
static McpsReq_t mcpsReq;

static uint8_t _deveui[8];
static uint8_t _joineui[8];
static uint8_t _appkey[16];

bool is_joined = false;

void BoardGetUniqueId(uint8_t *id)
{
    /* Do not change the default value */
}

static void McpsConfirm(McpsConfirm_t *mcpsConfirm)
{
    printf("McpsConfirm");
}

static void McpsIndication(McpsIndication_t *mcpsIndication)
{
    printf("McpsIndication");
}

static void MlmeIndication(MlmeIndication_t *mlmeIndication)
{
}

static void MlmeConfirm(MlmeConfirm_t *mlmeConfirm)
{
    printf("MlmeConfirm Status %d", mlmeConfirm->Status);

    if (mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK)
    {
        switch (mlmeConfirm->MlmeRequest)
        {
        case MLME_JOIN:
        {
            is_joined = true;
            break;
        }
        case MLME_LINK_CHECK:
        {
            break;
        }
        default:
            break;
        }
    }
}

static void OnMacProcessNotify(void)
{
    LoRaMacProcess();
}

int lora_join()
{
    mlmeReq.Type = MLME_JOIN;
    mlmeReq.Req.Join.Datarate = DR_3;

    status = LoRaMacMlmeRequest(&mlmeReq);

    if (status != LORAMAC_STATUS_OK)
    {
        printf("Join failed:%d", status);
        return -1;
    }

    return 0;
}

int lora_init()
{
    LoRaMacPrimitives.MacMcpsConfirm = McpsConfirm;
    LoRaMacPrimitives.MacMcpsIndication = McpsIndication;
    LoRaMacPrimitives.MacMlmeConfirm = MlmeConfirm;
    LoRaMacPrimitives.MacMlmeIndication = MlmeIndication;
    LoRaMacCallbacks.GetBatteryLevel = NULL;
    LoRaMacCallbacks.GetTemperatureLevel = NULL;
    LoRaMacCallbacks.NvmDataChange = NULL;
    LoRaMacCallbacks.MacProcessNotify = OnMacProcessNotify;

    status = LoRaMacInitialization(&LoRaMacPrimitives, &LoRaMacCallbacks, LORA_REGION);

    if (status != LORAMAC_STATUS_OK)
    {
        printf("Status: %d", status);
        return -1;
    }

    mibReq.Type = MIB_DEV_EUI;
    mibReq.Param.DevEui = _deveui;
    LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_JOIN_EUI;
    mibReq.Param.JoinEui = _joineui;
    LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_NWK_KEY;
    mibReq.Param.NwkKey = _appkey;
    LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_APP_KEY;
    mibReq.Param.AppKey = _appkey;
    LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_PUBLIC_NETWORK;
    mibReq.Param.EnablePublicNetwork = true;
    LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_ADR;
    mibReq.Param.AdrEnable = LORA_ADR;
    LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_SYSTEM_MAX_RX_ERROR;
    mibReq.Param.SystemMaxRxError = MaxERROR;
    LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_NETWORK_ACTIVATION;
    LoRaMacMibSetRequestConfirm(&mibReq);

    LoRaMacStart();

    return 0;
}

static uint8_t tx_buffer[300];
static uint8_t tx_len = 0;
static bool is_tx = false;

void lora_thread(void *, void *, void *)
{
    memset(_deveui, 0, 8);
    memset(_joineui, 0, 8);
    memset(_appkey, 0, 8);

    hwinfo_get_device_id(_appkey, 16);
    memcpy(_deveui, _appkey + 8, 8);

    LOG_HEXDUMP_DBG(_deveui, 8, "DEVEUI:");
    LOG_HEXDUMP_DBG(_joineui, 8, "JOINEUI:");
    LOG_HEXDUMP_DBG(_appkey, 16, "APPKEY:");

    lora_init();

    while (!is_joined)
    {
        lora_join();
        k_msleep(10 * 1000);
    }

    while (1)
    {
        if (is_tx)
        {
            mcpsReq.Type = MCPS_UNCONFIRMED;
            mcpsReq.Req.Unconfirmed.fPort = 1;
            mcpsReq.Req.Unconfirmed.fBuffer = tx_buffer;
            mcpsReq.Req.Unconfirmed.fBufferSize = tx_len;
            status = LoRaMacMcpsRequest(&mcpsReq);
            is_tx = false;
        }
        k_msleep(10 * 1000);
    }
}

K_THREAD_DEFINE(lora_tid, 2048,
                lora_thread, NULL, NULL, NULL,
                7, 0, 0);

int lora_send(uint8_t *buffer, uint8_t len)
{
    memcpy(tx_buffer, buffer, len);
    is_tx = true;
    return 0;
}