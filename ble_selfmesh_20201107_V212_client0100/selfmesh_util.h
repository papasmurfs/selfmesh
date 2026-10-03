/**
 * Copyright (c) 2017 - 2019, DFT Tech Co.,Ltd
 *
 * All rights reserved.
 *
 *
 */



#ifndef SELFMESH_UTIL_H
#define SELFMESH_UTIL_H
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "selfmesh_common.h"
#include "selfmesh_node.h"


/**@brief Function of the util init.
 *
 * @param[in]  start_mode       The system start mode.
 * @param[out] cfg_set_ptr      The node config data.
 * @param[out] msg_history_p    The node message history.
 * @retval 0                    Init success.
 * @retval else                 Error.
 *
 */
/**@snippet [Handling the util init] */
extern int selfmesh_util_init(uint8_t start_mode, selfmesh_node_t* cfg_set_ptr, selfmesh_msg_history_t* msg_history_p);

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
extern int selfmesh_util_send_app_msg(selfmesh_addr_t dest_addr, uint8_t source_element, uint8_t dest_element, uint8_t hop, uint8_t* msg_ptr, uint8_t msg_len);

/**@brief Function of the net message send for next hop.
 *
 * @param[in] ud_ptr            The user data.
 * @retval 0                    send success.
 * @retval else                 Error.
 *
 */
/**@snippet [Handling the net messget send for next hop] */
extern int selfmesh_util_send_msg_next_hop(uint8_t* ud_ptr);

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
extern int selfmesh_util_store_cfg(uint8_t* cfg_ptr, int cfg_len, uint8_t flush);

/**@brief Function for system reset.
 *
 * @param[in]
 * @retval
 *
 */
/**@snippet [Handling the system reset] */
extern void selfmesh_util_system_reset(void);
#endif //SELFMESH_UTIL_H

