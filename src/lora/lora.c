#include "lora.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <LoRaMac.h>

LOG_MODULE_REGISTER(lora, LOG_LEVEL_DBG);

static LoRaMacPrimitives_t LoRaMacPrimitives;
static LoRaMacCallback_t LoRaMacCallbacks;
static LoRaMacStatus_t Status;
static MlmeReq_t mlme_req;
static MibRequestConfirm_t mib_req;
static McpsReq_t mcps_req;
static LoRaMacTxInfo_t tx_info;

static uint8_t join_status;
static uint8_t confirm_status;

static LoRadata_t rx_data;
static int is_rx_data = 0;

static void McpsConfirm(McpsConfirm_t *mcpsConfirm)
{
    //
}

static void McpsIndication(McpsIndication_t *mcpsIndication)
{
    // processing downlink
    LOG_DBG("Mcps Indication Type: %i Status: %i Port: %i RxDatarate: %i BufferSize: %i RxData: %i Rssi: %i Snr: %i AckReceived: %i",
            mcpsIndication->McpsIndication,
            mcpsIndication->Status,
            mcpsIndication->Port,
            mcpsIndication->RxDatarate,
            mcpsIndication->BufferSize,
            mcpsIndication->RxData,
            mcpsIndication->Rssi,
            mcpsIndication->Snr,
            mcpsIndication->AckReceived);
    if (mcpsIndication->RxData)
    {
        LOG_HEXDUMP_DBG(mcpsIndication->Buffer, mcpsIndication->BufferSize, "Rx Data: ");
        memcpy(rx_data.data, mcpsIndication->Buffer, mcpsIndication->BufferSize);
        rx_data.data_len = mcpsIndication->BufferSize;
        rx_data.port = mcpsIndication->Port;
        is_rx_data = 1;
    }

    if (mcpsIndication->AckReceived)
    {
        confirm_status = 1;
    }
}

static void MlmeConfirm(MlmeConfirm_t *mlmeConfirm)
{
    if (mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK)
    {
        switch (mlmeConfirm->MlmeRequest)
        {
        case MLME_JOIN:
        {
            LOG_INF("Node has successful joined the network");
            join_status = 1;
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
    // Implementation of the MLME-Indication primitive
}

static void OnMacProcessNotify(void)
{
    LoRaMacProcess();
}

static uint8_t GetBatteryLevel()
{
    return 255;
}

void BoardGetUniqueId(uint8_t *id)
{
    /* Do not change the default value */
}

int lora_init()
{
    LoRaMacPrimitives.MacMcpsConfirm = McpsConfirm;
    LoRaMacPrimitives.MacMcpsIndication = McpsIndication;
    LoRaMacPrimitives.MacMlmeConfirm = MlmeConfirm;
    LoRaMacPrimitives.MacMlmeIndication = MlmeIndication;
    LoRaMacCallbacks.GetBatteryLevel = GetBatteryLevel;
    LoRaMacCallbacks.GetTemperatureLevel = NULL; // apply board specific temperature reading
    LoRaMacCallbacks.NvmDataChange = NULL;
    LoRaMacCallbacks.MacProcessNotify = OnMacProcessNotify;

    Status = LoRaMacInitialization(&LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_EU433);

    if (Status != LORAMAC_STATUS_OK)
    {
        LOG_ERR("LoRaMacInitialization failed");
        return -1;
    }

    Status = LoRaMacStart();

    if (Status != LORAMAC_STATUS_OK)
    {
        LOG_ERR("LoRaMac failed to start");
        return -1;
    }

    // mib_req.Type = MIB_SYSTEM_MAX_RX_ERROR;
    // mib_req.Param.SystemMaxRxError = 20;
    // LoRaMacMibSetRequestConfirm(&mib_req);

    return 0;
}

int lora_join_network(LoRaConfig_t *config)
{
    mlme_req.Type = MLME_JOIN;
    mlme_req.Req.Join.Datarate = config->join_dr;

    mib_req.Type = MIB_DEV_EUI;
    mib_req.Param.DevEui = config->deveui;
    LoRaMacMibSetRequestConfirm(&mib_req);

    mib_req.Type = MIB_JOIN_EUI;
    mib_req.Param.JoinEui = config->joineui;
    LoRaMacMibSetRequestConfirm(&mib_req);

    mib_req.Type = MIB_NWK_KEY;
    mib_req.Param.NwkKey = config->appkey;
    LoRaMacMibSetRequestConfirm(&mib_req);

    mib_req.Type = MIB_APP_KEY;
    mib_req.Param.AppKey = config->appkey;
    LoRaMacMibSetRequestConfirm(&mib_req);

    if (config->adr == 1)
    {
        mib_req.Type = MIB_ADR;
        mib_req.Param.AdrEnable = true;
        LoRaMacMibSetRequestConfirm(&mib_req);
    }

    Status = LoRaMacMlmeRequest(&mlme_req);
    if (Status != LORAMAC_STATUS_OK)
    {
        LOG_ERR("LoRaMac failed to join %i", Status);
        return -1;
    }

    join_status = 0;
    k_msleep(6000);
    if (join_status == 0)
    {
        LOG_ERR("LoRaMac not receive join accept");
        return -1;
    }
    return 0;
}

int lora_send_data(LoRadata_t *data)
{
    if (data == NULL)
    {
        LOG_ERR("Sending data is null");
        return -1;
    }

    Status = LoRaMacQueryTxPossible(data->data_len, &tx_info);
    if (Status != LORAMAC_STATUS_OK)
    {
        /*
         * If status indicates an error, then most likely the payload
         * has exceeded the maximum possible length for the current
         * region and datarate. We can't do much other than sending
         * empty frame in order to flush MAC commands in stack and
         * hoping the application to lower the payload size for
         * next try.
         */
        LOG_ERR("LoRaWAN Query Tx Possible Failed");
        mcps_req.Type = MCPS_UNCONFIRMED;
        mcps_req.Req.Unconfirmed.fBuffer = NULL;
        mcps_req.Req.Unconfirmed.fBufferSize = 0;
        Status = LoRaMacMcpsRequest(&mcps_req);
        return -1;
    }
    else
    {
        switch (data->is_confirmed)
        {
        case 0:
            mcps_req.Type = MCPS_UNCONFIRMED;
            break;
        case 1:
            mcps_req.Type = MCPS_CONFIRMED;
            break;
        }
        mcps_req.Req.Unconfirmed.fPort = data->port;
        mcps_req.Req.Unconfirmed.fBuffer = data->data;
        mcps_req.Req.Unconfirmed.fBufferSize = data->data_len;
        Status = LoRaMacMcpsRequest(&mcps_req);
        if (Status != LORAMAC_STATUS_OK)
        {
            LOG_ERR("LoRaWAN Send failed");
            return -1;
        }

        if (data->is_confirmed)
        {
            confirm_status = 0;
            k_msleep(6000);
            if (confirm_status == 0)
            {
                LOG_ERR("Message not confirmed");
                return -1;
            }
            else
            {
                LOG_INF("Message confirmed");
            }
        }
        return 0;
    }
}

int lora_get_downlink(LoRadata_t *data)
{
    if (is_rx_data != 1)
    {
        LOG_INF("No Rx data");
        return -1;
    }

    is_rx_data = 0;
    memcpy(data, &rx_data, sizeof(LoRadata_t));
    return 0;
}