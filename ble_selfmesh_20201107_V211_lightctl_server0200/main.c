/* Copyright (c) 2014 - 2020, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
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
/** @file
 *
 * @defgroup ble_sdk_uart_over_ble_main main.c
 * @{
 * @ingroup  ble_sdk_app_nus_eval
 * @brief    UART over BLE application main file.
 *
 * This file contains the source code for a sample application that uses the Nordic UART service.
 * This application uses the @ref srvlib_conn_params module.
 */


#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "app_timer.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "app_util_platform.h"
#include "bsp_btn_ble.h"
#include "nrf_pwr_mgmt.h"
#include "mesh_main.h"
#include "mesh_app_utils.h"
#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"
#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"
#include "rand.h"
#include "selfmesh_common.h"
#include "selfmesh_util.h"
#include "selfmesh_node.h"
#include "selfmesh_element.h"
#include "selfmesh_service.h"
#include "selfsec.h"
#include "lightdrv.h"

#if defined (UART_PRESENT)
#include "nrf_uart.h"
#endif
#if defined (UARTE_PRESENT)
#include "nrf_uarte.h"
#endif

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define USER_DATA_ADDR          0x77000
#define USER_DATA_LEN           0x1000
#define FLASH_PAGE_SIZE         4096
#define BACK_DATA_ADDR          0x78000
#define BACK_DATA_LEN           0x1000
#define PACKET_INDEX_ADDR       0x79000
#define PACKET_INDEX_LEN        0x1000
#define LIGHT_PARAM_ADDR        0x7A000
#define LIGHT_PARAM_LEN         0x1000

#define APP_BLE_CONN_CFG_TAG            1                                           /**< A tag identifying the SoftDevice BLE configuration. */

#define DEVICE_NAME                     "S-0200"                                    /**< Name of device. Will be included in the advertising data. */
#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                  /**< UUID type for the Nordic UART Service (vendor specific). */

#define APP_BLE_OBSERVER_PRIO           3                                           /**< Application's BLE observer priority. You shouldn't need to modify this value. */

#define APP_ADV_INTERVAL                200                                         /**< The advertising interval (in units of 0.625 ms. This value corresponds to 500 ms). */

//#define APP_ADV_DURATION                BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED       /**< Disable advertising timeout. */
#define APP_ADV_DURATION                1800                                        /**< Disable advertising timeout. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(150,  UNIT_1_25_MS)           /**< Minimum acceptable connection interval. */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(250,  UNIT_1_25_MS)           /**< Maximum acceptable connection interval. */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define UART_TX_BUF_SIZE                256                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                256                                         /**< UART RX buffer size. */
#define SELFMESH_PDU_SEND_INTERVAL      50

BLE_NUS_DEF(m_nus, NRF_SDH_BLE_TOTAL_LINK_COUNT);                                   /**< BLE NUS service instance. */
NRF_BLE_GATT_DEF(m_gatt);                                                           /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                                             /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);                                                 /**< Advertising module instance. */
APP_TIMER_DEF(m_selfmesh_pdu_snd_tmr);

static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);

NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
    /* Set a handler for fstorage events. */
    .evt_handler = fstorage_evt_handler,

    /* These below are the boundaries of the flash space assigned to this instance of fstorage.
     * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
     * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
     * last page of flash available to write data. */
    .start_addr = USER_DATA_ADDR,
    .end_addr   = USER_DATA_ADDR + (4 * USER_DATA_LEN) - 1,
};
static uint16_t   m_conn_handle          = BLE_CONN_HANDLE_INVALID;                 /**< Handle of the current connection. */
static uint16_t   m_ble_nus_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;            /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
static ble_uuid_t m_adv_uuids[]          =                                          /**< Universally unique service identifier. */
{
    {BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE}
};

//modify for selfmesh begin


#define SELFMESH_FOR_CACHE_CNT         24
#define SELFMESH_FOR_PROCESS_CNT       8
#define SELFMESH_CHANNEL_PBLE          0x01
#define SELFMESH_CHANNEL_ADV           0x02
#define SELFMESH_CHANNEL_CBLE          0x04
    
typedef struct
{
    volatile uint8_t    need_store;
    volatile uint8_t    write_status;//0 - init status
                                     //1 - erase begin
                                     //2 - write begin
    void *     data_ptr;
    int        offset;
    int        len;
} selfmesh_ud_store_t;

typedef struct
{
    uint8_t    is_retrain_message;
    uint8_t    ud_buf[SELFMESH_UD_BUF_SIZE];
} selfmesh_ud_buffer_t;

typedef struct
{
    uint16_t   rcv_index;
    uint16_t   process_index;
    selfmesh_ud_buffer_t    buf_ud[SELFMESH_FOR_PROCESS_CNT];
} selfmesh_ud_process_t;

typedef struct
{
    uint16_t   rcv_index;
    uint16_t   process_index;
    selfmesh_ud_buffer_t    buf_ud[SELFMESH_FOR_CACHE_CNT];
} selfmesh_ud_cache_t;

#define ROM_MISSION_NO              0
#define ROM_MISSION_YES             1

#define ROM_STATE_IDLE              0
#define ROM_STATE_WAIT              1
#define ROM_STATE_ERASE_START       2
#define ROM_STATE_ERASE_WAIT        3
#define ROM_STATE_WRITE_START       4
#define ROM_STATE_WRITE_WAIT        5

typedef struct {
    uint16_t mission;
    uint16_t cur_state;

    /* 4 byte aligned */
    selfmesh_node_t node;

    uint32_t next_write_offset;
} hw_config_t;

typedef struct {
    uint16_t mission;
    uint16_t cur_state;

    uint8_t index;

    uint32_t next_write_offset;
} packet_index_t;

typedef struct {
    uint16_t mission;
    uint16_t cur_state;

    selfmesh_light_t param;

    uint32_t next_write_offset;
} light_param_t;

static hw_config_t main_config;
static hw_config_t back_config;
static packet_index_t packet_index;
light_param_t light_param;

static prng_t                s_selfmesh_main_prng;
static uint8_t               s_selfmesh_in_config = 0;
static uint8_t               s_selfmesh_in_proxy_relay = 0;
static uint8_t               s_selfmesh_channel   = 0;
static volatile selfmesh_ud_process_t s_selfmesh_ud_in_buf = {0x00};
static volatile selfmesh_ud_process_t s_selfmesh_ud_out_buf = {0x00};
static volatile selfmesh_ud_cache_t   s_selfmesh_cache_in_buf = {0x00};
static volatile uint8_t      s_Adv_channel_busy = 0;
static volatile uint8_t      s_Is_scanning = 0;

static selfmesh_node_t s_selfmesh_node = {0x00};
static selfmesh_ud_store_t s_selfmesh_store = {0x00};
//modify for selfmesh end

static void idle_state_handle(void);

/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyse
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    if (p_evt->result != NRF_SUCCESS)
    {
        //NRF_LOG_INFO("--> Event received: ERROR while executing an fstorage operation.");
        return;
    }

    switch (p_evt->id)
    {
        case NRF_FSTORAGE_EVT_WRITE_RESULT:
        {
            //NRF_LOG_INFO("--> Event received: wrote %d bytes at address 0x%x.",
            //             p_evt->len, p_evt->addr);
        } break;

        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
            //NRF_LOG_INFO("--> Event received: erased %d page from address 0x%x.",
            //             p_evt->len, p_evt->addr);
        } break;

        default:
            break;
    }
}

void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage)
{
    /* While fstorage is busy, sleep and wait for an event. */
    while (nrf_fstorage_is_busy(p_fstorage))
    {
        idle_state_handle();
    }
}

static void s_selfmess_handle_cache(void)
{
    uint8_t data_in = 0;
    uint8_t temp_ud[SELFMESH_UD_BUF_SIZE] = {0};
    uint8_t temp_in_buf[SELFMESH_UD_BUF_SIZE] = {0};
    if(s_selfmesh_cache_in_buf.process_index != s_selfmesh_cache_in_buf.rcv_index)
    {
        memcpy(temp_in_buf, (uint8_t*)(s_selfmesh_cache_in_buf.buf_ud[s_selfmesh_cache_in_buf.process_index].ud_buf), SELFMESH_UD_BUF_SIZE);
        s_selfmesh_cache_in_buf.process_index = (s_selfmesh_cache_in_buf.process_index+1)%SELFMESH_FOR_CACHE_CNT;
        data_in = 1;
    }
    else
    {
        return;
    }
    if(selfmesh_node_pretreat(temp_in_buf,&s_selfmesh_node,temp_ud) != 0)
    {
        NRF_LOG_ERROR("Rx data pretreat error.");
        return;
    }
    for(int i=0;i<SELFMESH_FOR_PROCESS_CNT;i++)
    {
        //if(temp_ud[SELFMASH_MAGIC_M_OFFSET] == s_selfmesh_ud_in_buf.buf_ud[i].ud_buf[SELFMASH_MAGIC_M_OFFSET]
        //   &&temp_ud[SELFMASH_INDEX_OFFSET] == s_selfmesh_ud_in_buf.buf_ud[i].ud_buf[SELFMASH_INDEX_OFFSET]
        //   &&temp_ud[SELFMASH_SOURCE_ADDR_M_OFFSET] == s_selfmesh_ud_in_buf.buf_ud[i].ud_buf[SELFMASH_SOURCE_ADDR_M_OFFSET]
        //   &&temp_ud[SELFMASH_SOURCE_ADDR_L_OFFSET] == s_selfmesh_ud_in_buf.buf_ud[i].ud_buf[SELFMASH_SOURCE_ADDR_L_OFFSET])
        //if(0 == memcmp((const void*)temp_ud,(const void*)s_selfmesh_ud_in_buf.buf_ud[i].ud_buf,SELFMESH_UD_BUF_SIZE))
        {
        //    NRF_LOG_ERROR("Rx data bufferd data dupli.");
        //    return;
        }
    }
    if((s_selfmesh_ud_in_buf.rcv_index+1)%SELFMESH_FOR_PROCESS_CNT != s_selfmesh_ud_in_buf.process_index)
    {
        //NRF_LOG_INFO("Rx data->rcv buf from %02X%02X to %02X%02X.",
        //               temp_ud[SELFMASH_SOURCE_ADDR_M_OFFSET], 
        //               temp_ud[SELFMASH_SOURCE_ADDR_L_OFFSET], 
        //               temp_ud[SELFMASH_DEST_ADDR_M_OFFSET], 
        //               temp_ud[SELFMASH_DEST_ADDR_L_OFFSET]);
        memcpy(s_selfmesh_ud_in_buf.buf_ud[s_selfmesh_ud_in_buf.rcv_index].ud_buf, temp_ud, SELFMESH_UD_BUF_SIZE);
        s_selfmesh_ud_in_buf.rcv_index = (s_selfmesh_ud_in_buf.rcv_index+1)%SELFMESH_FOR_PROCESS_CNT;
    }
}

static void s_selfmesh_pdu_rx(uint8_t * p_pdu, uint16_t len)
{
    if((s_selfmesh_cache_in_buf.rcv_index+1)%SELFMESH_FOR_CACHE_CNT != s_selfmesh_cache_in_buf.process_index)
    {
        memcpy(s_selfmesh_cache_in_buf.buf_ud[s_selfmesh_cache_in_buf.rcv_index].ud_buf, p_pdu, len);
        s_selfmesh_cache_in_buf.rcv_index = (s_selfmesh_cache_in_buf.rcv_index+1)%SELFMESH_FOR_CACHE_CNT;
    }
}

/**@brief Function for initializing the timer module.
 */
static void timers_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function will set up all the necessary GAP (Generic Access Profile) parameters of
 *          the device. It also sets the permissions and appearance.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    uint32_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send
 *          it to the UART module.
 *
 * @param[in] p_evt       Nordic UART Service event.
 */
/**@snippet [Handling the data received over BLE] */
static void nus_data_handler(ble_nus_evt_t * p_evt)
{

    if (p_evt->type == BLE_NUS_EVT_RX_DATA)
    {
        uint32_t err_code;
        if(p_evt->params.rx_data.length != SELFMESH_UD_BUF_SIZE)
        {
            NRF_LOG_ERROR("User buffer length ERROR.");
            return;
        }
        s_selfmesh_pdu_rx(p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);

        //NRF_LOG_DEBUG("Received data from BLE NUS. Writing data on UART.");
    }

}
/**@snippet [Handling the data received over BLE] */


/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    uint32_t           err_code;
    ble_nus_init_t     nus_init;
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    // Initialize NUS.
    memset(&nus_init, 0, sizeof(nus_init));

    nus_init.data_handler = nus_data_handler;

    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling an event from the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module
 *          which are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply setting
 *       the disconnect_on_fail config parameter, but instead we use the event handler
 *       mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    uint32_t err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    uint32_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            break;
        case BLE_ADV_EVT_IDLE:
            if(s_selfmesh_in_config == 1)
            {
                advertising_start();
            }
            //sleep_mode_enter();
            break;
        default:
            break;
    }
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    uint32_t err_code;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected");
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected");
            // LED indication will be changed when advertising starts.
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for the SoftDevice initialization.
 *
 * @details This function initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**@brief Function for handling events from the GATT library. */
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    if ((m_conn_handle == p_evt->conn_handle) && (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {
        m_ble_nus_max_data_len = p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
        NRF_LOG_INFO("Data len is set to 0x%X(%d)", m_ble_nus_max_data_len, m_ble_nus_max_data_len);
    }
    NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
                  p_gatt->att_mtu_desired_central,
                  p_gatt->att_mtu_desired_periph);
}


/**@brief Function for initializing the GATT library. */
void gatt_init(void)
{
    ret_code_t err_code;

    err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
void bsp_event_handler(bsp_event_t event)
{
    switch (event)
    {
        case BSP_EVENT_KEY_0:
        case BSP_EVENT_KEY_1:
        case BSP_EVENT_KEY_2:
        case BSP_EVENT_KEY_3:
            break;

        default:
            break;
    }
}


/**@brief   Function for handling app_uart events.
 *
 * @details This function will receive a single character from the app_uart module and append it to
 *          a string. The string will be be sent over BLE when the last character received was a
 *          'new line' '\n' (hex 0x0A) or if the string has reached the maximum data length.
 */
/**@snippet [Handling the data received over UART] */
void uart_event_handle(app_uart_evt_t * p_event)
{
    static uint8_t data_array[BLE_NUS_MAX_DATA_LEN];
    static uint8_t index = 0;
    uint32_t       err_code;

    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY:
            UNUSED_VARIABLE(app_uart_get(&data_array[index]));
            index++;

            if ((data_array[index - 1] == '\n') ||
                (data_array[index - 1] == '\r') ||
                (index >= m_ble_nus_max_data_len))
            {
                if (index > 1)
            {
                NRF_LOG_DEBUG("Ready to send data over BLE NUS");
                NRF_LOG_HEXDUMP_DEBUG(data_array, index);

                do
                {
                    uint16_t length = (uint16_t)index;
                    err_code = ble_nus_data_send(&m_nus, data_array, &length, m_conn_handle);
                        if ((err_code != NRF_ERROR_INVALID_STATE) &&
                            (err_code != NRF_ERROR_RESOURCES) &&
                         (err_code != NRF_ERROR_NOT_FOUND) )
                    {
                        APP_ERROR_CHECK(err_code);
                    }
                    } while (err_code == NRF_ERROR_RESOURCES);
                }

                index = 0;
            }
            break;

        case APP_UART_COMMUNICATION_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_communication);
            break;

        case APP_UART_FIFO_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_code);
            break;

        default:
            break;
    }
}
/**@snippet [Handling the data received over UART] */


/**@brief  Function for initializing the UART module.
 */
/**@snippet [UART Initialization] */
static void uart_init(void)
{
    uint32_t                     err_code;
    app_uart_comm_params_t const comm_params =
    {
        .rx_pin_no    = RX_PIN_NUMBER,
        .tx_pin_no    = TX_PIN_NUMBER,
        .rts_pin_no   = RTS_PIN_NUMBER,
        .cts_pin_no   = CTS_PIN_NUMBER,
        .flow_control = APP_UART_FLOW_CONTROL_DISABLED,
        .use_parity   = false,
#if defined (UART_PRESENT)
        .baud_rate    = NRF_UART_BAUDRATE_115200
#else
        .baud_rate    = NRF_UARTE_BAUDRATE_115200
#endif
    };

    APP_UART_FIFO_INIT(&comm_params,
                       UART_RX_BUF_SIZE,
                       UART_TX_BUF_SIZE,
                       uart_event_handle,
                       APP_IRQ_PRIORITY_LOWEST,
                       err_code);
    APP_ERROR_CHECK(err_code);
}
/**@snippet [UART Initialization] */


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    uint32_t               err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance = false;
    init.advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_adv_uuids;

    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;
    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}


/**@brief Function for initializing buttons and leds.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 */
static void buttons_leds_init(bool * p_erase_bonds)
{
    bsp_event_t startup_event;

    uint32_t err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}

//modify for selfmesh begin
static void gpiote_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
}

void module_board_init(void)
{
    uint32_t err_code;

    if (!nrf_drv_gpiote_is_init())
    {
        err_code = nrf_drv_gpiote_init();
    }
    nrf_drv_gpiote_in_config_t config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
    config.pull = NRF_GPIO_PIN_PULLUP;
    nrf_gpio_cfg_input(LIGHT_CTR_BUTTON, NRF_GPIO_PIN_PULLUP);
    err_code = nrf_drv_gpiote_in_init(LIGHT_CTR_BUTTON, &config, gpiote_event_handler);
    nrf_drv_gpiote_in_event_enable(LIGHT_CTR_BUTTON, true);

    nrf_gpio_cfg_output(LIGHT_CTR_LED);
    nrf_gpio_pin_set(LIGHT_CTR_LED);
        
}

int selfmesh_is_key_pressed(uint8_t key)
{
    if((key == 1) && nrf_gpio_pin_read(LIGHT_CTR_BUTTON) == 0)
    {
        return 1;
    }
    return 0;
}

int32_t find_offset(nrf_fstorage_t const *fs, uint32_t start_address, uint32_t len)
{
    int32_t offset;
    uint8_t tmp[4];

    /* find valid data address */
    for (offset = len - 4; offset >= 0; offset -= 4) {
        nrf_fstorage_read(fs, start_address + offset, tmp, 4);
        if (tmp[3] != 0xFF) {
            offset += 3;
            break;
        } else if (tmp[2] != 0xFF) {
            offset += 2;
            break;
        } else if (tmp[1] != 0xFF) {
            offset += 1;
            break;
        } else if (tmp[0] != 0xFF) {
            offset += 0;
            break;
        }
    }

    return offset;
}

bool check_hw_info_valid(nrf_fstorage_t const *fs, uint32_t start_address, int32_t offset)
{
    const uint32_t size = sizeof(selfmesh_node_t) - 2;
    uint16_t magic_value;
    uint32_t index;
    uint32_t address;
    uint8_t tmp[4];

    bool ret = false;

    index = offset/size;
    address = start_address + index * size;

    /* header magic */
    nrf_fstorage_read(fs, address, &magic_value, 2);
    if (magic_value == SELFMESH_NODE_MAGIC_VALUE) {
        /* tail magic */
        nrf_fstorage_read(fs, address + size - 4, tmp, 4);
        magic_value = (tmp[2] << 8) + tmp[3];
        if (magic_value == SELFMESH_NODE_MAGIC_VALUE)
            ret = true;
    }

    return ret;
}

void read_packet_index(nrf_fstorage_t const *fs, packet_index_t *index)
{
    int32_t offset;
    uint8_t tmp[4];

    offset = find_offset(fs, PACKET_INDEX_ADDR, PACKET_INDEX_LEN);
    if (offset < 0) {
        /* default value */
        index->index = 0;
        index->next_write_offset = PACKET_INDEX_ADDR;
    } else {
        nrf_fstorage_read(fs, PACKET_INDEX_ADDR + (offset & (~0x03)), tmp, 4);
        index->index = tmp[offset & 0x03];
        index->next_write_offset = PACKET_INDEX_ADDR + offset + 1;
    }

    index->mission = ROM_MISSION_NO;
    index->cur_state = ROM_STATE_IDLE;
    return;
}

uint32_t read_rom_light_param(selfmesh_light_t *param)
{
    const uint32_t size = 6;
    uint32_t corr_addr, deviation;
    int32_t offset;
    uint8_t tmp[8];

    offset = find_offset(&fstorage, LIGHT_PARAM_ADDR, LIGHT_PARAM_LEN);
    if (offset < 0) {
        /* default value */
        param->swtch = 0xFFFF;
        param->lightness = 0xFFFF;
        param->lighttemp = 0xFFFF;

        return LIGHT_PARAM_ADDR;
    } else {
        corr_addr = offset/size;
        deviation = (corr_addr * size) & 0x03;
        corr_addr = LIGHT_PARAM_ADDR + corr_addr * size - deviation;
        nrf_fstorage_read(&fstorage, corr_addr, tmp, 8);

        param->swtch = (tmp[deviation + 1] << 8) + tmp[deviation + 0];
        param->lightness = (tmp[deviation + 3] << 8) + tmp[deviation + 2];
        param->lighttemp = (tmp[deviation + 5] << 8) + tmp[deviation + 4];

        return (corr_addr + deviation + size);
    }
}

void read_light_param(nrf_fstorage_t const *fs, light_param_t *light)
{
    uint32_t addr;
    
    addr = read_rom_light_param(&light->param);
    light->next_write_offset = addr;

    light->mission = ROM_MISSION_NO;
    light->cur_state = ROM_STATE_IDLE;
    return;
}

void read_hw_info(nrf_fstorage_t const *fs, hw_config_t *config, hw_config_t *bconfig)
{
    const uint32_t size = sizeof(selfmesh_node_t) - 2;
    int32_t main_offset;
    bool main_valid = false;
    int32_t back_offset;
    bool back_valid = false;

    uint32_t index;

    /* check main area */
    main_offset = find_offset(fs, USER_DATA_ADDR, USER_DATA_LEN);
    if (main_offset >= 0)
        main_valid = check_hw_info_valid(fs, USER_DATA_ADDR, main_offset);

    /* check backup area */
    back_offset = find_offset(fs, BACK_DATA_ADDR, BACK_DATA_LEN);
    if (back_offset >= 0)
        back_valid = check_hw_info_valid(fs, BACK_DATA_ADDR, back_offset);

    /* main area is empty, all bytes are 0xFF */
    if (main_offset < 0) {
        if ((back_valid == true) && ((back_offset/size) == 0)) {
            /* back area has data in the start address */
            nrf_fstorage_read(fs, BACK_DATA_ADDR, &config->node, size);
            config->next_write_offset = USER_DATA_ADDR + size;

            /* write to main area again */
            nrf_fstorage_write(fs, USER_DATA_ADDR, &config->node, size, NULL);
            wait_for_flash_ready(fs);
        } else {
            /* back area has data, but not in start address */
            if (back_offset >= 0) {
                nrf_fstorage_erase(fs, BACK_DATA_ADDR, 1, NULL);
                wait_for_flash_ready(fs);
            }

            /* default value */
            memset(&config->node, 0xFF, size);
            config->next_write_offset = USER_DATA_ADDR;
        }

        config->mission = ROM_MISSION_NO;
        config->cur_state = ROM_STATE_IDLE;

        /* copy data */
        memcpy(bconfig, config, sizeof(hw_config_t));
        bconfig->next_write_offset += USER_DATA_LEN;
        return;
    }

    /* main area has broken data, so check back area */
    if (main_valid == false) {
        index = back_offset/size;

        if ((back_valid == true) && ((main_offset/size) == index)) {
            /* back area has data in the same position */
            nrf_fstorage_read(fs, BACK_DATA_ADDR + index * size, &config->node, size);
            config->next_write_offset = USER_DATA_ADDR + size * (index + 1);

            /* write to main area again */
            nrf_fstorage_write(fs, USER_DATA_ADDR + index * size, &config->node, size, NULL);
            wait_for_flash_ready(fs);
        } else {
            nrf_fstorage_erase(fs, USER_DATA_ADDR, 1, NULL);
            wait_for_flash_ready(fs);

            if (back_offset >= 0) {
                nrf_fstorage_erase(fs, BACK_DATA_ADDR, 1, NULL);
                wait_for_flash_ready(fs);
            }

            /* default value */
            memset(&config->node, 0xFF, size);
            config->next_write_offset = USER_DATA_ADDR;
        }
    } else {
        index = main_offset/size;
        nrf_fstorage_read(fs, USER_DATA_ADDR + index * size, &config->node, size);
        config->next_write_offset = USER_DATA_ADDR + size * (index + 1);

        /* write to back area */
        nrf_fstorage_write(fs, BACK_DATA_ADDR + index * size, &config->node, size, NULL);
        wait_for_flash_ready(fs);
    }

    config->mission = ROM_MISSION_NO;
    config->cur_state = ROM_STATE_IDLE;

    /* copy data */
    memcpy(bconfig, config, sizeof(hw_config_t));
    bconfig->next_write_offset += USER_DATA_LEN;
    return;
}

void write_hw_info(nrf_fstorage_t const *fs, uint32_t base_address, hw_config_t *config)
{
    const uint32_t size = sizeof(selfmesh_node_t) - 2;
    const uint32_t number = USER_DATA_LEN/size;
    uint16_t magic_value;

    switch (config->cur_state) {
    case ROM_STATE_IDLE:
        if (config->next_write_offset >= (base_address + (size * number))) {
            config->cur_state = ROM_STATE_ERASE_START;
        } else if (config->next_write_offset == base_address) {
            nrf_fstorage_read(fs, base_address, &magic_value, 2);
            if (magic_value != 0xFFFF)
                config->cur_state = ROM_STATE_ERASE_START;
            else
                config->cur_state = ROM_STATE_WRITE_START;
        } else {
            config->cur_state = ROM_STATE_WRITE_START;
        }
        break;
    case ROM_STATE_ERASE_START:
        if (nrf_fstorage_is_busy(fs) == false) {
            nrf_fstorage_erase(fs, base_address, 1, NULL);
            config->cur_state = ROM_STATE_ERASE_WAIT;
        }
        break;
    case ROM_STATE_ERASE_WAIT:
        if (nrf_fstorage_is_busy(fs) == false) {
            config->cur_state = ROM_STATE_WRITE_START;
            config->next_write_offset = base_address;
        }
        break;
    case ROM_STATE_WRITE_START:
        if (nrf_fstorage_is_busy(fs) == false) {
            nrf_fstorage_write(fs, config->next_write_offset, &config->node, size, NULL);
            config->cur_state = ROM_STATE_WRITE_WAIT;
        }
        break;
    case ROM_STATE_WRITE_WAIT:
        if (nrf_fstorage_is_busy(fs) == false) {
            /* write finished */
            config->mission = ROM_MISSION_NO;
            config->cur_state = ROM_STATE_IDLE;
            config->next_write_offset += size;
        }
        break;
    case ROM_STATE_WAIT:
    default:
        break;
    }
}

int selfmesh_store(selfmesh_node_t* node_cfg_ptr, int offset, int msg_len, uint8_t flush)
{
    const uint32_t size = sizeof(selfmesh_node_t) - 2;
    const uint32_t number = USER_DATA_LEN/size;
    hw_config_t *mconfig = &main_config;
    hw_config_t *bconfig = &back_config;

    if (flush == 1)
    {
        /* copy data */
        memcpy(&mconfig->node, node_cfg_ptr, sizeof(selfmesh_node_t));

        /* write it to rom */
        wait_for_flash_ready(&fstorage);

        mconfig->mission = ROM_MISSION_YES;
        mconfig->cur_state = ROM_STATE_IDLE;
        if (mconfig->next_write_offset >= (USER_DATA_ADDR + size * number)) {
            /* full, so erase it */
            nrf_fstorage_erase(&fstorage, USER_DATA_ADDR, 1, NULL);
            wait_for_flash_ready(&fstorage);

            mconfig->next_write_offset = USER_DATA_ADDR;
        }

        nrf_fstorage_write(&fstorage, mconfig->next_write_offset, &mconfig->node, size, NULL);
        wait_for_flash_ready(&fstorage);

        /* update statistic */
        mconfig->next_write_offset += size;
        mconfig->mission = ROM_MISSION_NO;

        /* mark it, so later will write to another rom address */
        bconfig->mission = ROM_MISSION_YES;
        bconfig->cur_state = ROM_STATE_IDLE;

        /* copy data */
        memcpy(&bconfig->node, &mconfig->node, sizeof(selfmesh_node_t));
        bconfig->next_write_offset = mconfig->next_write_offset + 0x1000 - size;
    }
    else
    {
        mconfig->mission = ROM_MISSION_YES;
        if ((bconfig->cur_state != ROM_STATE_IDLE)
            && (bconfig->cur_state != ROM_STATE_WAIT)) {
            mconfig->cur_state = ROM_STATE_WAIT;
        } else {
            mconfig->cur_state = ROM_STATE_IDLE;
        }

        /* copy data */
        memcpy(&mconfig->node, node_cfg_ptr, sizeof(selfmesh_node_t));
    }

    return 0;
}

void write_packet_index(nrf_fstorage_t const *fs, uint32_t base_address, packet_index_t *packet)
{
    const uint32_t size = 1;
    const uint32_t number = PACKET_INDEX_LEN/size;
    uint32_t offset;
    uint8_t tmp[4];

    switch (packet->cur_state) {
    case ROM_STATE_IDLE:
        if (packet->next_write_offset >= (base_address + (size * number))) {
            packet->cur_state = ROM_STATE_ERASE_START;
        } else if (packet->next_write_offset == base_address) {
            nrf_fstorage_read(fs, base_address, tmp, 2);
            if ((tmp[0] != 0xFF) || (tmp[1] != 0xFF))
                packet->cur_state = ROM_STATE_ERASE_START;
            else
                packet->cur_state = ROM_STATE_WRITE_START;
        } else {
            packet->cur_state = ROM_STATE_WRITE_START;
        }
        break;
    case ROM_STATE_ERASE_START:
        if (nrf_fstorage_is_busy(fs) == false) {
            nrf_fstorage_erase(fs, base_address, 1, NULL);
            packet->cur_state = ROM_STATE_ERASE_WAIT;
        }
        break;
    case ROM_STATE_ERASE_WAIT:
        if (nrf_fstorage_is_busy(fs) == false) {
            packet->cur_state = ROM_STATE_WRITE_START;
            packet->next_write_offset = base_address;
        }
        break;
    case ROM_STATE_WRITE_START:
        if (nrf_fstorage_is_busy(fs) == false) {
            memset(tmp, 0xFF, sizeof(tmp));

            /* 4 bytes align */
            offset = packet->next_write_offset & 0x03;
            if (offset != 0)
                nrf_fstorage_read(fs, packet->next_write_offset - offset, tmp, offset);
            tmp[offset] = packet->index;
            
            nrf_fstorage_write(fs, packet->next_write_offset - offset, tmp, 4, NULL);
            packet->cur_state = ROM_STATE_WRITE_WAIT;
        }
        break;
    case ROM_STATE_WRITE_WAIT:
        if (nrf_fstorage_is_busy(fs) == false) {
            /* write finished */
            packet->mission = ROM_MISSION_NO;
            packet->cur_state = ROM_STATE_IDLE;
            packet->next_write_offset += size;
        }
        break;
    case ROM_STATE_WAIT:
    default:
        break;
    }
}

void write_light_param(nrf_fstorage_t const *fs, uint32_t base_address, light_param_t *light)
{
    const uint32_t size = 6;
    const uint32_t number = LIGHT_PARAM_LEN/size;
    uint32_t offset;
    uint8_t tmp[8];

    switch (light->cur_state) {
    case ROM_STATE_IDLE:
        if (light->next_write_offset >= (base_address + (size * number))) {
            light->cur_state = ROM_STATE_ERASE_START;
        } else if (light->next_write_offset == base_address) {
            nrf_fstorage_read(fs, base_address, tmp, 2);
            if ((tmp[0] != 0xFF) || (tmp[1] != 0xFF))
                light->cur_state = ROM_STATE_ERASE_START;
            else
                light->cur_state = ROM_STATE_WRITE_START;
        } else {
            light->cur_state = ROM_STATE_WRITE_START;
        }
        break;
    case ROM_STATE_ERASE_START:
        if (nrf_fstorage_is_busy(fs) == false) {
            nrf_fstorage_erase(fs, base_address, 1, NULL);
            light->cur_state = ROM_STATE_ERASE_WAIT;
        }
        break;
    case ROM_STATE_ERASE_WAIT:
        if (nrf_fstorage_is_busy(fs) == false) {
            light->cur_state = ROM_STATE_WRITE_START;
            light->next_write_offset = base_address;
        }
        break;
    case ROM_STATE_WRITE_START:
        if (nrf_fstorage_is_busy(fs) == false) {
            memset(tmp, 0xFF, sizeof(tmp));

            /* 4 bytes align */
            offset = light->next_write_offset & 0x03;
            if (offset != 0)
                nrf_fstorage_read(fs, light->next_write_offset - offset, tmp, offset);
            tmp[offset + 0] = (light->param.swtch >> 0) & 0xFF;
            tmp[offset + 1] = (light->param.swtch >> 8) & 0xFF;
            tmp[offset + 2] = (light->param.lightness >> 0) & 0xFF;
            tmp[offset + 3] = (light->param.lightness >> 8) & 0xFF;
            tmp[offset + 4] = (light->param.lighttemp >> 0) & 0xFF;
            tmp[offset + 5] = (light->param.lighttemp >> 8) & 0xFF;
            
            nrf_fstorage_write(fs, light->next_write_offset - offset, tmp, 8, NULL);
            light->cur_state = ROM_STATE_WRITE_WAIT;
        }
        break;
    case ROM_STATE_WRITE_WAIT:
        if (nrf_fstorage_is_busy(fs) == false) {
            /* write finished */
            light->mission = ROM_MISSION_NO;
            light->cur_state = ROM_STATE_IDLE;
            light->next_write_offset += size;
        }
        break;
    case ROM_STATE_WAIT:
    default:
        break;
    }
}

void selfmesh_store_index(uint8_t index)
{
    packet_index.mission = ROM_MISSION_YES;
    packet_index.cur_state = ROM_STATE_IDLE;

    packet_index.index = index;
    return;
}

void selfmesh_store_light(uint16_t swtch, uint16_t lightness, uint16_t lighttemp)
{
    light_param.mission = ROM_MISSION_YES;
    light_param.cur_state = ROM_STATE_IDLE;

    light_param.param.swtch = swtch;
    light_param.param.lightness = lightness;
    light_param.param.lighttemp = lighttemp;
    return;
}

int selfmesh_open_proxy_channel(void)
{
    if(s_selfmesh_in_config != 1)
    {
        s_selfmesh_in_proxy_relay = 1;
        s_selfmesh_channel |= SELFMESH_CHANNEL_PBLE;
        advertising_start();
    }
    return 0;
}

int selfmesh_open_config_channel(void)
{
    s_selfmesh_in_config = 1;
    s_selfmesh_channel |= SELFMESH_CHANNEL_PBLE;
    //advertising_init();
    advertising_start();
    return 0;
}

int selfmesh_open_normal_channel(void)
{
    s_selfmesh_channel |= SELFMESH_CHANNEL_ADV;
    return 0;
}

int selfmesh_output_stack_log(char* log)
{
    NRF_LOG_INFO("%s", NRF_LOG_PUSH(log));
    return 0;
}

void selfmesh_reset(void)
{
    NVIC_SystemReset();
}

int selfmesh_send_net_msg(uint8_t* msg_ptr, uint8_t msg_len, uint8_t is_retrain_msg)
{
    uint8_t temp_mtx_try_cnt = 0;
    if((s_selfmesh_ud_out_buf.rcv_index+1)%SELFMESH_FOR_PROCESS_CNT != s_selfmesh_ud_out_buf.process_index)
    {
        memcpy(s_selfmesh_ud_out_buf.buf_ud[s_selfmesh_ud_out_buf.rcv_index].ud_buf, msg_ptr, msg_len);
        s_selfmesh_ud_out_buf.buf_ud[s_selfmesh_ud_out_buf.rcv_index].is_retrain_message = is_retrain_msg;
        s_selfmesh_ud_out_buf.rcv_index = (s_selfmesh_ud_out_buf.rcv_index+1)%SELFMESH_FOR_PROCESS_CNT;
    }
    return 0;
}

//modify for selfmesh end

/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    UNUSED_RETURN_VALUE(NRF_LOG_PROCESS());
    nrf_pwr_mgmt_run();
}



static void s_selfmesh_pdu_snd_timer_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    uint32_t       err_code;
    uint8_t data_out = 0;
    uint8_t temp_out_buf[SELFMESH_UD_BUF_SIZE] = {0};
    uint8_t temp_out_is_retrain_msg = 0;
    uint16_t temp_out_len = SELFMESH_UD_BUF_SIZE;
    if(s_selfmesh_ud_out_buf.process_index != s_selfmesh_ud_out_buf.rcv_index)
    {
        memcpy(temp_out_buf, (uint8_t*)(s_selfmesh_ud_out_buf.buf_ud[s_selfmesh_ud_out_buf.process_index].ud_buf), SELFMESH_UD_BUF_SIZE);
        temp_out_is_retrain_msg = s_selfmesh_ud_out_buf.buf_ud[s_selfmesh_ud_out_buf.process_index].is_retrain_message;
        s_selfmesh_ud_out_buf.process_index = (s_selfmesh_ud_out_buf.process_index+1)%SELFMESH_FOR_PROCESS_CNT;
        data_out = 1;
    }
    if(data_out)
    {
        if((s_selfmesh_channel&SELFMESH_CHANNEL_PBLE)!=0)
        {
            //NRF_LOG_INFO("Send data via ble peripheral.");
            do
            {
                err_code = ble_nus_data_send(&m_nus, temp_out_buf, &temp_out_len, m_conn_handle);
                if ((err_code != NRF_ERROR_INVALID_STATE) &&
                        (err_code != NRF_ERROR_RESOURCES) &&
                        (err_code != NRF_ERROR_NOT_FOUND) )
                {
                    APP_ERROR_CHECK(err_code);
                }
            } while (err_code == NRF_ERROR_RESOURCES);
        }
        if((s_selfmesh_channel&SELFMESH_CHANNEL_ADV)!=0)
        {
            //NRF_LOG_INFO("Send data via ble adv.");
            mesh_rawdata_send_via_adv(temp_out_buf, temp_out_len);
        }
    }

}

void ble_module_schedule(void)
{
    const uint32_t size = sizeof(selfmesh_node_t) - 2;
    const uint32_t number = USER_DATA_LEN/size;
    uint8_t data_in = 0;
    uint8_t temp_in_buf[SELFMESH_UD_BUF_SIZE] = {0};

    s_selfmess_handle_cache();
    if(s_selfmesh_ud_in_buf.process_index != s_selfmesh_ud_in_buf.rcv_index)
    {
        memcpy(temp_in_buf, (uint8_t*)(s_selfmesh_ud_in_buf.buf_ud[s_selfmesh_ud_in_buf.process_index].ud_buf), SELFMESH_UD_BUF_SIZE);
        s_selfmesh_ud_in_buf.process_index = (s_selfmesh_ud_in_buf.process_index+1)%SELFMESH_FOR_PROCESS_CNT;
        data_in = 1;
    }


    if(data_in)
    {
        if(2!=selfmesh_node_handle(temp_in_buf, &s_selfmesh_node))
        {
            //note: retrain the same message via PBLE and CBLE
            selfmesh_util_send_msg_next_hop(temp_in_buf);
        }
    }

    /* hardware information store */
    if (main_config.mission == ROM_MISSION_YES) {
        write_hw_info(&fstorage, USER_DATA_ADDR, &main_config);

        if (main_config.mission == ROM_MISSION_NO) {
            /* copy data, so later will write to another rom address */
            back_config.mission = ROM_MISSION_YES;
            back_config.cur_state = ROM_STATE_IDLE;
            memcpy(&back_config.node, &main_config.node, sizeof(selfmesh_node_t));
            back_config.next_write_offset = main_config.next_write_offset + 0x1000 - size;
        }
    }

    if (back_config.mission == ROM_MISSION_YES) {
        write_hw_info(&fstorage, BACK_DATA_ADDR, &back_config);

        if (back_config.mission == ROM_MISSION_NO) {
            if (main_config.cur_state == ROM_STATE_WAIT)
                main_config.cur_state = ROM_STATE_IDLE;
        }
    }

    if ((main_config.mission == ROM_MISSION_NO)
        && (back_config.mission == ROM_MISSION_NO)
        && (packet_index.mission == ROM_MISSION_YES)) {
        write_packet_index(&fstorage, PACKET_INDEX_ADDR, &packet_index);
    }

    if ((main_config.mission == ROM_MISSION_NO)
        && (back_config.mission == ROM_MISSION_NO)
        && (packet_index.mission == ROM_MISSION_NO)
        && (light_param.mission == ROM_MISSION_YES)) {
        write_light_param(&fstorage, LIGHT_PARAM_ADDR, &light_param);
    }

    selfmesh_node_sche(&s_selfmesh_node);
}


/**@brief Application main function.
 */
int main(void)
{
    bool erase_bonds;
    ret_code_t rc;
    uint32_t s_time = 0;
    nrf_fstorage_api_t * p_fs_api;
    /* Initialize an fstorage instance using the nrf_fstorage_sd backend.
     * nrf_fstorage_sd uses the SoftDevice to write to flash. This implementation can safely be
     * used whenever there is a SoftDevice, regardless of its status (enabled/disabled). */
    p_fs_api = &nrf_fstorage_sd;

    // Initialize.
    //uart_init();
    log_init();
    timers_init();
    //buttons_leds_init(&erase_bonds);
//modify for selfmesh begin
    module_board_init();
//modify for selfmesh end
    power_management_init();
    ble_stack_init();
    gap_params_init();
    gatt_init();
    services_init();
    advertising_init();
    conn_params_init();
    mesh_main_initialize(s_selfmesh_pdu_rx);
    rand_prng_seed(&s_selfmesh_main_prng);
    s_time = rand_prng_get(&s_selfmesh_main_prng) % (SELFMESH_PDU_SEND_INTERVAL);
    while(s_time)
    {
        nrf_delay_ms(1);
        s_time--;
    }

    // Start execution.
    NRF_LOG_INFO("Selfmesh lightctrl server started.");
    mesh_main_start();
    app_timer_create(&m_selfmesh_pdu_snd_tmr, APP_TIMER_MODE_REPEATED, s_selfmesh_pdu_snd_timer_handler);
    app_timer_start(m_selfmesh_pdu_snd_tmr, APP_TIMER_TICKS(SELFMESH_PDU_SEND_INTERVAL), NULL);
    rc = nrf_fstorage_init(&fstorage, p_fs_api, NULL);
    APP_ERROR_CHECK(rc);
//modify for selfmesh begin
    memset(&s_selfmesh_ud_in_buf, 0x00, sizeof(selfmesh_ud_process_t));
    memset(&s_selfmesh_ud_out_buf, 0x00, sizeof(selfmesh_ud_process_t));
    s_Adv_channel_busy = 0;
    //selfmesh_node_config_default(&s_selfmesh_node);
    read_hw_info(&fstorage, &main_config, &back_config);
    read_packet_index(&fstorage, &packet_index);
    read_light_param(&fstorage, &light_param);

    memcpy(&s_selfmesh_node, &main_config.node, sizeof(main_config.node));
    s_selfmesh_node.snd_indx = packet_index.index;
    selfmesh_node_init(&s_selfmesh_node);
    //s_selfmesh_test();
//modify for selfmesh end

    // Enter main loop.
    for (;;)
    {
        ble_module_schedule();
        idle_state_handle();
    }
}


/**
 * @}
 */
