/**
 * Copyright (c) 2017 - 2019, DFT Tech Co.,Ltd
 *
 * All rights reserved.
 *
 *
 */

#ifndef SELFMESH_TAG_H
#define SELFMESH_TAG_H
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "selfmesh_common.h"

//node: Tag id form 0x0000 ~ 0x00FF is reserved for net management
typedef enum
{
    SELFMESH_TAG_SET_LOC_ADDR            = 0x0000,          /**Tag for local address set. 
	                                                           * 000002xxxx-to set the local address, response pdu is 00010100*/
    SELFMESH_TAG_SET_LOC_ADDR_RSP        = 0x0001,
    SELFMESH_TAG_GET_LOC_ADDR            = 0x0002,          /**Tag for local address get. 
	                                                           * 000200-to get current local address, response pdu is 000302xxxx*/
    SELFMESH_TAG_GET_LOC_ADDR_RSP        = 0x0003,
    SELFMESH_TAG_SET_NETKEY              = 0x0004,          /**Tag for NETKEY set. 
	                                                           * 000408xxxxxxxxxxxxxxxx-to set the netkey, response pdu is 00050100*/
    SELFMESH_TAG_SET_NETKEY_RSP          = 0x0005,
    SELFMESH_TAG_GET_NETKEY              = 0x0006,          /**Tag for NETKEY get. 
	                                                           * 000600-to get the netkey, response pdu is 000708xxxxxxxxxxxxxxxx*/
    SELFMESH_TAG_GET_NETKEY_RSP          = 0x0007,
    SELFMESH_TAG_GET_PIDVID              = 0x0008,          /**Tag for PID/VID get. 
	                                                           * 000800-to get the netkey, response pdu is 000904ppppvvvv*/
    SELFMESH_TAG_GET_PIDVID_RSP          = 0x0009,
    SELFMESH_TAG_SYSTEM_RESET            = 0x000A,          /**Tag for system reset. 
	                                                           * 000A01rr-to reset the system, rr reset mode 0-reset for config mode 1-reset for normal mode
                                                             * no response*/
    SELFMESH_TAG_OPEN_PROXY_CHANNEL      = 0x000B,          /**Tag for proxy channel open. 
	                                                           * 000Blldddddddddddd-to open proxy channel, ll parameter length, dd dest address for advertisement
                                                             * no response*/

    SELFMESH_TAG_ELEMENT_MANAGE_START    = 0x0080,

    SELFMESH_TAG_SET_APPKEY              = 0x0080,          /**Tag for APPKEY set. 
	                                                           * 008008xxxxxxxxxxxxxxxx-to set the appkey, response pdu is 00810100*/
    SELFMESH_TAG_SET_APPKEY_RSP          = 0x0081,
    SELFMESH_TAG_GET_APPKEY              = 0x0082,          /**Tag for APPKEY get. 
	                                                           * 008200-to get the appkey, response pdu is 008308xxxxxxxxxxxxxxxx*/
    SELFMESH_TAG_GET_APPKEY_RSP          = 0x0083,
    SELFMESH_TAG_SET_PUB_ADDR            = 0x0084,          /**Tag for public address set. 
	                                                           * 008402xxxx-to set the public address, response pdu is 00850100*/
    SELFMESH_TAG_SET_PUB_ADDR_RSP        = 0x0085,
    SELFMESH_TAG_GET_PUB_ADDR            = 0x0086,          /**Tag for public address get. 
	                                                           * 008600-to get current public address, response pdu is 008702xxxx*/
    SELFMESH_TAG_GET_PUB_ADDR_RSP        = 0x0087,
    SELFMESH_TAG_SET_SUB_ADDR            = 0x0088,          /**Tag for subscrip address set. 
	                                                           * 008806xxxxyyyyzzzz-to set the subscrip address, response pdu is 00890100*/
    SELFMESH_TAG_SET_SUB_ADDR_RSP        = 0x0089,
    SELFMESH_TAG_GET_SUB_ADDR            = 0x008A,          /**Tag for subscrip address get. 
	                                                           * 008A00-to get current subscrip address, response pdu is 008B06xxxxyyyyzzzz*/
    SELFMESH_TAG_GET_SUB_ADDR_RSP        = 0x008B,

    SELFMESH_TAG_APP_SERVICE_START       = 0x0100,

    SELFMESH_TAG_NODE_IDENTIFY           = 0x0100,          /**Tag for node identify, note: the element id must be 0xFE. 
	                                                           * 010001pp-to identify the node, pp is the parameter,
	                                                           * response pdu is 01010100*/
    SELFMESH_TAG_NODE_IDENTIFY_RSP       = 0x0101,

    SELFMESH_TAG_SIMPLE_CLIENT_SET       = 0x0102,          /**Tag for simple client config data set. 
	                                                           * 0100lleeccccccc-to set the command, ll is the length, ee is the element, cc is the command,
	                                                           * response pdu is 01030100*/
    SELFMESH_TAG_SIMPLE_CLIENT_SET_RSP   = 0x0103,
    SELFMESH_TAG_SIMPLE_CLIENT_GET       = 0x0104,          /**Tag for simple client get. 
	                                                           * 010200-to get current simple client config data,
	                                                           * response pdu is 0105lleeccccccc, ll is the length, ee is the element, cc is the command*/
    SELFMESH_TAG_SIMPLE_CLIENT_GET_RSP   = 0x0105,
    SELFMESH_TAG_SCENE_LIST_CNT_GET      = 0x0106,          /**Tag for the scene LIST count get. 
	                                                           * 010600-to get the scene table max row,
	                                                           * response pdu is 010703rrccmm, rr is the result, cc is support scene count, mm is the max scene list length*/
    SELFMESH_TAG_SCENE_LIST_CNT_GET_RSP  = 0x0107,
    SELFMESH_TAG_SCENE_LIST_GET          = 0x0108,          /**Tag to get the scene config from the scene list. 
	                                                           * 010801ii-to Get the scene id, ii is the list index,from 0,
	                                                           * response pdu is 010907rrsssspppppppp, rr is the result, 00-success, 01 over flow, 02-invalid scene, ssss is the scene id, pp is the scene config data*/
    SELFMESH_TAG_SCENE_LIST_GET_RSP      = 0x0109,
    SELFMESH_TAG_SCENE_REMOVE            = 0x010A,          /**Tag to remove the scene id from the scene list. 
	                                                           * 010A02ssss-to remove the scene id, ssss is the scene id,
	                                                           * response pdu is 010B01rr, rr is the result, 00-success, 01-invalide scene id, 02-the scene id is not in the list*/
    SELFMESH_TAG_SCENE_REMOVE_RSP        = 0x010B,
    SELFMESH_TAG_SCENE_PARAMETER_SET     = 0x010C,          /**Tag for scene config data set. 
	                                                           * 010Cllsssspppppppp-to set the config of the scene, ll is the length, ss is the scene id, pp is the parameter,
	                                                           * response pdu is 010D02rrii, rr is the result-00:success, 01:invalide scene id, 02:table overflow, ii is the row index*/
    SELFMESH_TAG_SCENE_PARAMETER_SET_RSP = 0x010D,
    SELFMESH_TAG_SCENE_PARAMETER_GET     = 0x010E,          /**Tag for scene config data get. 
	                                                           * 010E02ssss-to get the config of the scene,ss is the scene id, 
	                                                           * response pdu is 010Fllrrpppppppp, ll is the length, rr is the result-00:success, 01:invalide scene id, 02:table overflow, pp is the parameter*/
    SELFMESH_TAG_SCENE_PARAMETER_GET_RSP = 0x010F,
    SELFMESH_TAG_SCENE_PUBLISH_ID        = 0x0110,          /**Tag for publish the scene id. 
	                                                           * 011002ssss-to publish the scene id,ss is the scene id, 
	                                                           * no response*/
    SELFMESH_TAG_LIGHTNESS_PUBLISH       = 0x0111,          /**Tag for publish the lightness up/down command. 
	                                                           * 0111llccllll-to publish the lightness command,cc 0-up,1-down,2-switch,3-on,4-off,5-set light
	                                                           * no response*/

    SELEMESH_TAG_MAX
}selfmesh_service_tag_e;

#endif //SELFMESH_TAG_H

