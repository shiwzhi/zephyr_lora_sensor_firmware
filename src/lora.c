#include "lora.h"
#include "LoRaMac.h"
#include "board.h"
#include <stdio.h>
#include <string.h>

#define LORA_REGION LORAMAC_REGION_EU433
#define LORA_ADR true
#define JOIN_DR DR_3
#define MaxERROR 100

#define PRINTFLN(...)        \
    do                       \
    {                        \
        printf(__VA_ARGS__); \
        puts("");            \
    } while (0)

static bool is_joined = false;
static bool is_init = false;

static LoRaMacCallback_t LoRaMacCallbacks;
static LoRaMacPrimitives_t LoRaMacPrimitives;
static LoRaMacStatus_t status;

static MibRequestConfirm_t mibReq;
static MlmeReq_t mlmeReq;
static McpsReq_t mcpsReq;

static uint8_t _deveui[8];
static uint8_t _joineui[8];
static uint8_t _appkey[16];

static uint16_t tx_counter = 0;

void BoardGetUniqueId(uint8_t *id)
{
    /* Do not change the default value */
}

static void McpsConfirm(McpsConfirm_t *mcpsConfirm)
{
    PRINTFLN("McpsConfirm");
}

static void McpsIndication(McpsIndication_t *mcpsIndication)
{
    PRINTFLN("McpsIndication");
    tx_counter = 0;
}

static void MlmeConfirm(MlmeConfirm_t *mlmeConfirm)
{
    PRINTFLN("MlmeConfirm Status %d", mlmeConfirm->Status);

    if (mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK)
    {
        switch (mlmeConfirm->MlmeRequest)
        {
        case MLME_JOIN:
        {
            PRINTFLN("Joined network");
            is_joined = true;
            break;
        }
        case MLME_LINK_CHECK:
        {
            // Check DemodMargin
            // Check NbGateways
            break;
        }
        default:
            break;
        }
    }
}

static void MlmeIndication(MlmeIndication_t *mlmeIndication)
{
}

static void OnMacProcessNotify(void)
{
    LoRaMacProcess();
}

void lora_join()
{
    LoRaMacStop();
    is_joined = false;

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

    mlmeReq.Type = MLME_JOIN;
    mlmeReq.Req.Join.Datarate = JOIN_DR;
    status = LoRaMacMlmeRequest(&mlmeReq);

    if (status != LORAMAC_STATUS_OK)
    {
        PRINTFLN("Join failed:%d", status);
    }
}

int lora_init(uint8_t deveui[8], uint8_t joineui[8], uint8_t appkey[16])
{
    memcpy(_deveui, deveui, 8);
    memcpy(_joineui, joineui, 8);
    memcpy(_appkey, appkey, 16);

    PRINTFLN("DEVEUI:");
    for (int i = 0; i < 8; i++)
    {
        printf("%02x", _deveui[i]);
    }
    PRINTFLN("");

    PRINTFLN("APPKEY:");
    for (int i = 0; i < 16; i++)
    {
        printf("%02x", _appkey[i]);
    }
    PRINTFLN("");

    if (!is_init)
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
            PRINTFLN("Status: %d", status);
            return -1;
        }
        is_init = true;
    }

    lora_join();

    return 0;
}

int lora_send(uint8_t *buffer, uint8_t len)
{
    if (!is_joined)
    {
        PRINTFLN("Not joined");

        lora_join();
        return -1;
    }

    tx_counter++;
    if (tx_counter < 5)
    {
        mcpsReq.Type = MCPS_UNCONFIRMED;
        mcpsReq.Req.Unconfirmed.fPort = 1;
        mcpsReq.Req.Unconfirmed.fBuffer = buffer;
        mcpsReq.Req.Unconfirmed.fBufferSize = len;
        status = LoRaMacMcpsRequest(&mcpsReq);
    }
    else
    {
        mcpsReq.Type = MCPS_CONFIRMED;
        mcpsReq.Req.Confirmed.fPort = 1;
        mcpsReq.Req.Confirmed.fBuffer = buffer;
        mcpsReq.Req.Confirmed.fBufferSize = len;
        status = LoRaMacMcpsRequest(&mcpsReq);
    }
    if (tx_counter > 10)
    {
        tx_counter = 0;
        lora_join();
    }

    if (status != LORAMAC_STATUS_OK)
    {
        PRINTFLN("Send uplink failed");
        return -1;
    }
    return 0;
}