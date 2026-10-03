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
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "boards.h"
#include "nrf_gpio.h"
#include "lightdrv.h"

typedef struct
{
    uint16_t scene_id;
    uint16_t lightness;
    uint16_t lighttemp;
}selfmesh_scene_setup_lightctrl_server_t;

typedef struct
{
    uint16_t cur_switch;
    uint16_t cur_lightness;
    uint16_t cur_lighttemp;
    selfmesh_scene_setup_lightctrl_server_t scene_setup[SELFMESH_SCENE_MAX_CNT];
}selfmesh_light_ctrl_t;

typedef struct
{
    uint16_t key_press;
    uint16_t press_time;
}selfmesh_light_ctr_t;

static volatile uint16_t s_current_time = 0x00;


static volatile selfmesh_light_ctr_t s_selfmesh_key_handle[SELFMESH_MAX_ELEMENT_CNT] = {0};

#define SELFMESH_TICK_MS            10

#define SELFMESH_LONG_PRESS_CNT     (200/SELFMESH_TICK_MS)

APP_TIMER_DEF(s_T1_tick_timer_id);  /**< tick. */
#define T1_TICK_INTERVAL        APP_TIMER_TICKS(SELFMESH_TICK_MS)

//NOTE: the time per hop is adv duration plus scan duration
#define SELFMESH_TICK_PER_HOP   ((120+120+20)/SELFMESH_TICK_MS)

extern int selfmesh_is_key_pressed(uint8_t key);


#define SELFMESH_SCENE_ID_INVALID                  0xFFFF
#define SELFMESH_LIGHTCTRL_LIGHTNESS_INVALIDE      0xFFFF
#define SELFMESH_LIGHTCTRL_LIGHTTEMP_INVALIDE      0xFFFF
#define SELFMESH_LIGHTCTRL_LIGHTNESS_MAX           1000

#define SELFMESH_LIGHTCTRL_LIGHTNESS_STEP            100
#define SELFMESH_LIGHTCTRL_LIGHTNESS_STEP_S          50
#define SELFMESH_LIGHTCTRL_LIGHTNESS_STEP_L          30



#define SELFMESH_LIGHTCTRL_LIGHTNESS_UP            0
#define SELFMESH_LIGHTCTRL_LIGHTNESS_DOWN          1
#define SELFMESH_LIGHTCTRL_LIGHTSWITCH             2
#define SELFMESH_LIGHTCTRL_LIGHTNESS_ON            3
#define SELFMESH_LIGHTCTRL_LIGHTNESS_OFF           4
#define SELFMESH_LIGHTCTRL_LIGHTNESS_SET           5
static volatile selfmesh_light_ctrl_t* s_light_ctrl_cfg = NULL;


static void tick_T1_timeout_handler(void * p_context)
{
    uint32_t err_code;
    uint8_t temp_work_mode = (uint8_t)(p_context);
    s_current_time += 1;
    app_timer_start(s_T1_tick_timer_id, T1_TICK_INTERVAL, NULL);
}

static void selfmesh_service_timer_init(void)
{

    app_timer_create(&s_T1_tick_timer_id,
                    APP_TIMER_MODE_SINGLE_SHOT,
                    tick_T1_timeout_handler);
    app_timer_start(s_T1_tick_timer_id, T1_TICK_INTERVAL, NULL);
    return ;

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
int selfmesh_service_0001_init(uint8_t start_mode, uint8_t element, selfmesh_service_t* cfg_set_ptr)
{
    selfmesh_light_t param;

    s_light_ctrl_cfg = (selfmesh_light_ctrl_t*)cfg_set_ptr->service_cfg;
    
    read_rom_light_param(&param);
    s_light_ctrl_cfg->cur_switch = param.swtch;
    s_light_ctrl_cfg->cur_lightness = param.lightness;
    s_light_ctrl_cfg->cur_lighttemp = param.lighttemp;
    
    lightdrv_init();
    if(s_light_ctrl_cfg->cur_switch == 0)
    {
        lightdrv_lightness_set(0);
    }
    else if(s_light_ctrl_cfg->cur_switch == 1)
    {
        lightdrv_lightness_set(s_light_ctrl_cfg->cur_lightness);
    }
    else
    {
        lightdrv_lightness_set(0);
    }
    lightdrv_start();
    selfmesh_service_timer_init();
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
int selfmesh_service_0001_handle(selfmesh_addr_t source_addr, uint8_t element, uint8_t* data_ptr, selfmesh_service_t* cfg_set_ptr)
{
    int ret_val = 1;
    uint8_t temp_app_buf[SELFMASH_APP_DATA_LEN] = {0x00};
    int temp_app_len = SELFMASH_APP_DATA_LEN;
    selfmesh_tag_t temp_tag;
    uint8_t temp_v_len;
    temp_tag = data_ptr[0];
    temp_tag = (temp_tag<<8)|data_ptr[1];
    temp_v_len = data_ptr[2];
    uint16_t temp_row_index = 0;
    uint16_t temp_scene_id = 0;
    uint16_t temp_lightness = 0;
    uint16_t temp_lighttemp = 0;
    uint8_t temp_light_cmd = 0;
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

        case SELFMESH_TAG_SCENE_LIST_CNT_GET:
        {
            if(temp_v_len == 0x00)//to get the simple client config data
            {
                for(temp_row_index = 0; temp_row_index<SELFMESH_SCENE_MAX_CNT;temp_row_index++)
                {
                    if(s_light_ctrl_cfg->scene_setup[temp_row_index].scene_id == SELFMESH_SCENE_ID_INVALID)
                    {
                        break;
                    }
                }
                temp_app_len = 0;
                temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_LIST_CNT_GET_RSP>>8)&0xFF;
                temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_LIST_CNT_GET_RSP)&0xFF;
                temp_app_buf[temp_app_len++] = 3;//length
                temp_app_buf[temp_app_len++] = 0;//operation success
                temp_app_buf[temp_app_len++] = temp_row_index;//current valid scene count
                temp_app_buf[temp_app_len++] = SELFMESH_SCENE_MAX_CNT;//max length
                selfmesh_util_send_app_msg(source_addr, element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_app_buf, temp_app_len);
                ret_val = 0;
            }
            else
            {
                ret_val = 2;
            }
        } break;

        case SELFMESH_TAG_SCENE_LIST_GET:
        {
            if(temp_v_len == 1)//to get the scene of the scene list
            {
                if(data_ptr[3] >= SELFMESH_SCENE_MAX_CNT)
                {
                    temp_app_len = 0;
                    temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_LIST_GET_RSP>>8)&0xFF;
                    temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_LIST_GET_RSP)&0xFF;
                    temp_app_buf[temp_app_len++] = 1;//length is 1
                    temp_app_buf[temp_app_len++] = 1;//operation result is fail, overflow
                    selfmesh_util_send_app_msg(source_addr, element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_app_buf, temp_app_len);
                    ret_val = 0;
                }
                else
                {
                    temp_scene_id = s_light_ctrl_cfg->scene_setup[data_ptr[3]].scene_id;
                    if(temp_scene_id == SELFMESH_SCENE_ID_INVALID)
                    {
                        temp_app_len = 0;
                        temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_LIST_GET_RSP>>8)&0xFF;
                        temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_LIST_GET_RSP)&0xFF;
                        temp_app_buf[temp_app_len++] = 1;//length is 1
                        temp_app_buf[temp_app_len++] = 2;//operation result is fail, invalid scene id
                        selfmesh_util_send_app_msg(source_addr, element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_app_buf, temp_app_len);
                        ret_val = 0;
                    }
                    else
                    {
                        temp_lightness = s_light_ctrl_cfg->scene_setup[data_ptr[3]].lightness;
                        temp_lighttemp = s_light_ctrl_cfg->scene_setup[data_ptr[3]].lighttemp;
                        temp_app_len = 0;
                        temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_LIST_GET_RSP>>8)&0xFF;
                        temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_LIST_GET_RSP)&0xFF;
                        temp_app_buf[temp_app_len++] = 7;//length is 7
                        temp_app_buf[temp_app_len++] = 0;//operation result is success
                        temp_app_buf[temp_app_len++] = (temp_scene_id>>8)&0xFF;
                        temp_app_buf[temp_app_len++] = (temp_scene_id)&0xFF;
                        temp_app_buf[temp_app_len++] = (temp_lightness>>8)&0xFF;
                        temp_app_buf[temp_app_len++] = (temp_lightness)&0xFF;
                        temp_app_buf[temp_app_len++] = (temp_lighttemp>>8)&0xFF;
                        temp_app_buf[temp_app_len++] = (temp_lighttemp)&0xFF;
                        selfmesh_util_send_app_msg(source_addr, element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_app_buf, temp_app_len);
                        ret_val = 0;
                    }
                }
            }
            else
            {
                ret_val = 2;
            }
        } break;

        case SELFMESH_TAG_SCENE_REMOVE:
        {
            if(temp_v_len == 2)//to get the scene id of the scene table row
            {
                temp_scene_id = data_ptr[3];
                temp_scene_id = temp_scene_id<<8;
                temp_scene_id = temp_scene_id|data_ptr[4];
                if(temp_scene_id == SELFMESH_SCENE_ID_INVALID)
                {
                    temp_app_len = 0;
                    temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_REMOVE_RSP>>8)&0xFF;
                    temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_REMOVE_RSP)&0xFF;
                    temp_app_buf[temp_app_len++] = 1;//length is 1
                    temp_app_buf[temp_app_len++] = 1;//operation result is fail, invalide scene id
                    selfmesh_util_send_app_msg(source_addr, element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_app_buf, temp_app_len);
                    ret_val = 0;
                }
                else
                {
                    for(temp_row_index = 0; temp_row_index<SELFMESH_SCENE_MAX_CNT;temp_row_index++)
                    {
                        if(s_light_ctrl_cfg->scene_setup[temp_row_index].scene_id == temp_scene_id)
                        {
                            break;
                        }
                    }
                    if(temp_row_index == SELFMESH_SCENE_MAX_CNT)
                    {
                        temp_app_len = 0;
                        temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_REMOVE_RSP>>8)&0xFF;
                        temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_REMOVE_RSP)&0xFF;
                        temp_app_buf[temp_app_len++] = 1;//length is 1
                        temp_app_buf[temp_app_len++] = 2;//operation result is fail, the scene id is not in the list
                        selfmesh_util_send_app_msg(source_addr, element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_app_buf, temp_app_len);
                        ret_val = 0;
                    }
                    else
                    {
                        memcpy(&(s_light_ctrl_cfg->scene_setup[temp_row_index]),
                               (uint8_t*)(&(s_light_ctrl_cfg->scene_setup[temp_row_index]))+sizeof(selfmesh_scene_setup_lightctrl_server_t),
                               (SELFMESH_SCENE_MAX_CNT-temp_row_index-1)*sizeof(selfmesh_scene_setup_lightctrl_server_t));
                        s_light_ctrl_cfg->scene_setup[SELFMESH_SCENE_MAX_CNT-1].scene_id = SELFMESH_SCENE_ID_INVALID;
                        s_light_ctrl_cfg->scene_setup[SELFMESH_SCENE_MAX_CNT-1].lightness = SELFMESH_LIGHTCTRL_LIGHTNESS_INVALIDE;
                        s_light_ctrl_cfg->scene_setup[SELFMESH_SCENE_MAX_CNT-1].lighttemp = SELFMESH_LIGHTCTRL_LIGHTTEMP_INVALIDE;
                        temp_app_len = 0;
                        temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_REMOVE_RSP>>8)&0xFF;
                        temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_REMOVE_RSP)&0xFF;
                        temp_app_buf[temp_app_len++] = 1;//length is 1
                        temp_app_buf[temp_app_len++] = 0;//operation result is success
                        selfmesh_util_store_cfg((uint8_t *)&(cfg_set_ptr->service_cfg), SELFMESH_MAX_VALUE_PER_SERVICE, 1);
                        selfmesh_util_send_app_msg(source_addr, element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_app_buf, temp_app_len);
                        ret_val = 0;
                    }
                }
            }
            else
            {
                ret_val = 2;
            }
        } break;

        case SELFMESH_TAG_SCENE_PARAMETER_SET:
        {
            if(temp_v_len == 6)//to set the scene parameter
            {
                temp_scene_id = data_ptr[3];
                temp_scene_id = temp_scene_id<<8;
                temp_scene_id = temp_scene_id|data_ptr[4];
                if(temp_scene_id == SELFMESH_SCENE_ID_INVALID)
                {
                    temp_app_len = 0;
                    temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_PARAMETER_SET_RSP>>8)&0xFF;
                    temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_PARAMETER_SET_RSP)&0xFF;
                    temp_app_buf[temp_app_len++] = 1;//length is 1
                    temp_app_buf[temp_app_len++] = 1;//operation result is fail, invalide scene id
                    selfmesh_util_send_app_msg(source_addr, element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_app_buf, temp_app_len);
                    ret_val = 0;
                }
                else
                {
                    for(temp_row_index = 0; temp_row_index<SELFMESH_SCENE_MAX_CNT;temp_row_index++)
                    {
                        if(s_light_ctrl_cfg->scene_setup[temp_row_index].scene_id == temp_scene_id
                           ||s_light_ctrl_cfg->scene_setup[temp_row_index].scene_id == SELFMESH_SCENE_ID_INVALID)
                        {
                            break;
                        }
                    }
                    if(temp_row_index == SELFMESH_SCENE_MAX_CNT)
                    {
                        temp_app_len = 0;
                        temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_PARAMETER_SET_RSP>>8)&0xFF;
                        temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_PARAMETER_SET_RSP)&0xFF;
                        temp_app_buf[temp_app_len++] = 1;//length is 1
                        temp_app_buf[temp_app_len++] = 2;//operation result is fail, table overflow
                        selfmesh_util_send_app_msg(source_addr, element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_app_buf, temp_app_len);
                        ret_val = 0;
                    }
                    else
                    {
                        temp_lightness = data_ptr[5];
                        temp_lightness = temp_lightness<<8;
                        temp_lightness = temp_lightness|data_ptr[6];
                        temp_lighttemp = data_ptr[7];
                        temp_lighttemp = temp_lighttemp<<8;
                        temp_lighttemp = temp_lighttemp|data_ptr[8];
                        s_light_ctrl_cfg->scene_setup[temp_row_index].scene_id = temp_scene_id;
                        s_light_ctrl_cfg->scene_setup[temp_row_index].lightness = temp_lightness;
                        s_light_ctrl_cfg->scene_setup[temp_row_index].lighttemp = temp_lighttemp;
                        temp_app_len = 0;
                        temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_PARAMETER_SET_RSP>>8)&0xFF;
                        temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_PARAMETER_SET_RSP)&0xFF;
                        temp_app_buf[temp_app_len++] = 2;//length is 1
                        temp_app_buf[temp_app_len++] = 0;//operation result is success
                        temp_app_buf[temp_app_len++] = temp_row_index;//row index
                        selfmesh_util_store_cfg((uint8_t *)&(cfg_set_ptr->service_cfg), SELFMESH_MAX_VALUE_PER_SERVICE, 1);
                        selfmesh_util_send_app_msg(source_addr, element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_app_buf, temp_app_len);
                        ret_val = 0;
                    }
                }
            }
            else
            {
                ret_val = 2;
            }
        } break;

        case SELFMESH_TAG_SCENE_PARAMETER_GET:
        {
            if(temp_v_len == 2)//to get the scene parameter
            {
                temp_scene_id = data_ptr[3];
                temp_scene_id = temp_scene_id<<8;
                temp_scene_id = temp_scene_id|data_ptr[4];
                if(temp_scene_id == SELFMESH_SCENE_ID_INVALID)
                {
                    temp_app_len = 0;
                    temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_PARAMETER_GET_RSP>>8)&0xFF;
                    temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_PARAMETER_GET_RSP)&0xFF;
                    temp_app_buf[temp_app_len++] = 1;//length is 1
                    temp_app_buf[temp_app_len++] = 1;//operation result is fail, invalide scene id
                    selfmesh_util_send_app_msg(source_addr, element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_app_buf, temp_app_len);
                    ret_val = 0;
                }
                else
                {
                    for(temp_row_index = 0; temp_row_index<SELFMESH_SCENE_MAX_CNT;temp_row_index++)
                    {
                        if(s_light_ctrl_cfg->scene_setup[temp_row_index].scene_id == temp_scene_id)
                        {
                            break;
                        }
                    }
                    if(temp_row_index == SELFMESH_SCENE_MAX_CNT)
                    {
                        temp_app_len = 0;
                        temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_PARAMETER_GET_RSP>>8)&0xFF;
                        temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_PARAMETER_GET_RSP)&0xFF;
                        temp_app_buf[temp_app_len++] = 1;//length is 1
                        temp_app_buf[temp_app_len++] = 2;//operation result is fail, current scene not configed
                        selfmesh_util_send_app_msg(source_addr, element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_app_buf, temp_app_len);
                        ret_val = 0;
                    }
                    else
                    {
                        temp_app_len = 0;
                        temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_PARAMETER_GET_RSP>>8)&0xFF;
                        temp_app_buf[temp_app_len++] = (SELFMESH_TAG_SCENE_PARAMETER_GET_RSP)&0xFF;
                        temp_app_buf[temp_app_len++] = 5;//length is 1
                        temp_app_buf[temp_app_len++] = 0;//operation result is success
                        temp_app_buf[temp_app_len++] = (s_light_ctrl_cfg->scene_setup[temp_row_index].lightness>>8)&0xFF;//lightness
                        temp_app_buf[temp_app_len++] = (s_light_ctrl_cfg->scene_setup[temp_row_index].lightness)&0xFF;//lightness
                        temp_app_buf[temp_app_len++] = (s_light_ctrl_cfg->scene_setup[temp_row_index].lighttemp>>8)&0xFF;//lighttemp
                        temp_app_buf[temp_app_len++] = (s_light_ctrl_cfg->scene_setup[temp_row_index].lighttemp)&0xFF;//lighttemp
                        selfmesh_util_send_app_msg(source_addr, element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_app_buf, temp_app_len);
                        ret_val = 0;
                    }
                }
            }
            else
            {
                ret_val = 2;
            }
        } break;

        case SELFMESH_TAG_SCENE_PUBLISH_ID:
        {
            if(temp_v_len == 2)//get the scene id
            {
                static uint16_t temp_lightness = 500;
                temp_scene_id = data_ptr[3];
                temp_scene_id = temp_scene_id<<8;
                temp_scene_id = temp_scene_id|data_ptr[4];
                NRF_LOG_INFO("Get scene id:%d.",temp_scene_id);
                if(temp_scene_id == SELFMESH_SCENE_ID_INVALID)
                {
                    ret_val = 2;
                }
                else
                {
                    for(temp_row_index = 0; temp_row_index<SELFMESH_SCENE_MAX_CNT;temp_row_index++)
                    {
                        if(s_light_ctrl_cfg->scene_setup[temp_row_index].scene_id == temp_scene_id)
                        {
                            break;
                        }
                    }
                    if(temp_row_index == SELFMESH_SCENE_MAX_CNT)
                    {
                        ret_val = 2;
                    }
                    else
                    {
                        s_light_ctrl_cfg->cur_lightness = s_light_ctrl_cfg->scene_setup[temp_row_index].lightness;
                        s_light_ctrl_cfg->cur_lighttemp = s_light_ctrl_cfg->scene_setup[temp_row_index].lighttemp;
                        if(s_light_ctrl_cfg->cur_lightness != 0)
                        {
                            s_light_ctrl_cfg->cur_switch = 1;
                        }
                        else
                        {
                            s_light_ctrl_cfg->cur_switch = 0;
                        }
                        lightdrv_stop();
                        if(s_light_ctrl_cfg->cur_switch == 0)
                        {
                            lightdrv_lightness_set(0);
                        }
                        else
                        {
                            lightdrv_lightness_set(s_light_ctrl_cfg->cur_lightness);
                        }
                        lightdrv_start();
                        selfmesh_store_light(s_light_ctrl_cfg->cur_switch, s_light_ctrl_cfg->cur_lightness, s_light_ctrl_cfg->cur_lighttemp);
                        ret_val = 0;
                    }
                }
            }
            else
            {
                ret_val = 2;
            }
        } break;

        case SELFMESH_TAG_LIGHTNESS_PUBLISH:
        {
            if(temp_v_len == 1 || temp_v_len == 3 )//set lightness command
            {
                temp_light_cmd = data_ptr[3];
                if(temp_light_cmd == SELFMESH_LIGHTCTRL_LIGHTNESS_UP)
                {
                    if(s_light_ctrl_cfg->cur_lightness<SELFMESH_LIGHTCTRL_LIGHTNESS_MAX )
                    {
                        s_light_ctrl_cfg->cur_lightness += SELFMESH_LIGHTCTRL_LIGHTNESS_STEP;
                    }
                    if(s_light_ctrl_cfg->cur_lightness>SELFMESH_LIGHTCTRL_LIGHTNESS_MAX )
                    {
                        s_light_ctrl_cfg->cur_lightness = SELFMESH_LIGHTCTRL_LIGHTNESS_MAX;
                    }
                    s_light_ctrl_cfg->cur_switch = 1;
                    ret_val = 0;
                }
                else if(temp_light_cmd == SELFMESH_LIGHTCTRL_LIGHTNESS_DOWN)
                {
                    if(s_light_ctrl_cfg->cur_lightness>SELFMESH_LIGHTCTRL_LIGHTNESS_STEP )
                    {
                        s_light_ctrl_cfg->cur_lightness -= SELFMESH_LIGHTCTRL_LIGHTNESS_STEP;
                    }
                    else
                    {
                        s_light_ctrl_cfg->cur_lightness = 0;
                    }
                    if(s_light_ctrl_cfg->cur_lightness>0 )
                    {
                        s_light_ctrl_cfg->cur_switch = 1;
                    }
                    s_light_ctrl_cfg->cur_switch = 1;
                    ret_val = 0;
                }
                else if(temp_light_cmd == SELFMESH_LIGHTCTRL_LIGHTSWITCH)
                {
                    s_light_ctrl_cfg->cur_switch = !(s_light_ctrl_cfg->cur_switch);
                    ret_val = 0;
                }
                else if(temp_light_cmd == SELFMESH_LIGHTCTRL_LIGHTNESS_ON)
                {
                    s_light_ctrl_cfg->cur_switch = 1;
                    ret_val = 0;
                }
                else if(temp_light_cmd == SELFMESH_LIGHTCTRL_LIGHTNESS_OFF)
                {
                    s_light_ctrl_cfg->cur_switch = 0;
                    ret_val = 0;
                }
                else if(temp_light_cmd == SELFMESH_LIGHTCTRL_LIGHTNESS_SET)
                {
                    s_light_ctrl_cfg->cur_switch = 1;
                    temp_lightness = data_ptr[4];
                    temp_lightness = temp_lightness<<8;
                    temp_lightness = temp_lightness|data_ptr[5];
                    s_light_ctrl_cfg->cur_lightness = temp_lightness;
                    ret_val = 0;
                }
                else
                {
                    ret_val = 2;
                }
                if(ret_val == 0)
                {
                    lightdrv_stop();
                    if(s_light_ctrl_cfg->cur_switch == 0)
                    {
                        lightdrv_lightness_set(0);
                    }
                    else
                    {
                        lightdrv_lightness_set(s_light_ctrl_cfg->cur_lightness);
                    }
                    lightdrv_start();
                    selfmesh_store_light(s_light_ctrl_cfg->cur_switch, s_light_ctrl_cfg->cur_lightness, s_light_ctrl_cfg->cur_lighttemp);
                }
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

static void selfmesh_service_light_ctr(uint16_t value,selfmesh_service_t* cfg_set_ptr)
{
    int ret_val = 1;
    
    if(value == 1)
    {
        if(s_light_ctrl_cfg->cur_lightness<SELFMESH_LIGHTCTRL_LIGHTNESS_MAX )
       {
           s_light_ctrl_cfg->cur_lightness += SELFMESH_LIGHTCTRL_LIGHTNESS_STEP_S;
       }
       if(s_light_ctrl_cfg->cur_lightness>SELFMESH_LIGHTCTRL_LIGHTNESS_MAX -50)
       {
           s_light_ctrl_cfg->cur_lightness = 50;
       }
       s_light_ctrl_cfg->cur_switch = 1;
       ret_val = 0; 
    }

    if(value == 2)
    {

       if(s_light_ctrl_cfg->cur_lightness<SELFMESH_LIGHTCTRL_LIGHTNESS_MAX )
       {
           s_light_ctrl_cfg->cur_lightness += SELFMESH_LIGHTCTRL_LIGHTNESS_STEP_L ;
       }
       if(s_light_ctrl_cfg->cur_lightness>SELFMESH_LIGHTCTRL_LIGHTNESS_MAX-60 )
       {
           s_light_ctrl_cfg->cur_lightness = 50;
       }
       
       s_light_ctrl_cfg->cur_switch = 1;
       ret_val = 0;

    }

    if(ret_val == 0)
    {
        lightdrv_stop();
        if(s_light_ctrl_cfg->cur_switch == 0)
        {
            lightdrv_lightness_set(0);
        }
        else
        {
            lightdrv_lightness_set(s_light_ctrl_cfg->cur_lightness);
        }
        lightdrv_start();
        selfmesh_util_store_cfg((uint8_t *)&(cfg_set_ptr->service_cfg), SELFMESH_MAX_VALUE_PER_SERVICE, 0);
    }
    return;

}

/**@brief Function for service schedule.
 *
 * @param[in]  element          The element id of the service.
 * @param[io]  cfg_set_ptr      The service config data.
 * @retval 
 *
 */
/**@snippet [Handling the service schedule] */
void selfmesh_service_0001_sche(uint8_t element, selfmesh_service_t* cfg_set_ptr)
{
    int temp_chk_cnt = 0;
    int temp_current_cnt = 0;
    uint16_t cur_pressed = 0;

    if(selfmesh_is_key_pressed(element+1) == 1)
    {
        if(s_selfmesh_key_handle[element].key_press == 0)
        {
            s_selfmesh_key_handle[element].key_press = 1;
            s_selfmesh_key_handle[element].press_time = s_current_time;
            cur_pressed = 1;
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
                    cur_pressed = 2;
                }
            }
            else
            {
                if(temp_current_cnt + 0x10000 - temp_chk_cnt >= SELFMESH_LONG_PRESS_CNT)
                {
                    s_selfmesh_key_handle[element].press_time = s_current_time;
                    cur_pressed = 2;
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

    selfmesh_service_light_ctr(cur_pressed,cfg_set_ptr);

    return;
}


