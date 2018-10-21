#include "main.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "sh_z_004.h"
#include "spiffs.h"

#define SPI_FLASH_CHIP_BUSY_BIT				(0x01U)	

extern SPI_HandleTypeDef hspi3;
extern osMutexId SpiFlashChipMutexHandle;
extern EventGroupHandle_t xComEventGroup;

spiffs SPI_FFS_fs;
uint8_t FS_Work_Buf[256 * 2];
uint8_t FS_FDS[32 * 4];
uint8_t FS_Cache_Buf[(256 + 32) * 4];

typedef enum{
	WR_FLASH_PAGE_PROGRAM = 0x02,
	RD_FLASH_READ_DATA = 0x03,	
	WR_FLASH_WR_ENABLE = 0x06,
	RD_FLASH_REG_STATUS_01 = 0x05,
	RD_FLASH_REG_STATUS_02 = 0x35,
	RD_FLASH_REG_STATUS_03 = 0x15,
//	RD_FLASH_REG_MANUF_DEV_ID = 0x9F,
	WR_FLASH_CHIP_ERASE = 0xC7,
	WR_FLASH_BLOCK_ERASE = 0xD8
}SPI_FlashChipCMD_TypeDef;

static inline void __set_spi_flash_com_event_bit(void) {
	BaseType_t xHigherPriorityTaskWoken, xResult;
	/* xHigherPriorityTaskWoken must be initialised to pdFALSE. */
	xHigherPriorityTaskWoken = pdFALSE;

	xResult = xEventGroupSetBitsFromISR(xComEventGroup, EG_EXT_FLASH_SPI_DMA_DONE_BIT, &xHigherPriorityTaskWoken );

	/* Was the message posted successfully? */
	if( xResult != pdFAIL ) {
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
	if (hspi == &hspi3) {
		__set_spi_flash_com_event_bit();	  
	}
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi){
	if (hspi == &hspi3) {
		__set_spi_flash_com_event_bit();  
	}
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi){
	if (hspi == &hspi3) {
		__set_spi_flash_com_event_bit(); 
	}
}

static void write_SPI_flash_chip(uint8_t unCMD, uint8_t* pWR_Data, uint32_t unDataLen) {
//	EventBits_t uxBits;
	HAL_GPIO_WritePin(SPI_FLASH_CS_GPIO_Port, SPI_FLASH_CS_Pin, GPIO_PIN_RESET);
	
	HAL_SPI_Transmit_DMA(&hspi3, &unCMD, 1);
	xEventGroupWaitBits(xComEventGroup, EG_EXT_FLASH_SPI_DMA_DONE_BIT, pdTRUE, pdFALSE, osWaitForever );
	
	if ((pWR_Data != NULL) && (unDataLen > 0) ){
		HAL_SPI_Transmit_DMA(&hspi3, pWR_Data, unDataLen);
		xEventGroupWaitBits(xComEventGroup, EG_EXT_FLASH_SPI_DMA_DONE_BIT, pdTRUE, pdFALSE, osWaitForever );
	}
	
	HAL_GPIO_WritePin(SPI_FLASH_CS_GPIO_Port, SPI_FLASH_CS_Pin, GPIO_PIN_SET);
}

static void write_SPI_flash_data(uint32_t unAddr, uint8_t* pWR_Data, uint32_t unDataLen) {
//	EventBits_t uxBits;
	uint8_t unBuf[3];
	
	HAL_GPIO_WritePin(SPI_FLASH_CS_GPIO_Port, SPI_FLASH_CS_Pin, GPIO_PIN_RESET);
	
	unBuf[0] = (uint8_t)WR_FLASH_PAGE_PROGRAM;
	HAL_SPI_Transmit_DMA(&hspi3, unBuf, 1);
	xEventGroupWaitBits(xComEventGroup, EG_EXT_FLASH_SPI_DMA_DONE_BIT, pdTRUE, pdFALSE, osWaitForever );
	
	unBuf[0] = (unAddr >> 16) & 0xFF;
	unBuf[1] = (unAddr >> 8) & 0xFF;
	unBuf[2] = unAddr & 0xFF;
	HAL_SPI_Transmit_DMA(&hspi3, unBuf, 3);
	xEventGroupWaitBits(xComEventGroup, EG_EXT_FLASH_SPI_DMA_DONE_BIT, pdTRUE, pdFALSE, osWaitForever );
	
	if ((pWR_Data != NULL) && (unDataLen > 0) ){
		HAL_SPI_Transmit_DMA(&hspi3, pWR_Data, unDataLen);
		xEventGroupWaitBits(xComEventGroup, EG_EXT_FLASH_SPI_DMA_DONE_BIT, pdTRUE, pdFALSE, osWaitForever );
	}
	
	HAL_GPIO_WritePin(SPI_FLASH_CS_GPIO_Port, SPI_FLASH_CS_Pin, GPIO_PIN_SET);
}

static void read_SPI_flash_chip(uint8_t unCMD, uint8_t* pRD_Data, uint32_t unDataLen) {
//	EventBits_t uxBits;
	HAL_GPIO_WritePin(SPI_FLASH_CS_GPIO_Port, SPI_FLASH_CS_Pin, GPIO_PIN_RESET);
	
	HAL_SPI_Transmit_DMA(&hspi3, &unCMD, 1);
	xEventGroupWaitBits(xComEventGroup, EG_EXT_FLASH_SPI_DMA_DONE_BIT, pdTRUE, pdFALSE, osWaitForever );
	
	HAL_SPI_Receive_DMA(&hspi3, pRD_Data, unDataLen);
	xEventGroupWaitBits(xComEventGroup, EG_EXT_FLASH_SPI_DMA_DONE_BIT, pdTRUE, pdFALSE, osWaitForever );
	
	HAL_GPIO_WritePin(SPI_FLASH_CS_GPIO_Port, SPI_FLASH_CS_Pin, GPIO_PIN_SET);
}

static void read_SPI_flash_data(uint32_t unAddr, uint8_t* pRD_Data, uint32_t unDataLen) {
//	EventBits_t uxBits;
	uint8_t unBuf[3];
	
	HAL_GPIO_WritePin(SPI_FLASH_CS_GPIO_Port, SPI_FLASH_CS_Pin, GPIO_PIN_RESET);
	
	unBuf[0] = (uint8_t)RD_FLASH_READ_DATA;
	HAL_SPI_Transmit_DMA(&hspi3, unBuf, 1);
	xEventGroupWaitBits(xComEventGroup, EG_EXT_FLASH_SPI_DMA_DONE_BIT, pdTRUE, pdFALSE, osWaitForever );

	unBuf[0] = (unAddr >> 16) & 0xFF;
	unBuf[1] = (unAddr >> 8) & 0xFF;
	unBuf[2] = unAddr & 0xFF;
	HAL_SPI_Transmit_DMA(&hspi3, unBuf, 3);
	xEventGroupWaitBits(xComEventGroup, EG_EXT_FLASH_SPI_DMA_DONE_BIT, pdTRUE, pdFALSE, osWaitForever );
	
	HAL_SPI_Receive_DMA(&hspi3, pRD_Data, unDataLen);
	xEventGroupWaitBits(xComEventGroup, EG_EXT_FLASH_SPI_DMA_DONE_BIT, pdTRUE, pdFALSE, osWaitForever );
	
	HAL_GPIO_WritePin(SPI_FLASH_CS_GPIO_Port, SPI_FLASH_CS_Pin, GPIO_PIN_SET);
}

static uint8_t get_SPI_flash_chip_status(SPI_FlashChipCMD_TypeDef tStatusReg) {
	static uint8_t unRegValue;
	read_SPI_flash_chip(tStatusReg, &unRegValue, sizeof(unRegValue));
	return unRegValue;
}

#define IS_SPI_FLASH_CHIP_BUSY()			(((get_SPI_flash_chip_status(RD_FLASH_REG_STATUS_01) & SPI_FLASH_CHIP_BUSY_BIT) == 0) ? (pdFALSE):(pdTRUE))

void spi_flash_erase_block(uint32_t addr) {
	static uint8_t unBuf[3];
	// get mutex to use spi resource
	osMutexWait(SpiFlashChipMutexHandle, osWaitForever);
	
	unBuf[0] = (addr >> 16) & 0xFF;
	unBuf[1] = (addr >> 8) & 0xFF;
	unBuf[2] = addr & 0xFF;
	
	// wait until spi flash chip is not BUSY
	while (pdTRUE == IS_SPI_FLASH_CHIP_BUSY()) {
		osDelay(20);
	}
	// write command to spi flash chip
	write_SPI_flash_chip(WR_FLASH_WR_ENABLE, NULL, 0);
	
	write_SPI_flash_chip(WR_FLASH_BLOCK_ERASE, unBuf, sizeof(unBuf));
	
	// wait until spi flash chip is not BUSY
	while (pdTRUE == IS_SPI_FLASH_CHIP_BUSY()) {
		osDelay(40);
	}	
	// release spi resource
	osMutexRelease(SpiFlashChipMutexHandle);
}

void spi_flash_erase_chip(void) {
	// get mutex to use spi resource
	osMutexWait(SpiFlashChipMutexHandle, osWaitForever);
	// wait until spi flash chip is not BUSY
	while (pdTRUE == IS_SPI_FLASH_CHIP_BUSY()) {
		osDelay(20);
	}
	// write command to spi flash chip
	write_SPI_flash_chip(WR_FLASH_WR_ENABLE, NULL, 0);
	
	write_SPI_flash_chip(WR_FLASH_CHIP_ERASE, NULL, 0);
	
	// wait until spi flash chip is not BUSY
	while (pdTRUE == IS_SPI_FLASH_CHIP_BUSY()) {
		osDelay(50);
	}	
	// release spi resource
	osMutexRelease(SpiFlashChipMutexHandle);
}

void spi_flash_read(uint32_t addr, uint32_t size, uint8_t *dst) {
	// get mutex to use spi resource
	osMutexWait(SpiFlashChipMutexHandle, osWaitForever);
	// wait until spi flash chip is not BUSY
	while (pdTRUE == IS_SPI_FLASH_CHIP_BUSY()) {
		osDelay(20);
	}
	// write command to spi flash chip
	read_SPI_flash_data(addr, dst, size);
	
	// release spi resource
	osMutexRelease(SpiFlashChipMutexHandle);
}

void spi_flash_write(uint32_t addr, uint32_t size, uint8_t *dst) {
	// get mutex to use spi resource
	osMutexWait(SpiFlashChipMutexHandle, osWaitForever);
	
	// wait until spi flash chip is not BUSY
	while (pdTRUE == IS_SPI_FLASH_CHIP_BUSY()) {
		osDelay(20);
	}
	// write command to spi flash chip
	write_SPI_flash_chip(WR_FLASH_WR_ENABLE, NULL, 0);
	
	write_SPI_flash_data(addr, dst, size);

	// page write time is typic 0.4ms and max 3ms in datasheet
	// so no wait here

	// release spi resource
	osMutexRelease(SpiFlashChipMutexHandle);	
}

static int32_t _spiffs_erase(uint32_t addr, uint32_t len)
{
    uint32_t i = 0;
    uint32_t erase_count = (len + SPIFFS_CFG_PHYS_ERASE_SZ(ignore) - 1) / SPIFFS_CFG_PHYS_ERASE_SZ(ignore);
    for (i = 0; i < erase_count; i++) {
        spi_flash_erase_block(addr + i * SPIFFS_CFG_PHYS_ERASE_SZ(ignore));
    }
    return 0;
}

static int32_t _spiffs_read(uint32_t addr, uint32_t size, uint8_t *dst)
{
    spi_flash_read(addr, size, dst);
    return 0;
}

static int32_t _spiffs_write(uint32_t addr, uint32_t size, uint8_t *dst)
{
    spi_flash_write(addr, size, dst);
    return 0;
}

void spiffs_init(void) {
	spiffs_config spiffs_cfg;
	int32_t res;
//	static uint8_t unManuID[3];
//	spi_flash_read_manufacturer_ID(unManuID, sizeof(unManuID));
//	spi_flash_erase_chip();
  	spiffs_cfg.hal_erase_f = _spiffs_erase;
	spiffs_cfg.hal_read_f = _spiffs_read;
	spiffs_cfg.hal_write_f = _spiffs_write;
	if (((res = SPIFFS_mount(&SPI_FFS_fs, &spiffs_cfg, FS_Work_Buf, FS_FDS, sizeof(FS_FDS), FS_Cache_Buf, sizeof(FS_Cache_Buf), NULL)) != SPIFFS_OK) && 
		(SPIFFS_errno(&SPI_FFS_fs) == SPIFFS_ERR_NOT_A_FS)) {
        printf("formatting spiffs...\n");
        if (SPIFFS_format(&SPI_FFS_fs) != SPIFFS_OK) {
            printf("SPIFFS format failed: %d\n", SPIFFS_errno(&SPI_FFS_fs));
        }
        printf("ok\n");
        printf("mounting\n");
        res = SPIFFS_mount(&SPI_FFS_fs, &spiffs_cfg, FS_Work_Buf, FS_FDS, sizeof(FS_FDS), FS_Cache_Buf, sizeof(FS_Cache_Buf), NULL);
    }
    if (res != SPIFFS_OK){
        printf("SPIFFS mount failed: %d\n", SPIFFS_errno(&SPI_FFS_fs));
    } else {
//		xEventGroupSetBits(xDiEventGroup, SPIFFS_READY_EVENT_BIT);
        printf("SPIFFS mounted\n");
    }	
}
//void spi_flash_read_manufacturer_ID(uint8_t* pRD_Data, uint32_t unDataLen) {
//	read_SPI_flash_chip(RD_FLASH_REG_MANUF_DEV_ID, pRD_Data, unDataLen);
//}

