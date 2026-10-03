/**
 * Copyright (c) 2017 - 2019, DFT Tech Co.,Ltd
 *
 * All rights reserved.
 *
 *
 */

#ifndef SELFMESH_SERVICE_INTER_H
#define SELFMESH_SERVICE_INTER_H
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "selfmesh_common.h"
#include "selfmesh_tag.h"
#include "selfmesh_service.h"

/**@brief Function of the service init.
 *
 * @param[in]  start_mode       The system start mode.
 * @param[in]  element          The element id of the service.
 * @param[io]  cfg_set_ptr      The service config data.
 * @retval 0                    Init success.
 * @retval else                 Error.
 *
 */
/**@snippet [Handling the service init] */
typedef int (* selfmesh_service_int_init)(uint8_t start_mode, uint8_t element, selfmesh_service_t* cfg_set_ptr);

/**@brief Function of the service to handle the data from the net.
 *
 * @param[in]  source_addr      The address of the node which send the command the the service.
 * @param[in]  element          The element id of the service.
 * @param[in]  data_ptr         The data get from the mesh net.
 * @param[io]  cfg_set_ptr      The service config data.
 * @retval 0                    The data is processed successfully by the service.
 * @retval 1                    The data can not be processed by the service.
 * @retval 2                    Error.
 *
 */
/**@snippet [Handling the data received from the net] */
typedef int (* selfmesh_service_int_data_handle)(selfmesh_addr_t source_addr, uint8_t element, uint8_t* data_ptr, selfmesh_service_t* cfg_set_ptr);

/**@brief Function for service schedule.
 *
 * @param[in]  element          The element id of the service.
 * @param[io]  cfg_set_ptr      The service config data.
 * @retval 
 *
 */
/**@snippet [Handling the service schedule] */
typedef void (* selfmesh_service_int_schedule)(uint8_t element, selfmesh_service_t* cfg_set_ptr);


typedef struct
{
    uint16_t                          service_uuid;
    selfmesh_service_int_init         service_init;
    selfmesh_service_int_data_handle  app_data_handle;
    selfmesh_service_int_schedule     sche_handle;
} selfmesh_service_handle_t;

/**@brief Function of the service to handle the data from the net.
 *
 * @param[in]  element          The element id of the service.
 * @param[io]  cfg_set_ptr      The service config data.
 * @retval 0                    Init success.
 * @retval else                 Error.
 *
 */
/**@snippet [Handling the service init] */
extern int selfmesh_service_0000_init(uint8_t start_mode, uint8_t element, selfmesh_service_t* cfg_set_ptr);

/**@brief Function of the service to handle the data from the net.
 *
 * @param[in]  source_addr      The address of the node which send the command the the service.
 * @param[in]  element          The element id of the service.
 * @param[in]  data_ptr         The data get from the mesh net.
 * @param[io]  cfg_set_ptr      The service config data.
 * @retval 0                    The data is processed successfully by the service.
 * @retval 1                    The data can not be processed by the service.
 * @retval 2                    Error.
 *
 */
/**@snippet [Handling the data received from the net] */
extern int selfmesh_service_0000_handle(selfmesh_addr_t source_addr, uint8_t element, uint8_t* data_ptr, selfmesh_service_t* cfg_set_ptr);

/**@brief Function for service schedule.
 *
 * @param[in]  element          The element id of the service.
 * @param[io]  cfg_set_ptr      The service config data.
 * @retval 
 *
 */
/**@snippet [Handling the service schedule] */
extern void selfmesh_service_0000_sche(uint8_t element, selfmesh_service_t* cfg_set_ptr);


#endif //SELFMESH_SERVICE_INTER_H

