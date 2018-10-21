#ifndef _UTILITY_H
#define _UTILITY_H
#include "cmsis_os.h"
#include "cJSON.h"
#include "sh_z.h"
#include "sh_z_002.h"

void UTL_sh_z_004_info_init(void);
void UTL_sh_z_eth_conf_init(void);
int UTL_create_default_eth_conf(void);
int UTL_create_byte_array_json(cJSON* pJsonWriter, char* pNodeName, uint8_t* pByteArray, uint32_t uArrayLength);
int UTL_get_byte_from_array_json(cJSON* pArrayJson, uint8_t* pByteArray, uint32_t uArrayLength);
int UTL_save_eth_conf(ETH_Conf_t* pEthConf);

#endif

