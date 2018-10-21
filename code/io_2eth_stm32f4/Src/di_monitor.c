#include "main.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include "cmsis_os.h"
#include "di_monitor.h"
#include "spiffs.h"
#include "cJSON.h"

#define DI_SAMPLE_INTERVAL				10
#define DI_SAMPLE_FILTER_TIME			(DI_SAMPLE_INTERVAL*5)
#define DI_CONF_FILE_NAME				"di_conf"
#define DI_CONF_LATCH_SET_JSON_TAG		"unLatchSet"
#define DI_CONF_CNT_ENABLE_JSON_TAG		"unCNT_Enable"

extern osMutexId DI_DataAccessHandle;
extern EventGroupHandle_t xDiEventGroup;
extern spiffs SPI_FFS_fs;

//typedef struct {
//	uint32_t unLatchSet;
//	uint32_t unCNT_Enable;
//}DI_ConfTypeDef;

static uint32_t unDI_Value; 
static uint32_t DI_EnableCNT;
static uint32_t DI_CNT_Overflow;
static uint32_t DI_LatchSet;
static uint32_t DI_LatchStatus;
//static DI_ConfTypeDef tDI_Conf[SH_Z_002_DI_NUM];
static uint32_t unDI_CNT_FreqValue[SH_Z_002_DI_NUM];
// limitation can only use pins in same port, e.g. GPIOD 
// Also the pins should be continuously
static GPIO_TypeDef* DI_Port = GPIOD;
static const uint16_t DI_Pins[SH_Z_002_DI_NUM] = {DI_0_Pin, DI_1_Pin, DI_2_Pin, DI_3_Pin};
static int di_save_conf(void);

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	BaseType_t xHigherPriorityTaskWoken, xResult;
	if ((DI_0_Pin == GPIO_Pin) || (DI_1_Pin == GPIO_Pin) || 
		(DI_2_Pin == GPIO_Pin) || (DI_3_Pin == GPIO_Pin)) {
		/* xHigherPriorityTaskWoken must be initialised to pdFALSE. */
		xHigherPriorityTaskWoken = pdFALSE;

		/* Set bit 0 and bit 4 in xEventGroup. */
		xResult = xEventGroupSetBitsFromISR(xDiEventGroup, GPIO_Pin, &xHigherPriorityTaskWoken );

		/* Was the message posted successfully? */
		if( xResult != pdFAIL ) {
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
		}	  
	}
}

static uint32_t get_DI_values(void) {
	uint8_t unIndex;
	uint32_t unDI_ValueTemp = 0;

	assert_param(IS_DI_PIN_NUM(SH_Z_002_DI_NUM));
	
	for (unIndex = 0; unIndex < SH_Z_002_DI_NUM; unIndex++) {
		(GPIO_PIN_SET == HAL_GPIO_ReadPin(DI_Port, DI_Pins[unIndex])) ? 
			(SET_BIT(unDI_ValueTemp, DI_Pins[unIndex])) : 
			(CLEAR_BIT(unDI_ValueTemp, DI_Pins[unIndex]));
	}
	return unDI_ValueTemp;
}

uint32_t DI_get_DI_values(void) {
	uint32_t unTemp;
	osMutexWait(DI_DataAccessHandle, osWaitForever);
	unTemp = unDI_Value >> SH_Z_002_DI_PIN_OFFSET;
	osMutexRelease(DI_DataAccessHandle);
	return unTemp;
}

void DI_get_DI_cnt_freq(uint32_t* pTarget, uint8_t unDI_Num) {
	osMutexWait(DI_DataAccessHandle, osWaitForever);
	memcpy(pTarget, unDI_CNT_FreqValue, sizeof(uint32_t) * unDI_Num);
	osMutexRelease(DI_DataAccessHandle);
}

uint32_t DI_get_DI_enable_CNT(void) {
	uint32_t unTemp;
	osMutexWait(DI_DataAccessHandle, osWaitForever);
	unTemp = DI_EnableCNT >> SH_Z_002_DI_PIN_OFFSET;
	osMutexRelease(DI_DataAccessHandle);
	return unTemp;
}

uint32_t DI_get_DI_CNT_overflow(void) {
	uint32_t unTemp;
	osMutexWait(DI_DataAccessHandle, osWaitForever);
	unTemp = DI_CNT_Overflow >> SH_Z_002_DI_PIN_OFFSET;
	osMutexRelease(DI_DataAccessHandle);
	return unTemp;
}

uint32_t DI_get_DI_latch_set(void) {
	uint32_t unTemp;
	osMutexWait(DI_DataAccessHandle, osWaitForever);
	unTemp = DI_LatchSet >> SH_Z_002_DI_PIN_OFFSET;
	osMutexRelease(DI_DataAccessHandle);
	return unTemp;
}

uint32_t DI_get_DI_latch_status(void) {
	uint32_t unTemp;
	osMutexWait(DI_DataAccessHandle, osWaitForever);
	unTemp = DI_LatchStatus >> SH_Z_002_DI_PIN_OFFSET;
	osMutexRelease(DI_DataAccessHandle);
	return unTemp;
}

void DI_set_DI_latch_status(uint32_t unValue) {
	osMutexWait(DI_DataAccessHandle, osWaitForever);
	DI_LatchStatus = unValue << SH_Z_002_DI_PIN_OFFSET;
	osMutexRelease(DI_DataAccessHandle);	
}

void DI_set_DI_enable_CNT(uint32_t unValue) {
	osMutexWait(DI_DataAccessHandle, osWaitForever);
	DI_EnableCNT = unValue << SH_Z_002_DI_PIN_OFFSET;
	osMutexRelease(DI_DataAccessHandle);
	di_save_conf();
}

void DI_clear_DI_CNT(uint8_t unDI_Index) {
	osMutexWait(DI_DataAccessHandle, osWaitForever);
	unDI_CNT_FreqValue[unDI_Index] = 0;
	osMutexRelease(DI_DataAccessHandle);
}

void DI_clear_DI_latch(uint8_t unDI_Index) {
	osMutexWait(DI_DataAccessHandle, osWaitForever);
	CLEAR_BIT(DI_LatchStatus, DI_Pins[unDI_Index]);
	osMutexRelease(DI_DataAccessHandle);
}

void DI_clear_DI_CNT_oveflow(uint8_t unDI_Index) {
	osMutexWait(DI_DataAccessHandle, osWaitForever);
	CLEAR_BIT(DI_CNT_Overflow, DI_Pins[unDI_Index]);
	osMutexRelease(DI_DataAccessHandle);
}

void DI_set_DI_latch_set(uint32_t unValue) {
	osMutexWait(DI_DataAccessHandle, osWaitForever);
	DI_LatchSet = unValue << SH_Z_002_DI_PIN_OFFSET;
	osMutexRelease(DI_DataAccessHandle);
	di_save_conf();
}

static int di_conf_wr_conf_file(spiffs_file tFileDesc) {
    char* pJsonString = NULL;
    cJSON* pLatchSet = NULL;
	cJSON* pCNT_Enable = NULL;
	
	cJSON* pDI_ConfJsonWriter = cJSON_CreateObject();
	if (pDI_ConfJsonWriter == NULL){
		printf("failed to create json root object.\n");
		return (-1);
	}
	osMutexWait(DI_DataAccessHandle, osWaitForever);
    pLatchSet = cJSON_CreateNumber(DI_LatchSet);
	osMutexRelease(DI_DataAccessHandle);
    if (pLatchSet == NULL){
		cJSON_Delete(pDI_ConfJsonWriter);
		printf("failed to create json latch set object.\n");
		return (-1);
    }	
	cJSON_AddItemToObject(pDI_ConfJsonWriter, DI_CONF_LATCH_SET_JSON_TAG, pLatchSet);
	osMutexWait(DI_DataAccessHandle, osWaitForever);
    pCNT_Enable = cJSON_CreateNumber(DI_EnableCNT);
	osMutexRelease(DI_DataAccessHandle);
    if (pCNT_Enable == NULL){
		cJSON_Delete(pDI_ConfJsonWriter);
		printf("failed to create json CNT enable object.\n");
		return (-1);
    }	
	cJSON_AddItemToObject(pDI_ConfJsonWriter, DI_CONF_CNT_ENABLE_JSON_TAG, pCNT_Enable);	
	
	pJsonString = cJSON_Print(pDI_ConfJsonWriter);
    if (pJsonString == NULL){
		cJSON_Delete(pDI_ConfJsonWriter);
		printf("failed to digest json object.\n");
		return (-1);
    }
	
	if (SPIFFS_write(&SPI_FFS_fs, tFileDesc, pJsonString, strlen(pJsonString)) < 0 ) {
		cJSON_Delete(pDI_ConfJsonWriter);
		free(pJsonString);
		printf("failed to write digested json string to file.\n");
		return (-1);		
	}
	
	cJSON_Delete(pDI_ConfJsonWriter);
	free(pJsonString);
	return (0);
}

static int di_conf_load(spiffs_file tFileDesc) {
	cJSON* pLatchSet = NULL;
	cJSON* pCNT_Enable = NULL;
	cJSON* pDI_ConfJson;
	char cConfString[512];
	int nReadNum = SPIFFS_read(&SPI_FFS_fs, tFileDesc, cConfString, sizeof(cConfString));
	if ((nReadNum <= 0) || (sizeof(cConfString) == nReadNum )) {
		printf("%d char was read from conf file.\n", nReadNum);
		return (-1);		
	}

	pDI_ConfJson = cJSON_Parse(cConfString);
	if (pDI_ConfJson == NULL) {
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL){
			printf("Error before: %s\n", error_ptr);
		}
		return (-1);
	}

	pLatchSet = cJSON_GetObjectItemCaseSensitive(pDI_ConfJson, DI_CONF_LATCH_SET_JSON_TAG);
	if (cJSON_IsNumber(pLatchSet)){
		osMutexWait(DI_DataAccessHandle, osWaitForever);
		DI_LatchSet = (uint32_t)(pLatchSet->valuedouble);
		osMutexRelease(DI_DataAccessHandle);
	}	

	pCNT_Enable = cJSON_GetObjectItemCaseSensitive(pDI_ConfJson, DI_CONF_CNT_ENABLE_JSON_TAG);
	if (cJSON_IsNumber(pCNT_Enable)){
		osMutexWait(DI_DataAccessHandle, osWaitForever);
		DI_EnableCNT = (uint32_t)(pCNT_Enable->valuedouble);
		osMutexRelease(DI_DataAccessHandle);
	}
	cJSON_Delete(pDI_ConfJson);
	return 0;
}

static int di_save_conf(void) {
	spiffs_file tFileDesc;
	tFileDesc = SPIFFS_open(&SPI_FFS_fs, DI_CONF_FILE_NAME, SPIFFS_RDWR | SPIFFS_CREAT, 0);
	if (tFileDesc < 0) {
		printf("failed to open di configuration file.\n");
		return (-1);
	} else {
		if (di_conf_wr_conf_file(tFileDesc) < 0) {
			printf("failed to write default di configuration value to file.\n");
			SPIFFS_close(&SPI_FFS_fs, tFileDesc);
			return (-1);
		} else {
			SPIFFS_close(&SPI_FFS_fs, tFileDesc);
			return (0);				
		}
	}
}

static int di_conf_initialize(void) {
	spiffs_file tFileDesc;
	tFileDesc = SPIFFS_open(&SPI_FFS_fs, DI_CONF_FILE_NAME, SPIFFS_RDONLY, 0);
	if (tFileDesc < 0) {
		// file not exist, save default configuration
		return di_save_conf();
	} else {
		// file exist, not first time run
		if (di_conf_load(tFileDesc) < 0) {
			printf("failed to load di configuration from file.\n");
			SPIFFS_close(&SPI_FFS_fs, tFileDesc);
			return di_save_conf();			
		} else {
			SPIFFS_close(&SPI_FFS_fs, tFileDesc);
			return (0);	
		}
	}	
}

void start_di_monitor(void const * argument) {
	uint8_t unIndex;
//	uint8_t* pTestBuf;
	static BaseType_t bSomeThingHappened;
	static uint32_t unDI_ValueTemp;
	static uint16_t unDI_FilterTimer[SH_Z_002_DI_NUM];
	
	// wait until fs ready, to read configuration file
//	xEventGroupWaitBits(xDiEventGroup, SPIFFS_READY_EVENT_BIT, pdTRUE, pdFALSE, osWaitForever );
	while(SPI_FFS_fs.mounted != 1) {
		osDelay(100);
	}
	di_conf_initialize();
	while (1) {
		xEventGroupWaitBits(xDiEventGroup, DI_0_Pin | DI_1_Pin | DI_2_Pin | DI_3_Pin, 
										pdTRUE, pdFALSE, osWaitForever );
		bSomeThingHappened = pdTRUE;
		while (pdTRUE == bSomeThingHappened) {
			unDI_ValueTemp = get_DI_values();
			bSomeThingHappened = pdFALSE;
			osMutexWait(DI_DataAccessHandle, osWaitForever);
			for (unIndex = 0; unIndex < SH_Z_002_DI_NUM; unIndex++) {
				if (READ_BIT(unDI_ValueTemp, DI_Pins[unIndex]) != READ_BIT(unDI_Value, DI_Pins[unIndex])) {
					unDI_FilterTimer[unIndex] += DI_SAMPLE_INTERVAL;
					bSomeThingHappened = pdTRUE;
				} else {
					unDI_FilterTimer[unIndex] = 0;
				}
			}
			for (unIndex = 0; unIndex < SH_Z_002_DI_NUM; unIndex++) {
				if (unDI_FilterTimer[unIndex] > DI_SAMPLE_FILTER_TIME) {
					if (READ_BIT(unDI_ValueTemp, DI_Pins[unIndex]) == DI_Pins[unIndex]) {
						(SET_BIT(unDI_Value, DI_Pins[unIndex]));
						if (READ_BIT(DI_LatchSet, DI_Pins[unIndex])) {
							SET_BIT(DI_LatchStatus, DI_Pins[unIndex]);
						}
					} else {
						(CLEAR_BIT(unDI_Value, DI_Pins[unIndex]));
						if (READ_BIT(DI_EnableCNT, DI_Pins[unIndex])) {
							unDI_CNT_FreqValue[unIndex]++;
							if (0 == unDI_CNT_FreqValue[unIndex]) {
								SET_BIT(DI_CNT_Overflow, DI_Pins[unIndex]);
							}
						}
						if (0 == READ_BIT(DI_LatchSet, DI_Pins[unIndex])) {
							SET_BIT(DI_LatchStatus, DI_Pins[unIndex]);
						}
					}
				}
			}		
			osMutexRelease(DI_DataAccessHandle);		
			osDelay(DI_SAMPLE_INTERVAL);
		}
	}
}

