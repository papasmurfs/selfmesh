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
#include <string.h>
#include "selfmesh_element.h"
#include "selfmesh_util.h"
#include "selfmesh_tag.h"

extern int selfmesh_output_stack_log(char* log);

/**@brief Function of the element message handle.
 *
 * @param[in]  source_addr      The address of the node which send the command.
 * @param[in]  dest_addr        The address of the dest node.
 * @param[in]  cur_element      The element id of the current element.
 * @param[in]  tag              The tag of the data to handle.
 * @param[in]  len              The data length.
 * @param[in]  data_ptr         The data to handle.
 * @param[in]  cfg_set_ptr      The service config data.
 * @retval 0                    The data is processed successfully by the element and the data shall be retrain.
 * @retval 1                    The data should not to be handled here.
 * @retval 2                    The data handled error and should not to be retrain.
 * @retval 10                   The data is processed successfully by the element and the data should not be retrain.
 * @retval else                 Error.
 *
 */
/**@snippet [Handling the element message handle] */
static int s_selfmesh_element_handle(selfmesh_addr_t source_addr, selfmesh_addr_t dest_addr, uint8_t cur_element, selfmesh_tag_t tag, uint8_t len, uint8_t* data_ptr, selfmesh_element_t* cfg_set_ptr)
{
    uint8_t index = 0;
    int ret_val = 1;
    uint8_t temp_msg[SELFMESH_UD_BUF_SIZE] = {0x00};
    int temp_len = 0;
    switch(tag)
    {
        case SELFMESH_TAG_SET_APPKEY:
        {
            if(len == SELFMESH_APPKEY_LENGTH)//to set the appkey
            {
                memcpy(cfg_set_ptr->app_key, data_ptr, SELFMESH_APPKEY_LENGTH);
                temp_len = 0;
                temp_msg[temp_len++] = (SELFMESH_TAG_SET_APPKEY_RSP>>8)&0xFF;
                temp_msg[temp_len++] = (SELFMESH_TAG_SET_APPKEY_RSP)&0xFF;
                temp_msg[temp_len++] = 1;//length is 1
                temp_msg[temp_len++] = 0;//operation result is success
                selfmesh_util_store_cfg((uint8_t *)&(cfg_set_ptr->app_key), SELFMESH_APPKEY_LENGTH, 1);
                selfmesh_util_send_app_msg(source_addr, cur_element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_msg, temp_len);
                ret_val = 0;
            }
            else
            {
                ret_val = 2;
            }
        } break;

        case SELFMESH_TAG_GET_APPKEY:
        {
            if(len == 0x00)//to get the appkey
            {
                temp_len = 0;
                temp_msg[temp_len++] = (SELFMESH_TAG_GET_APPKEY_RSP>>8)&0xFF;
                temp_msg[temp_len++] = (SELFMESH_TAG_GET_APPKEY_RSP)&0xFF;
                temp_msg[temp_len++] = SELFMESH_APPKEY_LENGTH;//length is 8
                memcpy(temp_msg+temp_len, cfg_set_ptr->app_key, SELFMESH_APPKEY_LENGTH);
                temp_len+=SELFMESH_APPKEY_LENGTH;
                //temp_msg[temp_len++] = cfg_set_ptr->app_key[0];//appkey[0]
                //temp_msg[temp_len++] = cfg_set_ptr->app_key[1];//appkey[1]
                //temp_msg[temp_len++] = cfg_set_ptr->app_key[2];//appkey[2]
                //temp_msg[temp_len++] = cfg_set_ptr->app_key[3];//appkey[3]
                selfmesh_util_send_app_msg(source_addr, cur_element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_msg, temp_len);
                ret_val = 0;
            }
            else
            {
                ret_val = 2;
            }
        } break;

        case SELFMESH_TAG_SET_PUB_ADDR:
        {
            if(len == 0x02)//to set the public address
            {
                cfg_set_ptr->pub_addr = *(data_ptr);
                cfg_set_ptr->pub_addr = cfg_set_ptr->pub_addr<<8;
                cfg_set_ptr->pub_addr = cfg_set_ptr->pub_addr|*(data_ptr+1);
                temp_len = 0;
                temp_msg[temp_len++] = (SELFMESH_TAG_SET_PUB_ADDR_RSP>>8)&0xFF;
                temp_msg[temp_len++] = (SELFMESH_TAG_SET_PUB_ADDR_RSP)&0xFF;
                temp_msg[temp_len++] = 1;//length is 1
                temp_msg[temp_len++] = 0;//operation result is success
                selfmesh_util_store_cfg((uint8_t *)&(cfg_set_ptr->pub_addr), 2, 1);
                selfmesh_util_send_app_msg(source_addr, cur_element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_msg, temp_len);
                ret_val = 0;
            }
            else
            {
                ret_val = 2;
            }
        } break;

        case SELFMESH_TAG_GET_PUB_ADDR:
        {
            if(len == 0x00)//to get the public address
            {
                temp_len = 0;
                temp_msg[temp_len++] = (SELFMESH_TAG_GET_PUB_ADDR_RSP>>8)&0xFF;
                temp_msg[temp_len++] = (SELFMESH_TAG_GET_PUB_ADDR_RSP)&0xFF;
                temp_msg[temp_len++] = 2;//length is 2
                temp_msg[temp_len++] = (cfg_set_ptr->pub_addr>>8)&0xFF;
                temp_msg[temp_len++] = (cfg_set_ptr->pub_addr)&0xFF;
                selfmesh_util_send_app_msg(source_addr, cur_element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_msg, temp_len);
                ret_val = 0;
            }
            else
            {
                ret_val = 2;
            }
        } break;
        
        case SELFMESH_TAG_SET_SUB_ADDR:
        {
            if(len == 0x06)//to set the subscrip address
            {
                cfg_set_ptr->sub_addr[0] = *(data_ptr);
                cfg_set_ptr->sub_addr[0] = cfg_set_ptr->sub_addr[0]<<8;
                cfg_set_ptr->sub_addr[0] = cfg_set_ptr->sub_addr[0]|*(data_ptr+1);
                cfg_set_ptr->sub_addr[1] = *(data_ptr+2);
                cfg_set_ptr->sub_addr[1] = cfg_set_ptr->sub_addr[1]<<8;
                cfg_set_ptr->sub_addr[1] = cfg_set_ptr->sub_addr[1]|*(data_ptr+3);
                cfg_set_ptr->sub_addr[2] = *(data_ptr+4);
                cfg_set_ptr->sub_addr[2] = cfg_set_ptr->sub_addr[2]<<8;
                cfg_set_ptr->sub_addr[2] = cfg_set_ptr->sub_addr[2]|*(data_ptr+5);
                temp_len = 0;
                temp_msg[temp_len++] = (SELFMESH_TAG_SET_SUB_ADDR_RSP>>8)&0xFF;
                temp_msg[temp_len++] = (SELFMESH_TAG_SET_SUB_ADDR_RSP)&0xFF;
                temp_msg[temp_len++] = 1;//length is 1
                temp_msg[temp_len++] = 0;//operation result is success
                selfmesh_util_store_cfg((uint8_t *)&(cfg_set_ptr->sub_addr), 6, 1);
                selfmesh_util_send_app_msg(source_addr, cur_element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_msg, temp_len);
                ret_val = 0;
            }
            else
            {
                ret_val = 2;
            }
        } break;
        
        case SELFMESH_TAG_GET_SUB_ADDR:
        {
            if(len == 0x00)//to get the public address
            {
                temp_len = 0;
                temp_msg[temp_len++] = (SELFMESH_TAG_GET_SUB_ADDR_RSP>>8)&0xFF;
                temp_msg[temp_len++] = (SELFMESH_TAG_GET_SUB_ADDR_RSP)&0xFF;
                temp_msg[temp_len++] = 6;//length is 2
                temp_msg[temp_len++] = (cfg_set_ptr->sub_addr[0]>>8)&0xFF;//subscrip address0 M
                temp_msg[temp_len++] = (cfg_set_ptr->sub_addr[0])&0xFF;//subscrip address0 L
                temp_msg[temp_len++] = (cfg_set_ptr->sub_addr[1]>>8)&0xFF;//subscrip address1 M
                temp_msg[temp_len++] = (cfg_set_ptr->sub_addr[1])&0xFF;//subscrip address1 L
                temp_msg[temp_len++] = (cfg_set_ptr->sub_addr[2]>>8)&0xFF;//subscrip address2 M
                temp_msg[temp_len++] = (cfg_set_ptr->sub_addr[2])&0xFF;//subscrip address2 L
                selfmesh_util_send_app_msg(source_addr, cur_element, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_msg, temp_len);
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
    return ret_val;
}

/**@brief Function of the element init.
 *
 * @param[in]  start_mode       The system start mode.
 * @param[in]  element          The element id of the element.
 * @param[io]  cfg_set_ptr      The element config data.
 * @retval 0                    Init success.
 * @retval else                 Error.
 *
 */
/**@snippet [Handling the element init] */
int selfmesh_element_init(uint8_t start_mode, uint8_t element, selfmesh_element_t* cfg_set_ptr)
{
    int index = 0;
    if(cfg_set_ptr->magic_value != SELFMESH_ELEMENT_MAGIC_VALUE)
    {
        return 1;
    }
    for(index = 0; index<SELFMESH_MAX_SERVICE_PER_ELEMENT; index++)
    {
        if(cfg_set_ptr->service_a[index].magic_value == SELFMESH_SERVICE_MAGIC_VALUE)
        {
            selfmesh_service_init(start_mode, element, cfg_set_ptr->service_a[index].service_uuid, &(cfg_set_ptr->service_a[index]));
        }
    }
    return 0;
}


/**@brief Function of the element to handle the data from the net.
 *
 * @param[in]  source_addr      The address of the node which send the command the the element.
 * @param[in]  dest_addr        The dest address of the command.
 * @param[in]  data_enc         1:data need to be decrypt first; 0:raw data.
 * @param[in]  element          The element id of the element.
 * @param[in]  data_ptr         The data get from the mesh net.
 * @param[io]  cfg_set_ptr      The element config data.
 * @retval 0                    The data is processed successfully by the element.
 * @retval 1                    The data should not to be handled here.
 *
 */
/**@snippet [Handling the data received from the net] */
int selfmesh_element_handle(selfmesh_addr_t source_addr, selfmesh_addr_t dest_addr, uint8_t element, uint8_t data_enc, uint8_t* data_ptr, selfmesh_element_t* cfg_set_ptr)
{
    int ret = 0;
    int index = 0;
    uint8_t dest_match = 0;
    uint8_t temp_app_buf[SELFMASH_APP_DATA_LEN] = {0x00};
    int temp_app_len = SELFMASH_APP_DATA_LEN;
    selfmesh_tag_t temp_tag;
    uint8_t temp_v_len;
    if(cfg_set_ptr->magic_value != SELFMESH_ELEMENT_MAGIC_VALUE)
    {
        return 2;
    }
		for(int i=0; i<SELFMESH_MAX_SUB_ADDRESS_PER_ELEMENT; i++)
		{
        if(dest_addr == cfg_set_ptr->sub_addr[i])
        {
            dest_match = 1;
            break;
        }
		}
		if(0 == dest_match)
    {
        if(dest_addr == SELFMESH_CURRENT_ADDRESS || dest_addr == SELFMESH_GOUP_ADDRESS_ALL)
        {
            dest_match = 1;
        }
        else
        {
            return 1;
        }
    }
		if(0 != data_enc)
    {
        selfsec_decrypt(cfg_set_ptr->app_key, data_ptr, SELFMASH_APP_DATA_LEN, temp_app_buf, &temp_app_len);
    }
		else
    {
        memcpy(temp_app_buf, data_ptr, SELFMASH_APP_DATA_LEN);
    }
    temp_tag = temp_app_buf[0];
    temp_tag = (temp_tag<<8)|temp_app_buf[1];
    temp_v_len = temp_app_buf[2];
    ret = s_selfmesh_element_handle(source_addr, dest_addr, element, temp_tag, temp_v_len, temp_app_buf+3, cfg_set_ptr);
    if(ret == 1)
    {
        for(index = 0; index<SELFMESH_MAX_SERVICE_PER_ELEMENT; index++)
        {
            if(cfg_set_ptr->service_a[index].magic_value == SELFMESH_SERVICE_MAGIC_VALUE)
            {
                selfmesh_service_handle(source_addr, element, cfg_set_ptr->service_a[index].service_uuid, temp_app_buf, &(cfg_set_ptr->service_a[index]));
            }
        }
    }
    return 0;
}


/**@brief Function for element schedule.
 *
 * @param[in]  element          The element id of the element.
 * @param[io]  cfg_set_ptr      The element config data.
 * @retval 
 *
 */
/**@snippet [Handling the element schedule] */
void selfmesh_element_sche(uint8_t element, selfmesh_element_t* cfg_set_ptr)
{
    int index = 0;
    if(cfg_set_ptr->magic_value != SELFMESH_ELEMENT_MAGIC_VALUE)
    {
        return;
    }
    for(index = 0; index<SELFMESH_MAX_SERVICE_PER_ELEMENT; index++)
    {
        if(cfg_set_ptr->service_a[index].magic_value == SELFMESH_SERVICE_MAGIC_VALUE)
        {
            selfmesh_service_sche(element, cfg_set_ptr->service_a[index].service_uuid, &(cfg_set_ptr->service_a[index]));
        }
    }
    return;
}


