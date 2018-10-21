#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "spiffs.h"
#include "sh_z_004.h"
#include "spi_flash.h"
//#include "fs_handling.h"

extern spiffs SPI_FFS_fs;

void FS_fw_flash_program(char* pFW_FileName, unsigned int unFW_FileSize, unsigned int unFW_FlashWriteAddr) {
	// disable all interrupt
	
	// disable all other OS function to prevent task switch
	
	// enter critical place, do it and only do it
	
	// enable internal flash write/erase
	
	// erase internal flash
	
	// 
	// reboot
}

void FS_remove_all_files(void) {
	// remove all files 
	spiffs_DIR d;
	struct spiffs_dirent e;
	struct spiffs_dirent *pe = &e;
	int res;

	SPIFFS_opendir(&SPI_FFS_fs, "/", &d);
	while ((pe = SPIFFS_readdir(&d, pe)) != NULL) {
		res = SPIFFS_remove(&SPI_FFS_fs, (char *)(pe->name));
		if (res < 0) {
			printf("remove file %s failed.\n", pe->name);
		}
	}
	SPIFFS_closedir(&d);
}

void start_erase_all_files_task(void* argument) {
	FS_remove_all_files();
	vTaskDelete(NULL);
}

void start_format_fs_task(void* argument) {
	SPIFFS_unmount(&SPI_FFS_fs);
	SPIFFS_format(&SPI_FFS_fs);
	spiffs_init();
	vTaskDelete(NULL);
}

