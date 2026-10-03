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
#include "selfmesh_node.h"
#include "selfmesh_util.h"
#include "selfmesh_tag.h"
#include "selfsec.h"

extern int selfmesh_output_stack_log(char* log);
extern int selfmesh_open_config_channel(void);
selfmesh_msg_history_t g_node_msg_histroy[SELFMESH_MAXNODE_PER_NET] = {SELFMESH_INDEX_MAX};

/**@brief Function of the node message handle.
 *
 * @param[in]  source_addr      The address of the node which send the command.
 * @param[in]  dest_addr        The address of the dest node.
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
/**@snippet [Handling the node message handle] */
static int s_selfmesh_node_handle(selfmesh_addr_t source_addr, selfmesh_addr_t dest_addr, selfmesh_tag_t tag, uint8_t len, uint8_t* data_ptr, selfmesh_node_t* cfg_set_ptr)
{
    uint8_t index = 0;
    selfmesh_addr_t temp_daddr_t = 0xFFFF;
    int ret_val = 1;
    uint8_t temp_msg[SELFMESH_UD_BUF_SIZE] = {0x00};
    int temp_len = 0;
    if(SELFMESH_CURRENT_ADDRESS != dest_addr && SELFMESH_INVALIDE_ADDRESS != dest_addr)
    {
        return 2;
    }
    switch(tag)
    {
        case SELFMESH_TAG_SET_LOC_ADDR:
        {
            if(len == 0x02)//to set the unicast address
            {
                temp_daddr_t = *(data_ptr);
                temp_daddr_t = temp_daddr_t<<8;
                temp_daddr_t = temp_daddr_t|*(data_ptr+1);
                if(temp_daddr_t != SELFMESH_UNICAST_ADDRESS_PROVISION && temp_daddr_t < SELFMESH_UNICAST_ADDRESS_END)
                {
                    cfg_set_ptr->unicast_addr = temp_daddr_t;
                    temp_len = 0;
                    temp_msg[temp_len++] = (SELFMESH_TAG_SET_LOC_ADDR_RSP>>8)&0xFF;
                    temp_msg[temp_len++] = (SELFMESH_TAG_SET_LOC_ADDR_RSP)&0xFF;
                    temp_msg[temp_len++] = 1;//length is 1
                    temp_msg[temp_len++] = 0;//operation result is success
                    selfmesh_util_store_cfg((uint8_t *)&(cfg_set_ptr->unicast_addr), 2, 1);
                    selfmesh_util_send_app_msg(source_addr, SELFMESH_ELEMENT_ID_INVALIDE, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_msg, temp_len);
                    ret_val = 0;
                }
                else
                {
                    temp_len = 0;
                    temp_msg[temp_len++] = (SELFMESH_TAG_SET_LOC_ADDR_RSP>>8)&0xFF;
                    temp_msg[temp_len++] = (SELFMESH_TAG_SET_LOC_ADDR_RSP)&0xFF;
                    temp_msg[temp_len++] = 1;//length is 1
                    temp_msg[temp_len++] = 1;//operation result is fail
                    selfmesh_util_send_app_msg(source_addr, SELFMESH_ELEMENT_ID_INVALIDE, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_msg, temp_len);
                    ret_val = 2;
                }
            }
            else
            {
                ret_val = 2;
            }
        } break;

        case SELFMESH_TAG_GET_LOC_ADDR:
        {
            if(len == 0x00)//to get the unicast address
            {
                temp_len = 0;
                temp_msg[temp_len++] = (SELFMESH_TAG_GET_LOC_ADDR_RSP>>8)&0xFF;
                temp_msg[temp_len++] = (SELFMESH_TAG_GET_LOC_ADDR_RSP)&0xFF;
                temp_msg[temp_len++] = 2;//length is 2
                temp_msg[temp_len++] = (cfg_set_ptr->unicast_addr>>8)&0xFF;
                temp_msg[temp_len++] = (cfg_set_ptr->unicast_addr)&0xFF;
                selfmesh_util_send_app_msg(source_addr, SELFMESH_ELEMENT_ID_INVALIDE, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_msg, temp_len);
                ret_val = 0;
            }
            else
            {
                ret_val = 2;
            }
        } break;

        case SELFMESH_TAG_SET_NETKEY:
        {
            if(len == SELFMESH_NETKEY_LENGTH)//to set the netkey
            {
                memcpy(cfg_set_ptr->net_key, data_ptr, SELFMESH_NETKEY_LENGTH);
                temp_len = 0;
                temp_msg[temp_len++] = (SELFMESH_TAG_SET_NETKEY_RSP>>8)&0xFF;
                temp_msg[temp_len++] = (SELFMESH_TAG_SET_NETKEY_RSP)&0xFF;
                temp_msg[temp_len++] = 1;//length is 1
                temp_msg[temp_len++] = 0;//operation result is success
                selfmesh_util_store_cfg((uint8_t *)&(cfg_set_ptr->net_key), SELFMESH_NETKEY_LENGTH, 1);
                selfmesh_util_send_app_msg(source_addr, SELFMESH_ELEMENT_ID_INVALIDE, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_msg, temp_len);
                ret_val = 0;
            }
            else
            {
                ret_val = 2;
            }
        } break;

        case SELFMESH_TAG_GET_NETKEY:
        {
            if(len == 0x00)//to get the netkey
            {
                temp_len = 0;
                temp_msg[temp_len++] = (SELFMESH_TAG_GET_NETKEY_RSP>>8)&0xFF;
                temp_msg[temp_len++] = (SELFMESH_TAG_GET_NETKEY_RSP)&0xFF;
                temp_msg[temp_len++] = SELFMESH_NETKEY_LENGTH;//length of netkey
                memcpy(temp_msg+temp_len, cfg_set_ptr->net_key, SELFMESH_NETKEY_LENGTH);
                temp_len+=SELFMESH_NETKEY_LENGTH;
                selfmesh_util_send_app_msg(source_addr, SELFMESH_ELEMENT_ID_INVALIDE, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_msg, temp_len);
                ret_val = 0;
            }
            else
            {
                ret_val = 2;
            }
        } break;

        case SELFMESH_TAG_GET_PIDVID:
        {
            if(len == 0x00)//to get the PID and VID
            {
                temp_len = 0;
                temp_msg[temp_len++] = (SELFMESH_TAG_GET_PIDVID_RSP>>8)&0xFF;
                temp_msg[temp_len++] = (SELFMESH_TAG_GET_PIDVID_RSP)&0xFF;
                temp_msg[temp_len++] = 4;//length is 4
                temp_msg[temp_len++] = (SELFMESH_NODE_PID>>8)&0xFF;//PID M
                temp_msg[temp_len++] = (SELFMESH_NODE_PID)&0xFF;//PID L
                temp_msg[temp_len++] = (SELFMESH_NODE_VID>>8)&0xFF;//PID M
                temp_msg[temp_len++] = (SELFMESH_NODE_VID)&0xFF;//PID L
                selfmesh_util_send_app_msg(source_addr, SELFMESH_ELEMENT_ID_INVALIDE, SELFMESH_ELEMENT_ID_PROVISION, 0, temp_msg, temp_len);
                ret_val = 0;
            }
            else
            {
                ret_val = 2;
            }
        } break;

        case SELFMESH_TAG_SYSTEM_RESET:
        {
            if(len == 0x01)//to reset the system
            {
                if(data_ptr[0] == SELFMESH_START_MODE_CONFIG)
                {
                    cfg_set_ptr->start_mode = SELFMESH_START_MODE_CONFIG;
                    selfmesh_util_store_cfg((uint8_t *)&(cfg_set_ptr->start_mode), 1, 1);
                }
                else if(data_ptr[0] == SELFMESH_START_MODE_NORMAL)
                {
                    cfg_set_ptr->start_mode = SELFMESH_START_MODE_NORMAL;
                    selfmesh_util_store_cfg((uint8_t *)&(cfg_set_ptr->start_mode), 1, 1);
                }
                selfmesh_util_system_reset();
                ret_val = 0;
            }
            else
            {
                ret_val = 2;
            }
        } break;

        case SELFMESH_TAG_OPEN_PROXY_CHANNEL:
        {
            if(len == 0x00 || len == 0x06)//to open the proxy channel
            {
                selfmesh_open_proxy_channel();
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

/**@brief Function of the node init.
 *
 * @param[io]  cfg_set_ptr      The node config data.
 * @retval 0                    Init success.
 * @retval else                 Error.
 *
 */
/**@snippet [Handling the element init] */
int selfmesh_node_init(selfmesh_node_t* cfg_set_ptr)
{
    uint8_t index = 0;
    uint8_t startmode = 0;
    memset(g_node_msg_histroy, 0xFF, sizeof(g_node_msg_histroy));
    if(cfg_set_ptr->magic_value != SELFMESH_NODE_MAGIC_VALUE)
    {
        cfg_set_ptr->magic_value = SELFMESH_NODE_MAGIC_VALUE;
        selfmesh_node_config_default(cfg_set_ptr);
        cfg_set_ptr->magic_value2 = SELFMESH_NODE_MAGIC_VALUE;
        startmode = cfg_set_ptr->start_mode;
        selfmesh_util_init(startmode, cfg_set_ptr, g_node_msg_histroy);
        selfmesh_util_store_cfg((uint8_t *)cfg_set_ptr, sizeof(selfmesh_node_t), 1);
    }
    else
    {
        startmode = cfg_set_ptr->start_mode;
        selfmesh_util_init(startmode, cfg_set_ptr, g_node_msg_histroy);
    }
    for(index = 0; index<SELFMESH_MAX_ELEMENT_CNT; index++)
    {
        if(cfg_set_ptr->element_a[index].magic_value == SELFMESH_ELEMENT_MAGIC_VALUE)
        {
            selfmesh_element_init(startmode, index, &(cfg_set_ptr->element_a[index]));
        }
    }
    return 0;
}


/**@brief Function of the node to handle the data from the net.
 *
 * @param[in]  data_ptr         The data get from the mesh net.
 * @param[io]  cfg_set_ptr      The node config data.
 * @retval 0                    The data is processed successfully by the element and the data will be retrain.
 * @retval 1                    The data is invalid.
 * @retval 2                    The data is duplicate.
 * @retval 10                   The data is processed successfully by the element and the data should not be retrain.
 *
 */
/**@snippet [Handling the data received from the net] */
int selfmesh_node_handle(uint8_t* data_ptr, selfmesh_node_t* cfg_set_ptr)
{
    uint8_t element_index = 0;
    uint8_t temp_magic_val_m = 0x00;
    uint8_t temp_hop_cnt = 0x00;
    uint8_t temp_flag = 0x00;
    uint8_t temp_need_dec = 1;
    int temp_sto_msg_index = 0;
    int temp_cur_msg_index = 0;
    selfmesh_addr_t temp_saddr = {0xFF};
    selfmesh_addr_t temp_daddr = {0xFF};
    uint8_t temp_ud_buf[SELFMESH_UD_BUF_SIZE] = {0x00};
    int temp_ud_len = SELFMESH_UD_BUF_SIZE;
    selfmesh_tag_t temp_tag;
    uint8_t temp_v_len;
#if (SELFMESH_MSG_PRETREAT)
#else
    if(SELFMESH_ALLOW_RAW_DATA)
#endif
    {
        temp_magic_val_m = *(data_ptr+SELFMASH_MAGIC_M_OFFSET);
        temp_hop_cnt = *(data_ptr+SELFMASH_HOP_CNT_OFFSET);
        temp_flag = temp_hop_cnt&0x0F;
        temp_hop_cnt = temp_hop_cnt>>4;
        if(temp_magic_val_m == SELFMESH_MSG_MAGIC_VAL_M && (!(temp_flag&SELFMESH_FLAG_USE_NETKEY_MASK)))
        {
            temp_need_dec = 0;
        }
        else
        {
        }
    }
    if(temp_need_dec)
    {
        selfsec_decrypt(cfg_set_ptr->net_key, data_ptr, SELFMESH_UD_BUF_SIZE, temp_ud_buf, &temp_ud_len);
        temp_magic_val_m = temp_ud_buf[SELFMASH_MAGIC_M_OFFSET];
        temp_hop_cnt = temp_ud_buf[SELFMASH_HOP_CNT_OFFSET];
        temp_flag = temp_hop_cnt&0x0F;
        temp_hop_cnt = temp_hop_cnt>>4;
        if(temp_magic_val_m == SELFMESH_MSG_MAGIC_VAL_M && (temp_flag&SELFMESH_FLAG_USE_NETKEY_MASK))
        {
        }
        else
        {
            return 1;
        }
    }
    else
    {
        memcpy(temp_ud_buf, data_ptr, SELFMESH_UD_BUF_SIZE);
    }
    if(temp_hop_cnt > SELFMESH_MSG_MAX_HOP)
    {
        return 1;//data error
    }
    temp_saddr = temp_ud_buf[SELFMASH_SOURCE_ADDR_M_OFFSET];
    temp_saddr = (temp_saddr<<8)|temp_ud_buf[SELFMASH_SOURCE_ADDR_L_OFFSET];
    if(temp_saddr >= SELFMESH_MAXNODE_PER_NET)
    {
        return 1;//data error
    }
    temp_daddr = temp_ud_buf[SELFMASH_DEST_ADDR_M_OFFSET];
    temp_daddr = (temp_daddr<<8)|temp_ud_buf[SELFMASH_DEST_ADDR_L_OFFSET];
    temp_cur_msg_index = temp_ud_buf[SELFMASH_INDEX_OFFSET];
    temp_sto_msg_index = g_node_msg_histroy[temp_saddr].msg_index;

    if(temp_sto_msg_index == SELFMESH_INDEX_MAX)
    {
        g_node_msg_histroy[temp_saddr].msg_index = temp_ud_buf[SELFMASH_INDEX_OFFSET];
    }
    else if(temp_sto_msg_index >= 0xF0 && temp_cur_msg_index <= 0x0F)
    {
        g_node_msg_histroy[temp_saddr].msg_index = temp_ud_buf[SELFMASH_INDEX_OFFSET];
    }
    else if(temp_sto_msg_index >= temp_cur_msg_index)
    {
        return 2;
    }
    else
    {
        g_node_msg_histroy[temp_saddr].msg_index = temp_ud_buf[SELFMASH_INDEX_OFFSET];
    }
    if(temp_daddr == cfg_set_ptr->unicast_addr)
    {
        temp_daddr = SELFMESH_CURRENT_ADDRESS;
    }
    element_index = temp_ud_buf[SELFMASH_ELEMENT_OFFSET];
    if(element_index == SELFMESH_ELEMENT_ID_INVALIDE)
    {
        temp_tag = temp_ud_buf[SELFMASH_APP_DATA_OFFSET];
        temp_tag = (temp_tag<<8)|temp_ud_buf[SELFMASH_APP_DATA_OFFSET+1];
        temp_v_len = temp_ud_buf[SELFMASH_APP_DATA_OFFSET+2];
        s_selfmesh_node_handle(temp_saddr, temp_daddr, temp_tag, temp_v_len, temp_ud_buf+SELFMASH_APP_DATA_OFFSET+3, cfg_set_ptr);
    }
    else if(element_index < SELFMESH_MAX_ELEMENT_CNT && cfg_set_ptr->element_a[element_index].magic_value == SELFMESH_ELEMENT_MAGIC_VALUE)
    {
        return selfmesh_element_handle(temp_saddr, temp_daddr, element_index, (temp_flag&SELFMESH_FLAG_USE_APPKEY_MASK), temp_ud_buf+SELFMASH_APP_DATA_OFFSET, &(cfg_set_ptr->element_a[element_index]));
    }
    else if(element_index == SELFMESH_ELEMENT_ID_ALL)
    {
        for(int i=0;i<SELFMESH_MAX_ELEMENT_CNT;i++)
        {
            selfmesh_element_handle(temp_saddr, temp_daddr, i, (temp_flag&SELFMESH_FLAG_USE_APPKEY_MASK), temp_ud_buf+SELFMASH_APP_DATA_OFFSET, &(cfg_set_ptr->element_a[i]));
        }
        return 0;
    }
    else
    {
        return 1;
    }
}

/**@brief Function of the node to pretreat the data from the net.
 **@brief To decrypt the data and check the magic value and the duplicate.
 *
 * @param[in]  data_ptr         The data get from the mesh net, the length must be 20 bytes.
 * @param[io]  cfg_set_ptr      The node config data.
 * @param[out] out_ptr          The pretreat output, the length must be 20 bytes.
 * @retval 0                    The data is processed successfully by the element.
 * @retval 1                    The data is invalid.
 * @retval 2                    The data is duplicate.
 *
 */
/**@snippet [Handling the data received from the net] */
int selfmesh_node_pretreat(const uint8_t* data_ptr, selfmesh_node_t* cfg_set_ptr, uint8_t* out_ptr)
{
#if (SELFMESH_MSG_PRETREAT)
    uint8_t element_index = 0;
    uint8_t temp_magic_val_m = 0x00;
    uint8_t temp_hop_cnt = 0x00;
    uint8_t temp_flag = 0x00;
    uint8_t temp_need_dec = 1;
    int temp_sto_msg_index = 0;
    int temp_cur_msg_index = 0;
    selfmesh_addr_t temp_saddr = {0xFF};
    uint8_t temp_ud_buf[SELFMESH_UD_BUF_SIZE] = {0x00};
#if (SELFMESH_STACK_LOG_OUT)
    char temp_log_out[SELFMESH_STACK_LOG_MAX] = {0x00};
#endif
    int temp_ud_len = SELFMESH_UD_BUF_SIZE;
    if(SELFMESH_ALLOW_RAW_DATA)
    {
        temp_magic_val_m = *(data_ptr+SELFMASH_MAGIC_M_OFFSET);
        temp_hop_cnt = *(data_ptr+SELFMASH_HOP_CNT_OFFSET);
        temp_flag = temp_hop_cnt&0x0F;
        temp_hop_cnt = temp_hop_cnt>>4;
        if(temp_magic_val_m == SELFMESH_MSG_MAGIC_VAL_M && (!(temp_flag&SELFMESH_FLAG_USE_NETKEY_MASK)))
        {
            temp_need_dec = 0;
        }
        else
        {
        }
    }
    if(temp_need_dec)
    {
        selfsec_decrypt(cfg_set_ptr->net_key, data_ptr, SELFMESH_UD_BUF_SIZE, temp_ud_buf, &temp_ud_len);
        temp_magic_val_m = temp_ud_buf[SELFMASH_MAGIC_M_OFFSET];
        temp_hop_cnt = temp_ud_buf[SELFMASH_HOP_CNT_OFFSET];
        temp_flag = temp_hop_cnt&0x0F;
        temp_hop_cnt = temp_hop_cnt>>4;
        if(temp_magic_val_m == SELFMESH_MSG_MAGIC_VAL_M && (temp_flag&SELFMESH_FLAG_USE_NETKEY_MASK))
        {
        }
        else
        {
            return 1;
        }
    }
    else
    {
        memcpy(temp_ud_buf, data_ptr, SELFMESH_UD_BUF_SIZE);
    }
    if(temp_hop_cnt > SELFMESH_MSG_MAX_HOP)
    {
        return 1;//data error
    }
    temp_saddr = temp_ud_buf[SELFMASH_SOURCE_ADDR_M_OFFSET];
    temp_saddr = (temp_saddr<<8)|temp_ud_buf[SELFMASH_SOURCE_ADDR_L_OFFSET];
    if(temp_saddr >= SELFMESH_MAXNODE_PER_NET)
    {
        return 1;//data error
    }
    temp_cur_msg_index = temp_ud_buf[SELFMASH_INDEX_OFFSET];
    temp_sto_msg_index = g_node_msg_histroy[temp_saddr].msg_index;
    if(temp_saddr == cfg_set_ptr->unicast_addr)
    {
        return 2;
    }

    if(temp_sto_msg_index == SELFMESH_INDEX_MAX)
    {
    }
    else if(temp_sto_msg_index >= 0xF0 && temp_cur_msg_index <= 0x0F)
    {
    }
    else if(temp_sto_msg_index >= temp_cur_msg_index)
    {
        return 2;
    }
    else
    {
    }
    temp_flag &= (~SELFMESH_FLAG_USE_NETKEY_MASK);
    temp_hop_cnt = temp_hop_cnt<<4;
    temp_hop_cnt |= temp_flag;
    temp_ud_buf[SELFMASH_HOP_CNT_OFFSET] = temp_hop_cnt;
    memcpy(out_ptr, temp_ud_buf, SELFMESH_UD_BUF_SIZE);
#if (SELFMESH_STACK_LOG_OUT)
    sprintf(temp_log_out, "RCV:"); 
    for(int i=0;i<SELFMESH_UD_BUF_SIZE;i++)
    {
        sprintf(temp_log_out+strlen(temp_log_out), "%02X", out_ptr[i]);
    }
    selfmesh_output_stack_log(temp_log_out);
#endif
    return 0;
#else
    return 1;
#endif
}

/**@brief Function for node schedule.
 *
 * @param[io]  cfg_set_ptr      The element config data.
 * @retval 
 *
 */
/**@snippet [Handling the node schedule] */
void selfmesh_node_sche(selfmesh_node_t* cfg_set_ptr)
{
    uint8_t index = 0;
    for(index = 0; index<SELFMESH_MAX_ELEMENT_CNT; index++)
    {
        if(cfg_set_ptr->element_a[index].magic_value == SELFMESH_ELEMENT_MAGIC_VALUE)
        {
            selfmesh_element_sche(index, &(cfg_set_ptr->element_a[index]));
        }
    }
    return;
}


