#include <string.h>
#include "cmsis_os.h"
#include "lwip.h"
#include "sh_z_002.h"
#include "lwip/apps/fs.h"
#include "httpd.h"
#include "spiffs.h"

extern spiffs SPI_FFS_fs;
extern EventGroupHandle_t xComEventGroup;
extern osMutexId WebServerFileMutexHandle;

extern void http_server_socket_thread(void *arg);

int fs_open_custom(struct fs_file *file, const char *name) {
	spiffs_stat FileState;
	int res;
	spiffs_file tFileDesc;
  
	osMutexWait(WebServerFileMutexHandle, osWaitForever);
	tFileDesc = SPIFFS_open(&SPI_FFS_fs, name, SPIFFS_RDONLY, 0);
	if (tFileDesc >= 0) {	
		res = SPIFFS_fstat(&SPI_FFS_fs, tFileDesc, &FileState);
		if (res < 0 ) {
			return 0;
		}
		file->index = 0;
		file->len = FileState.size;
		file->pextension = (void *)tFileDesc;
		return 1;
	} else {
		return 0;
	}	
}

void fs_close_custom(struct fs_file *file) {
	SPIFFS_close(&SPI_FFS_fs, (spiffs_file)(file->pextension));
	osMutexRelease(WebServerFileMutexHandle);
}

int fs_read_custom(struct fs_file *file, char *buffer, int count) {
	int res;
	res = SPIFFS_read(&SPI_FFS_fs, (spiffs_file)(file->pextension), (u8_t *)buffer, count);
	if (res >= 0) {
		file->index += res;
	} 
	return res;
}

//void httpd_cgi_handler(const char* uri, int iNumParams, char **pcParam, char **pcValue) {
//	
//}

err_t httpd_post_begin(void *connection, const char *uri, const char *http_request,
                       u16_t http_request_len, int content_len, char *response_uri,
                       u16_t response_uri_len, u8_t *post_auto_wnd) {
	return ERR_OK;						   
}
					   
err_t httpd_post_receive_data(void *connection, struct pbuf *p) {
	
	
	return ERR_OK;
}

void httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len) {
	

}

void start_webserver(void const * argument) {
	xEventGroupWaitBits(xComEventGroup, EG_ETH_NETIF_UP_BIT, pdFALSE, pdFALSE, osWaitForever );
//	httpd_init();
	http_server_socket_thread(NULL);
}

//void start_tftp(void const * argument) {
//	
//	while(1) {
//		osDelay(5000);
//	}
//}
