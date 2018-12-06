#include "main.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "lwip.h"
#include "sockets.h"
#include "cJSON.h"
#include "spiffs.h"
#include "lwip/apps/tftp_server.h"
#include "sh_z_004.h"
#include "mqtt.h"

extern osMutexId WebServerFileMutexHandle;
extern spiffs SPI_FFS_fs;
extern EventGroupHandle_t xComEventGroup;

char SH_Z_004_SN[SH_Z_SN_LEN + 1] = "SHZ004.201811170";
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
	
	pJsonString = cJSON_PrintUnformatted(pDI_ConfJsonWriter);
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
	
	pJsonString = cJSON_PrintUnformatted(pEthConfJsonWriter);
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

void example_do_connect(mqtt_client_t *client);

void UTL_start_udp_broadcast(void const * argument){
	static char cReportSlaveID[sizeof("sh-z-004") - 1 + 4 + sizeof(SH_Z_004_SN)];
	int udpSocket;
	struct sockaddr_in sDestAddr;
	int broadcast=1;
 	memset(cReportSlaveID, 0, sizeof(cReportSlaveID));
	sprintf(cReportSlaveID, "%s%04X%s", "sh-z-004", SH_Z_004_VERSION, SH_Z_004_SN);
		
	xEventGroupWaitBits(xComEventGroup, EG_ETH_NETIF_UP_BIT, pdFALSE, pdFALSE, osWaitForever );
	osDelay(1000);
//	mqtt_client_t *client = mqtt_client_new();
//  if(client != NULL) {
//    example_do_connect(client);
//  }	
	
	udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(udpSocket < 0) {
		printf("socket() failed!!\n");
		return;
	}
	
	int rslt = setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
	if(rslt < 0) {
		printf("Error in setting broadcast option, %d.\n", rslt);
		close(udpSocket);
		return;
	}
		
	/*Destination*/
	memset((char *)&sDestAddr, 0, sizeof(sDestAddr));
	sDestAddr.sin_family = AF_INET;
	sDestAddr.sin_len = sizeof(sDestAddr);
	sDestAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	sDestAddr.sin_port = htons(52018);
 	while (1) {
		sendto(udpSocket, cReportSlaveID, sizeof(cReportSlaveID), 0, (struct sockaddr *)&sDestAddr, sizeof(sDestAddr));
		osDelay( 5000 );  //some delay!
	}
	close(udpSocket);
}
 
//void my_mqtt_connect(void) {
//	struct MqttSampleContext ctx[1];
//	int keep_alive = 1200;
//	int bytes;
//	MqttSample_Connect(ctx, PROD_ID, SN, ctx->devid, keep_alive, clean_session);
//	bytes = Mqtt_SendPkt(ctx->mqttctx, ctx->mqttbuf, 0);
//	MqttBuffer_Reset(ctx->mqttbuf);	
//}
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len);
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
static void mqtt_sub_request_cb(void *arg, err_t result);
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);

void example_do_connect(mqtt_client_t *client)
{
  struct mqtt_connect_client_info_t ci;
  err_t err;
  ip_addr_t ip_addr;
  /* Setup an empty client info structure */
  memset(&ci, 0, sizeof(ci));
  
  /* Minimal amount of information required is client identifier, so set it here */ 
  ci.client_id = "504952551";
	ci.client_user = "125058";
	ci.client_pass = "201811232103";
  
  /* Initiate client and connect to server, if this fails immediately an error code is returned
     otherwise mqtt_connection_cb will be called with connection result after attempting 
     to establish a connection with the server. 
     For now MQTT version 3.1.1 is always used */
  ipaddr_aton("183.230.40.39", &ip_addr);
  err = mqtt_client_connect(client, &ip_addr, 6002, mqtt_connection_cb, 0, &ci);
  
  /* For now just print the result code if something goes wrong */
  if(err != ERR_OK) {
    printf("mqtt_connect return %d\n", err);
  }
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
  err_t err;
  if(status == MQTT_CONNECT_ACCEPTED) {
    printf("mqtt_connection_cb: Successfully connected\n");
    
    /* Setup callback for incoming publish requests */
    mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, arg);
    
    /* Subscribe to a topic named "subtopic" with QoS level 1, call mqtt_sub_request_cb with result */ 
    err = mqtt_subscribe(client, "subtopic", 1, mqtt_sub_request_cb, arg);

    if(err != ERR_OK) {
      printf("mqtt_subscribe return: %d\n", err);
    }
  } else {
    printf("mqtt_connection_cb: Disconnected, reason: %d\n", status);
    
    /* Its more nice to be connected, so try to reconnect */
    example_do_connect(client);
  }  
}

static void mqtt_sub_request_cb(void *arg, err_t result)
{
  /* Just print the result code here for simplicity, 
     normal behaviour would be to take some action if subscribe fails like 
     notifying user, retry subscribe or disconnect from server */
  printf("Subscribe result: %d\n", result);
}
static int inpub_id;
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
  printf("Incoming publish at topic %s with total length %u\n", topic, (unsigned int)tot_len);

  /* Decode topic string into a user defined reference */
  if(strcmp(topic, "print_payload") == 0) {
    inpub_id = 0;
  } else if(topic[0] == 'A') {
    /* All topics starting with 'A' might be handled at the same way */
    inpub_id = 1;
  } else {
    /* For all other topics */
    inpub_id = 2;
  }
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
  printf("Incoming publish payload with length %d, flags %u\n", len, (unsigned int)flags);

  if(flags & MQTT_DATA_FLAG_LAST) {
    /* Last fragment of payload received (or whole part if payload fits receive buffer
       See MQTT_VAR_HEADER_BUFFER_LEN)  */

    /* Call function or do action depending on reference, in this case inpub_id */
    if(inpub_id == 0) {
      /* Don't trust the publisher, check zero termination */
      if(data[len-1] == 0) {
        printf("mqtt_incoming_data_cb: %s\n", (const char *)data);
      }
    } else if(inpub_id == 1) {
      /* Call an 'A' function... */
    } else {
      printf("mqtt_incoming_data_cb: Ignoring payload...\n");
    }
  } else {
    /* Handle fragmented payload, store in buffer, write to file or whatever */
  }
}

