/**
 * Copyright (c) 2017 - 2019, DFT Tech Co.,Ltd
 *
 * All rights reserved.
 *
 *
 */



#ifndef SELFMESH_COMMON_H
#define SELFMESH_COMMON_H

typedef unsigned short     int selfmesh_addr_t;
typedef unsigned short     int selfmesh_tag_t;
typedef unsigned short     int selfmesh_pvid_t;
#define SELFMESH_MSG_PRETREAT                1
#define SELFMESH_ALLOW_RAW_DATA              1
#define SELFMESH_STACK_LOG_OUT               1
#define SELFMESH_STACK_LOG_MAX               80

#define SELFMESH_NODE_PID                    0x0002
#define SELFMESH_NODE_VID                    0x0000

#define SELFMESH_MAN_ID_M                    0xCD
#define SELFMESH_MAN_ID_L                    0xFB

#define SELFMESH_START_MODE_CONFIG           0
#define SELFMESH_START_MODE_NORMAL           1

#define SELFMESH_MSG_MAGIC_VAL_M             0xDB

#define SELFMESH_MSG_MAX_HOP                 10
#define SELFMESH_MSG_DEFAULT_HOP             1

#define SELFMESH_MAXNODE_PER_NET             0xF0

#define SELFMESH_UD_BUF_SIZE                 20

#define SELFMESH_MAX_ELEMENT_CNT             1
#define SELFMESH_NETKEY_LENGTH               8
#define SELFMESH_APPKEY_LENGTH               8
#define SELFMESH_MAX_SUB_ADDRESS_PER_ELEMENT 3 /*node: do not modify the subscrip address count*/

#define SELFMESH_MAX_SERVICE_PER_ELEMENT     1

#define SELFMESH_MAX_VALUE_PER_SERVICE       200

/**The mesh pdu is defined:
 *|magicM(8b)|magicL(4b)|flag(4b)|index(8b)|source addrM(8b)|source addrL(8b)|dest addrM(8b)|dest addrL(8b)|element(8b)|appdata(96b)|
 */
#define SELFMASH_MAGIC_M_OFFSET              0
#define SELFMASH_HOP_CNT_OFFSET              1
#define SELFMASH_FLAG_BITS                   4
#define SELFMASH_INDEX_OFFSET                2
#define SELFMASH_SOURCE_ADDR_M_OFFSET        3
#define SELFMASH_SOURCE_ADDR_L_OFFSET        4
#define SELFMASH_DEST_ADDR_M_OFFSET          5
#define SELFMASH_DEST_ADDR_L_OFFSET          6
#define SELFMASH_ELEMENT_OFFSET              7
#define SELFMASH_APP_DATA_OFFSET             8
#define SELFMASH_APP_DATA_LEN                (SELFMESH_UD_BUF_SIZE - SELFMASH_APP_DATA_OFFSET)

#define SELFMESH_FLAG_USE_NETKEY_MASK        0x02
#define SELFMESH_FLAG_USE_APPKEY_MASK        0x01

#define SELFMESH_INDEX_MAX                   0xFF

/**The unicast address of the node is from 0x0000--0x00F0
 * 0x00FF is the invalide address
 * The else are for futher use*/
#define SELFMESH_UNICAST_ADDRESS_START       0x0000
#define SELFMESH_UNICAST_ADDRESS_PROVISION   0x0000
#define SELFMESH_UNICAST_ADDRESS_END         0x00F0
#define SELFMESH_UNICAST_ADDRESS_INVALIDE    0x00FF

/**The group address of the net is from 0xD000--0xDFF0*/
#define SELFMESH_GOUP_ADDRESS_START          0xD000
#define SELFMESH_GOUP_ADDRESS_END            0xDFF0
#define SELFMESH_GOUP_ADDRESS_ALL            0xD000
#define SELFMESH_GOUP_ADDRESS_INVALIDE       0xDFFF

/**The virtual address of the net is from 0xE000--0xEFF0*/
#define SELFMESH_VIRTUAL_ADDRESS_START       0xE000
#define SELFMESH_VIRTUAL_ADDRESS_END         0xEFF0
#define SELFMESH_VIRTUAL_ADDRESS_INVALIDE    0xEFFF

#define SELFMESH_CURRENT_ADDRESS             0xFFF0

#define SELFMESH_INVALIDE_ADDRESS            0xFFFF

/**The element id 0x00--0xF0*/
#define SELFMESH_ELEMENT_ID_START            0x00
#define SELFMESH_ELEMENT_ID_END              0xF0
#define SELFMESH_ELEMENT_ID_INVALIDE         0xFF
#define SELFMESH_ELEMENT_ID_ALL              0xFE

#define SELFMESH_ELEMENT_ID_PROVISION        0x00

/**scene setup server*/
#define SELFMESH_SCENE_MAX_CNT               20

#endif //SELFMESH_COMMON_H

