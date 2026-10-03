/**
 * Copyright (c) 2017 - 2019, DFT Tech Co.,Ltd
 *
 * All rights reserved.
 *
 *
 */

#ifndef SELFMESH_ELEMENT_H
#define SELFMESH_ELEMENT_H
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "selfmesh_common.h"
#include "selfmesh_service.h"

#define SELFMESH_ELEMENT_MAGIC_VALUE   0x5A5A

//#pragma pack(1) 
typedef struct
{
    uint16_t             magic_value;
    selfmesh_addr_t      pub_addr;
    selfmesh_addr_t      sub_addr[SELFMESH_MAX_SUB_ADDRESS_PER_ELEMENT];
    uint8_t              app_key[SELFMESH_APPKEY_LENGTH];
    selfmesh_service_t   service_a[SELFMESH_MAX_SERVICE_PER_ELEMENT];
} selfmesh_element_t;


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
extern int selfmesh_element_init(uint8_t start_mode, uint8_t element, selfmesh_element_t* cfg_set_ptr);

/**@brief Function of the element to handle the data from the net.
 *
 * @param[in]  source_addr      The address of the node which send the command to the element.
 * @param[in]  dest_addr        The dest address of the command.
 * @param[in]  data_enc         1:data need to be decrypt first; 0:raw data.
 * @param[in]  element          The element id of the element.
 * @param[in]  data_ptr         The data get from the mesh net.
 * @param[io]  cfg_set_ptr      The element config data.
 * @retval 0                    The data is processed successfully by the element.
 *
 */
/**@snippet [Handling the data received from the net] */
extern int selfmesh_element_handle(selfmesh_addr_t source_addr, selfmesh_addr_t dest_addr, uint8_t element, uint8_t data_enc, uint8_t* data_ptr, selfmesh_element_t* cfg_set_ptr);

/**@brief Function for element schedule.
 *
 * @param[in]  element          The element id of the element.
 * @param[io]  cfg_set_ptr      The element config data.
 * @retval 
 *
 */
/**@snippet [Handling the element schedule] */
extern void selfmesh_element_sche(uint8_t element, selfmesh_element_t* cfg_set_ptr);


#endif //SELFMESH_ELEMENT_H

