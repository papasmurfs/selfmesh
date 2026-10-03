/**
 * Copyright (c) 2017 - 2019, DFT Tech Co.,Ltd
 *
 * All rights reserved.
 *
 *
 */ 

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "selfmesh_service.h"
#include "selfmesh_service_inter.h"
#include "selfmesh_util.h"
#include "selfmesh_tag.h"
#include "app_timer.h"

typedef struct
{
    uint16_t key_press;
    uint16_t press_time;
}selfmesh_key_handle_t;

static volatile uint16_t s_current_time = 0x00;

static volatile uint16_t s_msg_send_time = 0x00;
static volatile uint16_t s_msg_cool_time = 0x00;
static volatile uint16_t s_msg_can_send  = 0x01;

static volatile selfmesh_key_handle_t s_selfmesh_key_handle[SELFMESH_MAX_ELEMENT_CNT] = {0};

#define SELFMESH_TICK_MS            10

#define SELFMESH_LONG_PRESS_CNT     (500/SELFMESH_TICK_MS)

APP_TIMER_DEF(s_T0_tick_timer_id);  /**< tick. */
#define T0_TICK_INTERVAL        APP_TIMER_TICKS(SELFMESH_TICK_MS)

//NOTE: the time per hop is adv duration plus scan duration
#define SELFMESH_TICK_PER_HOP   ((120+120+20)/SELFMESH_TICK_MS)

extern int selfmesh_is_key_pressed(uint8_t key);

static void tick_T0_timeout_handler(void * p_context)
{
    uint32_t err_code;
    uint8_t temp_work_mode = (uint8_t)(p_context);
    s_current_time += 1;
    app_timer_start(s_T0_tick_timer_id, T0_TICK_INTERVAL, NULL);
}

/**@brief Function of the service to handle the data from the net.
 *
 * @param[in]  start_mode       The system start mode.
 * @param[in]  element          The element id of the service.
 * @param[io]  cfg_set_ptr      The service config data.
 * @retval 0                    Init success.
 * @retval else                 Error.
 *
 */
/**@snippet [Handling the service init] */
int selfmesh_service_0000_init(uint8_t start_mode, uint8_t element, selfmesh_service_t* cfg_set_ptr)
{
    app_timer_create(&s_T0_tick_timer_id,
                      APP_TIMER_MODE_SINGLE_SHOT,
                      tick_T0_timeout_handler);
    app_timer_start(s_T0_tick_timer_id, T0_TICK_INTERVAL, NULL);
    return 0;
}

/**@brief Function of the service to handle the data from the net.
 *
 * @param[in]  source_addr      The address of the node which send the command the the service.
 * @param[in]  element          The element id of the service.
 * @param[in]  data_ptr         The data get from the mesh net.
 * @param[out] out_ptr          The handle result of the service.
 * @param[io]  cfg_set_ptr      The service config data.
 * @retval 0                    The data is processed successfully by the service.
 * @retval 1                    The data can not be processed by the service.
 * @retval 2                    Error.
 *
 */
/**@snippet [Handling the data received from the net] */
int selfmesh_service_0000_handle(selfmesh_addr_t source_addr, uint8_t element, uint8_t* data_ptr, selfmesh_service_t* cfg_set_ptr)
{
    int ret_val = 1;
    uint8_t temp_app_buf[SELFMASH_APP_DATA_LEN] = {0x00};
    int temp_app_len = SELFMASH_APP_DATA_LEN;
    selfmesh_tag_t temp_tag;
    uint8_t temp_v_len;
    temp_tag = data_ptr[0];
    temp_tag = (temp_tag<<8)|data_ptr[1];
    temp_v_len = data_ptr[2];
    switch(temp_tag)
    {
        case SELFMESH_TAG_NODE_IDENTIFY:
        {
            if(temp_v_len == 1)//to identify the node device
            {
                //the parameter is data_ptr[3];
                //TODO: identify the device
                temp_app_len = 0;
                temp_app_buf[temp_app_len++] = (SELFMESH_TAG_NODE_IDENTIFY_RSP>>8)&0xFF;
                temp_app_buf[temp_app_len++] = (SELFMESH_TAG_NODE_IDENTIFY_RSP)&0xFF;
                temp_app_buf[temp_app_len++] = 1;//length is 1
                temp_app_buf[temp_app_len++] = 0;//operation result is success
                selfmesh_util_send_app_msg(source_addr, element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_app_buf, temp_app_len);
                ret_val = 0;
            }
            else
            {
                ret_val = 2;
            }
        } break;

        case SELFMESH_TAG_SIMPLE_CLIENT_SET:
        {
            if(temp_v_len > 0 && temp_v_len<SELFMESH_MAX_VALUE_PER_SERVICE-1)//to set the simple client config data
            {
                cfg_set_ptr->service_cfg[0] = temp_v_len;
                memcpy(cfg_set_ptr->service_cfg+1, data_ptr+3, temp_v_len);
                temp_app_len = 0;
                temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SIMPLE_CLIENT_SET_RSP>>8)&0xFF;
                temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SIMPLE_CLIENT_SET_RSP)&0xFF;
                temp_app_buf[temp_app_len++] = 1;//length is 4
                temp_app_buf[temp_app_len++] = 0;//operation result is success
                selfmesh_util_store_cfg((uint8_t *)&(cfg_set_ptr->service_cfg), SELFMESH_MAX_VALUE_PER_SERVICE, 1);
                selfmesh_util_send_app_msg(source_addr, element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_app_buf, temp_app_len);
                ret_val = 0;
            }
            else
            {
                ret_val = 2;
            }
        } break;

        case SELFMESH_TAG_SIMPLE_CLIENT_GET:
        {
            if(temp_v_len == 0x00)//to get the simple client config data
            {
                temp_app_len = 0;
                temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SIMPLE_CLIENT_GET_RSP>>8)&0xFF;
                temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SIMPLE_CLIENT_GET_RSP)&0xFF;
                temp_app_buf[temp_app_len++] = cfg_set_ptr->service_cfg[0];//length
                if(cfg_set_ptr->service_cfg[0] > SELFMASH_APP_DATA_LEN-3)
                {
                    ret_val = 2;
                }
                else
                {
                    memcpy(temp_app_buf+temp_app_len, cfg_set_ptr->service_cfg+1, cfg_set_ptr->service_cfg[0]);
                    temp_app_len += cfg_set_ptr->service_cfg[0];
                }
                selfmesh_util_send_app_msg(source_addr, element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_app_buf, temp_app_len);
                ret_val = 0;
            }
            else
            {
                ret_val = 2;
            }
        } break;

        default:
        break;
    }
    return 0;
}

/**@brief Function for service schedule.
 *
 * @param[in]  element          The element id of the service.
 * @param[io]  cfg_set_ptr      The service config data.
 * @retval 
 *
 */
/**@snippet [Handling the service schedule] */
void selfmesh_service_0000_sche(uint8_t element, selfmesh_service_t* cfg_set_ptr)
{
    uint8_t temp_app_buf[SELFMASH_APP_DATA_LEN] = {0x00};
    int temp_app_len = SELFMASH_APP_DATA_LEN;
    int temp_chk_cnt = 0;
    int temp_current_cnt = 0;
    uint16_t cur_key = 0xFF;
    if(cfg_set_ptr->service_cfg[0] == 0 ||  cfg_set_ptr->service_cfg[0] > SELFMASH_APP_DATA_LEN+1)
    {
        return;
    }
    if(s_msg_can_send == 0)
    {
        if(s_current_time >= s_msg_send_time)
        {
            if(s_current_time - s_msg_send_time >= s_msg_cool_time)
            {
                s_msg_can_send = 1;
            }
            else
            {
                return;
            }
        }
        else
        {
            if(s_current_time + 0x10000 - s_msg_send_time >= s_msg_cool_time)
            {
                s_msg_can_send = 1;
            }
            else
            {
                return;
            }
        }
    }
    
    if(selfmesh_is_key_pressed(element+1) == 1)
    {
        cur_key = element+1;
        if(s_selfmesh_key_handle[element].key_press == 0)
        {
            s_selfmesh_key_handle[element].key_press = 1;
            s_selfmesh_key_handle[element].press_time = s_current_time;
            temp_app_len = 0;
            memcpy(temp_app_buf, cfg_set_ptr->service_cfg+2, cfg_set_ptr->service_cfg[0] - 1);
            temp_app_len += cfg_set_ptr->service_cfg[0] - 1;
            s_msg_can_send = 0;
            s_msg_send_time = s_current_time;
            s_msg_cool_time = SELFMESH_MSG_DEFAULT_HOP*SELFMESH_TICK_PER_HOP;//note:the delay time should not exceed 2 hop
            selfmesh_util_send_app_msg(SELFMESH_INVALIDE_ADDRESS, element, cfg_set_ptr->service_cfg[1], SELFMESH_MSG_DEFAULT_HOP, temp_app_buf, temp_app_len);
        }
        else
        {
            temp_chk_cnt = s_selfmesh_key_handle[element].press_time;
            temp_current_cnt = s_current_time;
            if(temp_current_cnt >= temp_chk_cnt)
            {
                if(temp_current_cnt - temp_chk_cnt >= SELFMESH_LONG_PRESS_CNT)
                {
                    s_selfmesh_key_handle[element].press_time = s_current_time;
                    temp_app_len = 0;
                    memcpy(temp_app_buf, cfg_set_ptr->service_cfg+2, cfg_set_ptr->service_cfg[0] - 1);
                    temp_app_len += cfg_set_ptr->service_cfg[0] - 1;
                    s_msg_can_send = 0;
                    s_msg_send_time = s_current_time;
                    s_msg_cool_time = SELFMESH_MSG_DEFAULT_HOP*SELFMESH_TICK_PER_HOP;//note:the delay time should not exceed 2 hop
                    selfmesh_util_send_app_msg(SELFMESH_INVALIDE_ADDRESS, element, cfg_set_ptr->service_cfg[1], SELFMESH_MSG_DEFAULT_HOP, temp_app_buf, temp_app_len);
                }
            }
            else
            {
                if(temp_current_cnt + 0x10000 - temp_chk_cnt >= SELFMESH_LONG_PRESS_CNT)
                {
                    s_selfmesh_key_handle[element].press_time = s_current_time;
                    temp_app_len = 0;
                    memcpy(temp_app_buf, cfg_set_ptr->service_cfg+2, cfg_set_ptr->service_cfg[0] - 1);
                    temp_app_len += cfg_set_ptr->service_cfg[0] - 1;
                    s_msg_can_send = 0;
                    s_msg_send_time = s_current_time;
                    s_msg_cool_time = SELFMESH_MSG_DEFAULT_HOP*SELFMESH_TICK_PER_HOP;//note:the delay time should not exceed 2 hop
                    selfmesh_util_send_app_msg(SELFMESH_INVALIDE_ADDRESS, element, cfg_set_ptr->service_cfg[1], SELFMESH_MSG_DEFAULT_HOP, temp_app_buf, temp_app_len);
                }
            }
        }
    }
    else
    {
        s_selfmesh_key_handle[element].key_press = 0;
        s_selfmesh_key_handle[element].press_time = 0;
        return;
    }

}


