/* Copyright (c) 2014 - 2020, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdint.h>
#include <string.h>

/* HAL */
#include "boards.h"
#include "app_timer.h"

/* Core */
#include "access_config.h"
#include "nrf_sdh_soc.h"

/* Provisioning and configuration */
#include "scanner.h"
#include "advertiser.h"

/* Models */

/* Logging and RTT */
#include "nrf_log.h"

/* Example specific includes */
#include "app_config.h"
#include "example_common.h"
#include "mesh_main.h"
#include "selfmesh_common.h"

#define MESH_SOC_OBSERVER_PRIO  0
#define MESH_ADV_TX_CACHE_SIZE  256
static uint8_t s_originator_adv_packet_buffer[MESH_ADV_TX_CACHE_SIZE];

static advertiser_t m_ori_advertiser;

static bearer_event_flag_t s_selfmesh_be_flag;
static uint8_t s_selfmesh_scan_enable = 0;

static selfmesh_rx_callback_t s_pdu_rx_cb = NULL;

static void mesh_soc_evt_handler(uint32_t evt_id, void * p_context)
{
    timeslot_sd_event_handler(evt_id);
}

NRF_SDH_SOC_OBSERVER(m_mesh_soc_observer, MESH_SOC_OBSERVER_PRIO, mesh_soc_evt_handler, NULL);


static void s_adv_tx_complete_callback(advertiser_t * p_adv,
                                     nrf_mesh_tx_token_t token,
                                     timestamp_t timestamp)
{
}

static bool s_scanner_packet_process_cb(void)
{
    /* Process all incoming packets: */
    const scanner_packet_t * p_scanner_packet = scanner_rx();

    if (p_scanner_packet != NULL)
    {
        nrf_mesh_rx_metadata_t metadata;

        metadata.source = NRF_MESH_RX_SOURCE_SCANNER;
        metadata.params.scanner = p_scanner_packet->metadata;

        /* Adv Ext packets in the advertising channels don't have regular advertising data */
        //if (p_scanner_packet->packet.header.type == BLE_PACKET_TYPE_ADV_NONCONN_IND)
        //{
        //    ad_listener_process((ble_packet_type_t) p_scanner_packet->packet.header.type,
        //                        p_scanner_packet->packet.payload,
        //                        p_scanner_packet->packet.header.length - BLE_ADV_PACKET_OVERHEAD,
        //                        &metadata);
        //}

        scanner_packet_release(p_scanner_packet);
    }

    return !scanner_rx_pending();
}


static void s_scanner_rx_cb(const scanner_packet_t * p_scanner_packet, ts_timestamp_t rx_timestamp_ts)
{
    uint8_t temp;
    if (p_scanner_packet->packet.header.type == BLE_PACKET_TYPE_ADV_NONCONN_IND)
    {
        for(int j=0;j<BLE_ADV_PACKET_PAYLOAD_MAX_LENGTH-20;j+=(1+p_scanner_packet->packet.payload[j]))
        {
            if(p_scanner_packet->packet.payload[j] == 23 && p_scanner_packet->packet.payload[j+1] == 0xFF 
            &&  p_scanner_packet->packet.payload[j+2] == SELFMESH_MAN_ID_L &&  p_scanner_packet->packet.payload[j+3] == SELFMESH_MAN_ID_M)
            {
                if(NULL != s_pdu_rx_cb)
                {
                    s_pdu_rx_cb(p_scanner_packet->packet.payload+j+4, 20);
                }
            }
        }
    }
}

static bool s_selfmesh_enabled_be_cb(void)
{
    if(0 == s_selfmesh_scan_enable)
    {
        scanner_enable();
    }

    return true;
}

void mesh_init(void)
{
    nrf_clock_lf_cfg_t lfclksrc = DEV_BOARD_LF_CLK_CFG;
    timer_sch_init();
    bearer_event_init(NRF_MESH_IRQ_PRIORITY_LOWEST);
    //bearer_event_init(NRF_MESH_IRQ_PRIORITY_THREAD);

#if !defined(HOST)
#if NRF_SD_BLE_API_VERSION >= 5
    /* From SD BLE API 5 and on, both RC and XTAL should use the PPM-defines. */
    uint32_t lfclk_accuracy = hal_lfclk_ppm_get(lfclksrc.accuracy);
#elif defined(S110)
    uint32_t lfclk_accuracy = hal_lfclk_ppm_get(lfclksrc);
#else
    uint32_t lfclk_accuracy = (lfclksrc.source == NRF_CLOCK_LF_SRC_XTAL)
                                  ? hal_lfclk_ppm_get(lfclksrc.xtal_accuracy)
                                  : 250;

#endif
    timeslot_init(lfclk_accuracy);
#if EXPERIMENTAL_INSTABURST_ENABLED
    instaburst_init(lfclk_accuracy, instaburst_packet_process_cb);
#endif
#endif /* !HOST */
    bearer_handler_init();
    scanner_init(s_scanner_packet_process_cb);
    advertiser_init();
    //ad_listener_init();
    s_selfmesh_be_flag = bearer_event_flag_add(s_selfmesh_enabled_be_cb);
}

void mesh_main_initialize(selfmesh_rx_callback_t callbak)
{
    __LOG_INIT(LOG_SRC_APP | LOG_SRC_ACCESS, LOG_LEVEL_INFO, LOG_CALLBACK_DEFAULT);

    mesh_init();
    s_pdu_rx_cb = callbak;
}


void mesh_main_start(void)
{
    bearer_event_start();
    bearer_handler_start();
    scanner_rx_callback_set(s_scanner_rx_cb);
    //scanner_enable();
    advertiser_instance_init(&m_ori_advertiser,
                             s_adv_tx_complete_callback,
                             s_originator_adv_packet_buffer,
                             sizeof(s_originator_adv_packet_buffer));
    advertiser_enable(&m_ori_advertiser);
    bearer_event_flag_set(s_selfmesh_be_flag);
}

int mesh_rawdata_send_via_adv(uint8_t* rawdata, uint16_t length)
{
    adv_packet_t * p_packet = advertiser_packet_alloc(&m_ori_advertiser, BLE_AD_DATA_OVERHEAD + length+4);
    if (p_packet != NULL)
    {
        ble_ad_data_t * p_ad_data = (ble_ad_data_t *) &p_packet->packet.payload[0];
        p_ad_data->length = length+3;
        p_ad_data->type = 0xFF;
        p_ad_data->data[0] = SELFMESH_MAN_ID_L;
        p_ad_data->data[1] = SELFMESH_MAN_ID_M;
        memcpy(&(p_ad_data->data[2]), rawdata, length);
        p_packet->config.repeats = 1;
        advertiser_packet_send(&m_ori_advertiser, p_packet);
        return 0;
    }
    else
    {
        return 1;
    }
}
