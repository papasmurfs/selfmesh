/**
 * Copyright (c) 2017 - 2019, DFT Tech Co.,Ltd
 *
 * All rights reserved.
 *
 *
 */

#ifndef SELFMESH_SERVICE_H
#define SELFMESH_SERVICE_H
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "selfmesh_common.h"
#include "selfmesh_tag.h"

#define SELFMESH_SERVICE_MAGIC_VALUE   0x5A5A

typedef enum
{
    SELEMESH_SERVICE_UUID0    = 0,            /**UUID for scene clint setup server 0. */
    SELEMESH_SERVICE_UUID1    = 1,            /**UUID for light control service1. */
    SELEMESH_SERVICE_UUIDMAX
}selfmesh_service_uuid_e;

//#pragma pack(1) 
typedef struct
{
    uint16_t   magic_value;
    uint16_t   service_uuid;
    uint8_t    service_cfg[SELFMESH_MAX_VALUE_PER_SERVICE];
} selfmesh_service_t;

/**@brief Function of the service to handle the data from the net.
 *
 * @param[in]  start_mode       The system start mode.
 * @param[in]  element          The element id of the service.
 * @param[in]  uuid             The service uuid of the service.
 * @param[io]  cfg_set_ptr      The service config data.
 * @retval 0                    Init success.
 * @retval else                 Error.
 *
 */
/**@snippet [Handling the service init] */
extern int selfmesh_service_init(uint8_t start_mode, uint8_t element, uint16_t uuid, selfmesh_service_t* cfg_set_ptr);

/**@brief Function of the service to handle the data from the net.
 *
 * @param[in]  source_addr      The address of the node which send the command the the service.
 * @param[in]  element          The element id of the service.
 * @param[in]  uuid             The service uuid of the service.
 * @param[in]  data_ptr         The data get from the mesh net.
 * @param[io]  cfg_set_ptr      The service config data.
 * @retval 0                    The data is processed successfully by the service.
 * @retval 1                    The data can not be processed by the service.
 * @retval 2                    Error.
 *
 */
/**@snippet [Handling the data received from the net] */
extern int selfmesh_service_handle(selfmesh_addr_t source_addr, uint8_t element, uint16_t uuid, uint8_t* data_ptr, selfmesh_service_t* cfg_set_ptr);

/**@brief Function for service schedule.
 *
 * @param[in]  element          The element id of the service.
 * @param[in]  uuid             The service uuid of the service.
 * @param[io]  cfg_set_ptr      The service config data.
 * @retval 
 *
 */
/**@snippet [Handling the service schedule] */
extern void selfmesh_service_sche(uint8_t element, uint16_t uuid, selfmesh_service_t* cfg_set_ptr);


#endif //SELFMESH_SERVICE_H

