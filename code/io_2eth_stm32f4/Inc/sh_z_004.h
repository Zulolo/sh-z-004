#ifndef _SH_Z_004_H
#define _SH_Z_004_H

#include "sh_z.h"

#define SH_Z_004_SLAVE_ID							2
#define SH_Z_004_DI_NUM								4
#define SH_Z_004_DO_NUM								4
#define SH_Z_004_DI_PIN_OFFSET				8

#define SH_Z_004_VERSION							((uint16_t)0x0100)

/* xComEventGroup bits */
#define EG_EXT_FLASH_SPI_DMA_DONE_BIT		(0x01 << 0)
#define EG_ETH_NETIF_UP_BIT							(0x01 << 1)

#define SH_Z_004_INFO_FILE_NAME				"info.json"
#define SH_Z_004_ETH_CONF_FILE_NAME		"conf.json"

#define CONF_STATIC_IP_JSON_TAG				"static_ip"
#define CONF_IP_ADDR_JSON_TAG					"ip_addr"
#define CONF_NETMASK_JSON_TAG					"netmask"
#define CONF_GW_ADDR_JSON_TAG					"gw_addr"
#define CONF_MAC_ADDR_JSON_TAG				"mac_addr"


#endif
