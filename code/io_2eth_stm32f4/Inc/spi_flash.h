#ifndef __SPI_FLASH_H
#define __SPI_FLASH_H

//#include "cmsis_os.h"
//void spi_flash_erase_block(uint32_t addr);
//void spi_flash_erase_chip(void);
//void spi_flash_read(uint32_t addr, uint32_t size, uint8_t *dst);
//void spi_flash_write(uint32_t addr, uint32_t size, uint8_t *dst);
void spiffs_init(void);
//void spi_flash_read_manufacturer_ID(uint8_t* pRD_Data, uint32_t unDataLen);
#endif

