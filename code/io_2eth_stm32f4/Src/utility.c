#include "main.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "lwip.h"
#include "cJSON.h"
#include "spiffs.h"
#include "lwip/apps/tftp_server.h"
#include "sh_z_004.h"

extern osMutexId WebServerFileMutexHandle;
extern spiffs SPI_FFS_fs;

char SH_Z_004_SN[SH_Z_SN_LEN + 1] = "SHZ002.201809190";
ETH_Conf_t tEthConf;

static void* tftp_file_open(const char* fname, const char* mode, u8_t write) {
	spiffs_file nFileHandle;
	osMutexWait(WebServerFileMutexHandle, osWaitForever);
	if (write) {
		nFileHandle = SPIFFS_open(&SPI_FFS_fs, fname, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
		printf("errno %d\n", SPIFFS_errno(&SPI_FFS_fs));
	} else {
		nFileHandle = SPIFFS_open(&SPI_FFS_fs, fname, SPIFFS_RDONLY, 0);
	}
	if (nFileHandle <= 0 ) {
		osMutexRelease(WebServerFileMutexHandle);
		return NULL;
	} else {
		return ((void*)((uint32_t)nFileHandle));
	}	
}

static void tftp_file_close(void* handle) {
	SPIFFS_close(&SPI_FFS_fs, (spiffs_file)handle);
	osMutexRelease(WebServerFileMutexHandle);
}

static int tftp_file_read(void* handle, void* buf, int bytes) {
	int res;
	res = SPIFFS_read(&SPI_FFS_fs, (spiffs_file)handle, (u8_t *)buf, bytes);
	return res;
}

static int tftp_file_write(void* handle, struct pbuf* p) {
	return SPIFFS_write(&SPI_FFS_fs, (spiffs_file)handle, p->payload, p->len);
}

const struct tftp_context TFTP_Ctx = {.open = tftp_file_open, .close = tftp_file_close, .read = tftp_file_read, .write = tftp_file_write};

static int create_default_sh_z_002_info(void) {
	spiffs_file tFileDesc;
  char* pJsonString = NULL;
  cJSON* pSN = NULL;
	cJSON* pDI_ConfJsonWriter;
	
	tFileDesc = SPIFFS_open(&SPI_FFS_fs, SH_Z_004_INFO_FILE_NAME, SPIFFS_RDWR | SPIFFS_CREAT, 0);

	pDI_ConfJsonWriter = cJSON_CreateObject();
	if (pDI_ConfJsonWriter == NULL){
		SPIFFS_close(&SPI_FFS_fs, tFileDesc);
		printf("failed to create json root object.\n");
		return (-1);
	}

	pSN = cJSON_CreateString("123456789ABCDEFG");
	if (pSN == NULL){
		SPIFFS_close(&SPI_FFS_fs, tFileDesc);
		cJSON_Delete(pDI_ConfJsonWriter);
		printf("failed to create json latch set object.\n");
		return (-1);
	}	
	cJSON_AddItemToObject(pDI_ConfJsonWriter, DEVICE_SN_JSON_TAG, pSN);
	
	pJsonString = cJSON_Print(pDI_ConfJsonWriter);
	if (pJsonString == NULL){
		SPIFFS_close(&SPI_FFS_fs, tFileDesc);
		cJSON_Delete(pDI_ConfJsonWriter);
		printf("failed to digest json object.\n");
		return (-1);
	}
	
	if (SPIFFS_write(&SPI_FFS_fs, tFileDesc, pJsonString, strlen(pJsonString)) < 0 ) {
		SPIFFS_close(&SPI_FFS_fs, tFileDesc);
		cJSON_Delete(pDI_ConfJsonWriter);
		free(pJsonString);
		printf("failed to write digested json string to file.\n");
		return (-1);		
	}
	SPIFFS_close(&SPI_FFS_fs, tFileDesc);
	cJSON_Delete(pDI_ConfJsonWriter);
	free(pJsonString);
	return (0);	
}

int UTL_create_byte_array_json(cJSON* pJsonWriter, char* pNodeName, uint8_t* pByteArray, uint32_t uArrayLength) {
	cJSON* pArray = NULL;
	cJSON* pSubItem = NULL;
  pArray = cJSON_AddArrayToObject(pJsonWriter, pNodeName);
	if (pArray == NULL) {
		return (-1);
	}

	for (int index = 0; index < uArrayLength; ++index) {
		pSubItem = cJSON_CreateObject();
		if (cJSON_AddNumberToObject(pSubItem, "index", index) == NULL) {
			return (-1);
		}

		if(cJSON_AddNumberToObject(pSubItem, "value", pByteArray[index]) == NULL) {
			return (-1);
		}
		cJSON_AddItemToArray(pArray, pSubItem);
	}
	return 0;
}

int UTL_save_eth_conf(ETH_Conf_t* pEthConf) {
	spiffs_file tFileDesc;
  char* pJsonString = NULL;
	int nRslt;
	cJSON* pEthConfJsonWriter;
	
	tFileDesc = SPIFFS_open(&SPI_FFS_fs, SH_Z_004_ETH_CONF_FILE_NAME, SPIFFS_RDWR | SPIFFS_CREAT, 0);
	
	pEthConfJsonWriter = cJSON_CreateObject();
	if (pEthConfJsonWriter == NULL){
		SPIFFS_close(&SPI_FFS_fs, tFileDesc);
		printf("failed to create json root object.\n");
		return (-1);
	}
	
	if (UTL_create_byte_array_json(pEthConfJsonWriter, CONF_MAC_ADDR_JSON_TAG, 
		tEthConf.uMAC_Addr, sizeof(tEthConf.uMAC_Addr)/sizeof(uint8_t)) < 0 ) {
		SPIFFS_close(&SPI_FFS_fs, tFileDesc);
		cJSON_Delete(pEthConfJsonWriter);
		printf("failed to create MAC json object.\n");
		return (-1);			
	}
	
	if (UTL_create_byte_array_json(pEthConfJsonWriter, CONF_IP_ADDR_JSON_TAG, 
		tEthConf.uIP_Addr, sizeof(tEthConf.uIP_Addr)/sizeof(uint8_t)) < 0 ) {
		SPIFFS_close(&SPI_FFS_fs, tFileDesc);
		cJSON_Delete(pEthConfJsonWriter);
		printf("failed to create IP json object.\n");
		return (-1);			
	}
		
	if (UTL_create_byte_array_json(pEthConfJsonWriter, CONF_NETMASK_JSON_TAG, 
		tEthConf.uNetmask, sizeof(tEthConf.uNetmask)/sizeof(uint8_t)) < 0 ) {
		SPIFFS_close(&SPI_FFS_fs, tFileDesc);
		cJSON_Delete(pEthConfJsonWriter);
		printf("failed to create netmask json object.\n");
		return (-1);			
	}
		
	if (UTL_create_byte_array_json(pEthConfJsonWriter, CONF_GW_ADDR_JSON_TAG, 
		tEthConf.uGateway, sizeof(tEthConf.uGateway)/sizeof(uint8_t)) < 0 ) {
		SPIFFS_close(&SPI_FFS_fs, tFileDesc);
		cJSON_Delete(pEthConfJsonWriter);
		printf("failed to create gateway json object.\n");
		return (-1);			
	}
	
	if (cJSON_AddBoolToObject(pEthConfJsonWriter, CONF_STATIC_IP_JSON_TAG, tEthConf.bStaticIP) == NULL) {
		SPIFFS_close(&SPI_FFS_fs, tFileDesc);
		cJSON_Delete(pEthConfJsonWriter);
		printf("failed to create static IP json object.\n");
		return (-1);
	}
	
	if (nRslt < 0) {
		SPIFFS_close(&SPI_FFS_fs, tFileDesc);
		cJSON_Delete(pEthConfJsonWriter);
		printf("failed to create ETH conf json object.\n");
		return (-1);		
	}
	
	pJsonString = cJSON_Print(pEthConfJsonWriter);
	if (pJsonString == NULL){
		SPIFFS_close(&SPI_FFS_fs, tFileDesc);
		cJSON_Delete(pEthConfJsonWriter);
		printf("failed to digest json object.\n");
		return (-1);
	}
	
	if (SPIFFS_write(&SPI_FFS_fs, tFileDesc, pJsonString, strlen(pJsonString)) < 0 ) {
		SPIFFS_close(&SPI_FFS_fs, tFileDesc);
		cJSON_Delete(pEthConfJsonWriter);
		free(pJsonString);
		printf("failed to write digested json string to file.\n");
		return (-1);		
	}
	SPIFFS_close(&SPI_FFS_fs, tFileDesc);
	cJSON_Delete(pEthConfJsonWriter);
	free(pJsonString);
	return (0);	
}
	
int UTL_create_default_eth_conf(void) {
	tEthConf.bStaticIP = 0;
	memset(tEthConf.uIP_Addr, 0, sizeof(tEthConf.uIP_Addr));
	memset(tEthConf.uNetmask, 0, sizeof(tEthConf.uNetmask));
	memset(tEthConf.uGateway, 0, sizeof(tEthConf.uGateway));
	// 02:80:E1:83:05:24
	tEthConf.uMAC_Addr[0] = 0x02;
	tEthConf.uMAC_Addr[1] = 0x80;
	tEthConf.uMAC_Addr[2] = 0xE1;
	tEthConf.uMAC_Addr[3] = 0x83;
	tEthConf.uMAC_Addr[4] = 0x05;
	tEthConf.uMAC_Addr[5] = 0x24;
	
	return UTL_save_eth_conf(&tEthConf);
}

static int load_sh_z_002_info(spiffs_file tFileDesc) {
  cJSON* pSN = NULL;
  cJSON* pSN_ConfJson;
	char cConfString[256];
	int nReadNum = SPIFFS_read(&SPI_FFS_fs, tFileDesc, cConfString, sizeof(cConfString));
	if ((nReadNum <= 0) || (sizeof(cConfString) == nReadNum )) {
		printf("%d char was read from conf file.\n", nReadNum);
		return (-1);		
	}

	pSN_ConfJson = cJSON_Parse(cConfString);
	if (pSN_ConfJson == NULL) {
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL){
				printf("Error before: %s\n", error_ptr);
		}
		return (-1);
	}

	pSN = cJSON_GetObjectItemCaseSensitive(pSN_ConfJson, DEVICE_SN_JSON_TAG);
	if (cJSON_IsString(pSN)){
		strncpy(SH_Z_004_SN, pSN->valuestring, sizeof(SH_Z_004_SN));
		SH_Z_004_SN[SH_Z_SN_LEN] = '\0';
	}	else {
		// TODO
	}
	
	cJSON_Delete(pSN_ConfJson);
	return 0;
}

int UTL_get_byte_from_array_json(cJSON* pArrayJson, uint8_t* pByteArray, uint32_t uArrayLength) {
	cJSON* pSubItem = NULL;
	cJSON* pIndex = NULL;
	cJSON* pValue = NULL;
	cJSON_ArrayForEach(pSubItem, pArrayJson){
		pValue = cJSON_GetObjectItem(pSubItem, "value");
		pIndex = cJSON_GetObjectItem(pSubItem, "index");
		if ((pValue != NULL) && (pIndex != NULL)) {
			if (pIndex->valueint < uArrayLength) {
				pByteArray[pIndex->valueint] = pValue->valueint;
			} else {
				// TODO
				return (-1);
			}
		} else {
			// TODO
			return (-1);
		}
	} 
	return 0;
}

static int load_sh_z_002_eth_conf(spiffs_file tFileDesc) {
	cJSON* pStatic = NULL;
  cJSON* pETH_ConfJson;
	char cConfString[1024];
	int nTotalRead = 0;
	memset(cConfString, 0, sizeof(cConfString));
	int nReadNum = SPIFFS_read(&SPI_FFS_fs, tFileDesc, cConfString, 256);
	nTotalRead += nReadNum;
	while ((nReadNum > 0) && (nTotalRead < sizeof(cConfString))) {
		nReadNum = SPIFFS_read(&SPI_FFS_fs, tFileDesc, cConfString + nTotalRead, 256);		
		nTotalRead += nReadNum;			
	}

	pETH_ConfJson = cJSON_Parse(cConfString);
	if (pETH_ConfJson == NULL) {
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL){
				printf("Error before: %s\n", error_ptr);
		}
		return (-1);
	}

	pStatic = cJSON_GetObjectItemCaseSensitive(pETH_ConfJson, CONF_STATIC_IP_JSON_TAG);
	if (cJSON_IsFalse(pStatic)){
		tEthConf.bStaticIP = 0;
	}	else if (cJSON_IsTrue(pStatic)) {
		tEthConf.bStaticIP = 1;
	} else {
		// TODO
	}
	
	UTL_get_byte_from_array_json(cJSON_GetObjectItemCaseSensitive(pETH_ConfJson, CONF_IP_ADDR_JSON_TAG), 
		tEthConf.uIP_Addr, sizeof(tEthConf.uIP_Addr)/sizeof(uint8_t));
	
	UTL_get_byte_from_array_json(cJSON_GetObjectItemCaseSensitive(pETH_ConfJson, CONF_NETMASK_JSON_TAG), 
		tEthConf.uNetmask, sizeof(tEthConf.uNetmask)/sizeof(uint8_t));
	
	UTL_get_byte_from_array_json(cJSON_GetObjectItemCaseSensitive(pETH_ConfJson, CONF_GW_ADDR_JSON_TAG), 
		tEthConf.uGateway, sizeof(tEthConf.uGateway)/sizeof(uint8_t));
	
	UTL_get_byte_from_array_json(cJSON_GetObjectItemCaseSensitive(pETH_ConfJson, CONF_MAC_ADDR_JSON_TAG), 
		tEthConf.uMAC_Addr, sizeof(tEthConf.uMAC_Addr)/sizeof(uint8_t));

	cJSON_Delete(pETH_ConfJson);
	return 0;
}

void UTL_sh_z_004_info_init(void) {
	spiffs_file tFileDesc;
	
	tFileDesc = SPIFFS_open(&SPI_FFS_fs, SH_Z_004_INFO_FILE_NAME, SPIFFS_RDONLY, 0);
	if (tFileDesc < 0) {
		// file not exist, save default configuration
		create_default_sh_z_002_info();
	} else {
		// file exist, not first time run
		load_sh_z_002_info(tFileDesc);
		SPIFFS_close(&SPI_FFS_fs, tFileDesc);
	}	
}

void UTL_sh_z_eth_conf_init(void) {
	spiffs_file tFileDesc;
	
	tFileDesc = SPIFFS_open(&SPI_FFS_fs, SH_Z_004_ETH_CONF_FILE_NAME, SPIFFS_RDONLY, 0);
	if (tFileDesc < 0) {
		// file not exist, save default configuration
		UTL_create_default_eth_conf();
	} else {
		// file exist, not first time run
		load_sh_z_002_eth_conf(tFileDesc);
		SPIFFS_close(&SPI_FFS_fs, tFileDesc);
	}	
}

