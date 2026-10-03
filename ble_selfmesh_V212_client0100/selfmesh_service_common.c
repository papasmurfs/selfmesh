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


static const selfmesh_service_handle_t selfmesh_servcie_list[] =
{
    {SELEMESH_SERVICE_UUID0, selfmesh_service_0000_init, selfmesh_service_0000_handle, selfmesh_service_0000_sche},
    {SELEMESH_SERVICE_UUIDMAX, NULL, NULL, NULL},
};


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
int selfmesh_service_init(uint8_t start_mode, uint8_t element, uint16_t uuid, selfmesh_service_t* cfg_set_ptr)
{
    int index = 0;
    do
    {
        if(selfmesh_servcie_list[index].service_uuid == SELEMESH_SERVICE_UUIDMAX)
        {
            break;
        }
        else if(selfmesh_servcie_list[index].service_uuid == uuid)
        {
            if(selfmesh_servcie_list[index].service_init != NULL)
            {
                (selfmesh_servcie_list[index].service_init)(start_mode, element, cfg_set_ptr);
            }
            break;
        }
        index++;
    }while(1);
    return 0;
}

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
int selfmesh_service_handle(selfmesh_addr_t source_addr, uint8_t element, uint16_t uuid, uint8_t* data_ptr, selfmesh_service_t* cfg_set_ptr)
{
    int index = 0;
    do
    {
        if(selfmesh_servcie_list[index].service_uuid == SELEMESH_SERVICE_UUIDMAX)
        {
            break;
        }
        else if(selfmesh_servcie_list[index].service_uuid == uuid)
        {
            if(selfmesh_servcie_list[index].app_data_handle != NULL)
            {
                (selfmesh_servcie_list[index].app_data_handle)(source_addr, element, data_ptr, cfg_set_ptr);
            }
            break;
        }
        index++;
    }while(1);
    return 0;
}


/**@brief Function for service schedule.
 *
 * @param[in]  element          The element id of the service.
 * @param[in]  uuid             The service uuid of the service.
 * @param[io]  cfg_set_ptr      The service config data.
 * @retval 
 *
 */
/**@snippet [Handling the service schedule] */
void selfmesh_service_sche(uint8_t element, uint16_t uuid, selfmesh_service_t* cfg_set_ptr)
{
    int index = 0;
    do
    {
        if(selfmesh_servcie_list[index].service_uuid == SELEMESH_SERVICE_UUIDMAX)
        {
            break;
        }
        else if(selfmesh_servcie_list[index].service_uuid == uuid)
        {
            if(selfmesh_servcie_list[index].sche_handle != NULL)
            {
                (selfmesh_servcie_list[index].sche_handle)(element, cfg_set_ptr);
            }
            break;
        }
        index++;
    }while(1);
    return;
}


