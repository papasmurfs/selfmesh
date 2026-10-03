/**
 * Copyright (c) 2017 - 2019, DFT Tech Co.,Ltd
 *
 * All rights reserved.
 *
 *
 */



#ifndef SELFMESH_NODE_H
#define SELFMESH_NODE_H
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "selfmesh_common.h"
#include "selfmesh_element.h"
//#pragma pack(1) 
#define SELFMESH_NODE_MAGIC_VALUE   0x5A5A

typedef struct
{
    uint8_t                msg_index;
}selfmesh_msg_history_t;

typedef struct
{
    uint16_t                magic_value;
    uint8_t                 reserved0[6];

    uint8_t                 start_mode;
    uint8_t                 net_key[SELFMESH_NETKEY_LENGTH];
    uint8_t                 reserved1;

    selfmesh_addr_t         unicast_addr;
    selfmesh_element_t      element_a[SELFMESH_MAX_ELEMENT_CNT];

    /* for align */
    uint8_t                 reserved[2];
    uint16_t                magic_value2;

    uint8_t                 snd_indx;
    uint8_t                 reserved2;
} selfmesh_node_t;

/**@brief Function of the node init.
 *
 * @param[out] cfg_set_ptr      The node config data.
 * @retval 0                    Init success.
 * @retval else                 Error.
 *
 */
/**@snippet [Handling the element init] */
extern int selfmesh_node_config_default(selfmesh_node_t* cfg_set_ptr);

/**@brief Function of the node init.
 *
 * @param[io]  cfg_set_ptr      The node config data.
 * @retval 0                    Init success.
 * @retval else                 Error.
 *
 */
/**@snippet [Handling the element init] */
extern int selfmesh_node_init(selfmesh_node_t* cfg_set_ptr);

/**@brief Function of the node to pretreat the data from the net.
 **@brief To decrypt the data and check the magic value and the duplicate.
 *
 * @param[in]  data_ptr         The data get from the mesh net, the length must be 20 bytes.
 * @param[io]  cfg_set_ptr      The node config data.
 * @param[out] out_ptr          The pretreat output, the length must be 20 bytes.
 * @retval 0                    The data is processed successfully by the element and the data can be retrain.
 * @retval 1                    The data is invalid.
 * @retval 2                    The data is duplicate.
 *
 */
/**@snippet [Handling the data received from the net] */
extern int selfmesh_node_pretreat(const uint8_t* data_ptr, selfmesh_node_t* cfg_set_ptr, uint8_t* out_ptr);

/**@brief Function of the node to handle the data from the net.
 *
 * @param[in]  data_ptr         The data get from the mesh net.
 * @param[io]  cfg_set_ptr      The node config data.
 * @retval 0                    The data is processed successfully by the element and the data can be retrain.
 * @retval 1                    The data is invalid.
 * @retval 2                    The data is duplicate.
 * @retval 10                   The data is processed successfully by the element and the data should not be retrain.
 *
 */
/**@snippet [Handling the data received from the net] */
extern int selfmesh_node_handle(uint8_t* data_ptr, selfmesh_node_t* cfg_set_ptr);

/**@brief Function for node schedule.
 *
 * @param[io]  cfg_set_ptr      The node config data.
 * @retval 
 *
 */
/**@snippet [Handling the node schedule] */
extern void selfmesh_node_sche(selfmesh_node_t* cfg_set_ptr);
#endif //SELFMESH_NODE_H

