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
#include "selfmesh_element.h"
#include "selfmesh_service.h"
#include "selfmesh_tag.h"

static const uint8_t   default_net_key[SELFMESH_NETKEY_LENGTH] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t   default_app_key[SELFMESH_APPKEY_LENGTH] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/**@brief Function of the node init.
 *
 * @param[out] cfg_set_ptr      The node config data.
 * @retval 0                    Init success.
 * @retval else                 Error.
 *
 */
/**@snippet [Handling the element init] */
int selfmesh_node_config_default(selfmesh_node_t* cfg_set_ptr)
{
    int temp_e_index = 0;
    cfg_set_ptr->snd_indx = 0;
    cfg_set_ptr->start_mode = SELFMESH_START_MODE_CONFIG;
    cfg_set_ptr->unicast_addr = SELFMESH_UNICAST_ADDRESS_INVALIDE;
    memcpy(cfg_set_ptr->net_key, default_net_key, SELFMESH_NETKEY_LENGTH);
    for(temp_e_index = 0; temp_e_index < SELFMESH_MAX_ELEMENT_CNT; temp_e_index++)
    {
        cfg_set_ptr->element_a[temp_e_index].magic_value = SELFMESH_ELEMENT_MAGIC_VALUE;
        cfg_set_ptr->element_a[temp_e_index].pub_addr = SELFMESH_INVALIDE_ADDRESS;
        memcpy(cfg_set_ptr->element_a[temp_e_index].app_key, default_app_key, SELFMESH_APPKEY_LENGTH);
        for(int i=0; i<SELFMESH_MAX_SUB_ADDRESS_PER_ELEMENT; i++)
        {
            cfg_set_ptr->element_a[temp_e_index].sub_addr[i] = SELFMESH_INVALIDE_ADDRESS;
        }
        for(int i=0; i<SELFMESH_MAX_SERVICE_PER_ELEMENT; i++)
        {
            cfg_set_ptr->element_a[temp_e_index].service_a[i].magic_value = SELFMESH_SERVICE_MAGIC_VALUE;
            cfg_set_ptr->element_a[temp_e_index].service_a[i].service_uuid = SELEMESH_SERVICE_UUID0;
            memset(cfg_set_ptr->element_a[temp_e_index].service_a[i].service_cfg, 0xFF, SELFMESH_MAX_VALUE_PER_SERVICE);
        }
    }
    cfg_set_ptr->element_a[0].service_a[0].service_uuid = SELEMESH_SERVICE_UUID0;
    return 0;
}

