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
#include "selfmesh_tag.h"
#include "selfmesh_util.h"

static selfmesh_node_t* s_selfmesh_util_node_p = NULL;
static selfmesh_msg_history_t* s_selfmesh_util_msg_history_p = NULL;
extern int selfmesh_send_net_msg(uint8_t* msg_ptr, uint8_t msg_len, uint8_t is_retrain_msg);
extern int selfmesh_store(selfmesh_node_t* node_cfg_ptr, int offset, int msg_len, uint8_t flush);
extern void selfmesh_store_index(uint8_t index);
extern int selfmesh_open_config_channel(void);
extern int selfmesh_open_normal_channel(void);
extern int selfmesh_output_stack_log(char* log);
extern void selfmesh_reset(void);

/**@brief Function of the util init.
 *
 * @param[in] start_mode        The system start mode.
 * @param[in] cfg_set_ptr       The node config data.
 * @param[in] msg_history_p     The node message history.
 * @retval 0                    Init success.
 * @retval else                 Error.
 *
 */
/**@snippet [Handling the util init] */
int selfmesh_util_init(uint8_t start_mode, selfmesh_node_t* cfg_set_ptr, selfmesh_msg_history_t* msg_history_p)
{
    s_selfmesh_util_node_p = cfg_set_ptr;
    s_selfmesh_util_msg_history_p = msg_history_p;
    if(start_mode == SELFMESH_START_MODE_NORMAL)
    {
        selfmesh_open_normal_channel();
    }
    else
    {
        selfmesh_open_config_channel();
    }
    return 0;
}

/**@brief Function of the app message send.
 *
 * @param[in] dest_addr         The dest address of the message.
 * @param[in] source_element    The source element of the message.
 * @param[in] dest_element      The dest element of the message.
 * @param[in] hop               The hop count of the message.
 * @param[in] msg_ptr           The user data, TLV format.
 * @param[in] msg_len           The user data length.
 * @retval 0                    send success.
 * @retval else                 Error.
 *
 */
/**@snippet [Handling the app data send] */
int selfmesh_util_send_app_msg(selfmesh_addr_t dest_addr, uint8_t source_element, uint8_t dest_element, uint8_t hop, uint8_t* msg_ptr, uint8_t msg_len)
{
    uint8_t temp_msg[SELFMESH_UD_BUF_SIZE] = {0x00};
    uint8_t temp_buf[SELFMESH_UD_BUF_SIZE] = {0x00};
#if (SELFMESH_STACK_LOG_OUT)
    char temp_log_out[SELFMESH_STACK_LOG_MAX] = {0x00};
#endif
    uint8_t temp_flag = 0;
    selfmesh_addr_t temp_dest_addr = dest_addr;
    int temp_len = 0;
    if(msg_len > SELFMASH_APP_DATA_LEN || hop>SELFMESH_MSG_MAX_HOP)
    {
        return 1;
    }
    if(SELFMESH_INVALIDE_ADDRESS == dest_addr)
    {
        temp_dest_addr = s_selfmesh_util_node_p->element_a[source_element].pub_addr;
    }
    memcpy(temp_msg+SELFMASH_APP_DATA_OFFSET, msg_ptr, msg_len);
    if(source_element != SELFMESH_ELEMENT_ID_INVALIDE
        && source_element != SELFMESH_ELEMENT_ID_ALL
        && source_element < SELFMESH_MAX_ELEMENT_CNT)
    {
        if(s_selfmesh_util_node_p->element_a[source_element].magic_value == SELFMESH_ELEMENT_MAGIC_VALUE)
        {
            //temp modify for not encrypt
            //temp_len = SELFMASH_APP_DATA_LEN;
            //selfsec_encrypt(s_selfmesh_util_node_p->element_a[source_element].app_key, temp_msg+SELFMASH_APP_DATA_OFFSET, SELFMASH_APP_DATA_LEN, temp_buf, &temp_len);
            //memcpy(temp_msg+SELFMASH_APP_DATA_OFFSET, temp_buf, temp_len);
            //temp_flag |= SELFMESH_FLAG_USE_APPKEY_MASK;
        }
    }
    temp_msg[0] = SELFMESH_MSG_MAGIC_VAL_M;
    //temp modify for not encrypt
    //temp_flag |= SELFMESH_FLAG_USE_NETKEY_MASK;
    temp_msg[1] = (hop<<4)|temp_flag;
    s_selfmesh_util_node_p->snd_indx = (1+s_selfmesh_util_node_p->snd_indx)%SELFMESH_INDEX_MAX;
    selfmesh_store_index(s_selfmesh_util_node_p->snd_indx & 0xFF);
    temp_msg[2] = s_selfmesh_util_node_p->snd_indx;
    temp_msg[3] = (s_selfmesh_util_node_p->unicast_addr>>8) & 0x00FF;
    temp_msg[4] = (s_selfmesh_util_node_p->unicast_addr) & 0x00FF;
    temp_msg[5] = (temp_dest_addr>>8) & 0x00FF;
    temp_msg[6] = (temp_dest_addr) & 0x00FF;
    temp_msg[7] = dest_element;
    temp_len = SELFMESH_UD_BUF_SIZE;
    //temp modify for not encrypt
    //selfsec_encrypt(s_selfmesh_util_node_p->net_key, temp_msg, SELFMESH_UD_BUF_SIZE, temp_buf, &temp_len);
    memcpy(temp_buf, temp_msg, temp_len);
#if (SELFMESH_STACK_LOG_OUT)
    sprintf(temp_log_out, "OUT:"); 
    for(int i=0;i<SELFMESH_UD_BUF_SIZE;i++)
    {
        sprintf(temp_log_out+strlen(temp_log_out), "%02X", temp_buf[i]);
    }
    selfmesh_output_stack_log(temp_log_out);
#endif
    selfmesh_send_net_msg(temp_buf, temp_len, 0);
    return 0;
}


/**@brief Function of the net message send for next hop.
 *
 * @param[in] ud_ptr            The user data.
 * @retval 0                    send success.
 * @retval else                 Error.
 *
 */
/**@snippet [Handling the net messget send for next hop] */
int selfmesh_util_send_msg_next_hop(uint8_t* ud_ptr)
{
    uint8_t temp_msg[SELFMESH_UD_BUF_SIZE] = {0x00};
    uint8_t temp_buf[SELFMESH_UD_BUF_SIZE] = {0x00};
    uint8_t temp_flag = 0;
    uint8_t temp_hop = 0;
    int temp_len = 0;
    memcpy(temp_msg, ud_ptr, SELFMESH_UD_BUF_SIZE);
    temp_flag = temp_msg[SELFMASH_HOP_CNT_OFFSET];
    temp_hop = temp_flag>>4;
    if(temp_hop == 0)
    {
        return 1;
    }
    temp_hop = temp_hop-1;
    temp_flag &= 0x0F;
    temp_flag |= temp_hop<<4;
    temp_flag |= SELFMESH_FLAG_USE_NETKEY_MASK;
    temp_msg[SELFMASH_HOP_CNT_OFFSET] = temp_flag;
    //if(temp_flag & SELFMESH_FLAG_USE_NETKEY_MASK)
    {
        temp_len = SELFMESH_UD_BUF_SIZE;
        selfsec_encrypt(s_selfmesh_util_node_p->net_key, temp_msg, SELFMESH_UD_BUF_SIZE, temp_buf, &temp_len);
        memcpy(temp_msg, temp_buf, SELFMESH_UD_BUF_SIZE);
    }
    selfmesh_send_net_msg(temp_msg, SELFMESH_UD_BUF_SIZE, 1);
    return 0;
}


/**@brief Function to store the config message.
 *
 * @param[in] cfg_ptr           The config data.
 * @param[in] cfg_len           The config data length.
 * @param[in] flush             0-not sync flush, 1-sync flush.
 * @retval 0                    send success.
 * @retval else                 Error.
 *
 */
/**@snippet [Handling the config store] */
int selfmesh_util_store_cfg(uint8_t* cfg_ptr, int cfg_len, uint8_t flush)
{
    uint8_t* temp_ptr = (uint8_t*)s_selfmesh_util_node_p;
    if(NULL == s_selfmesh_util_node_p || cfg_ptr == NULL)
    {
        return 1;
    }
    if(cfg_ptr < (uint8_t* )s_selfmesh_util_node_p || (cfg_ptr+cfg_len > ((uint8_t *)s_selfmesh_util_node_p)+sizeof(selfmesh_node_t)))
    {
        return 1;
    }
    return selfmesh_store(s_selfmesh_util_node_p, (cfg_ptr-temp_ptr), cfg_len, flush);
}
/**@brief Function for system reset.
 *
 * @param[in]
 * @retval
 *
 */
/**@snippet [Handling the system reset] */
void selfmesh_util_system_reset(void)
{
    selfmesh_reset();
}

