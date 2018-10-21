#ifndef _DI_MONITOR_H
#define _DI_MONITOR_H

#include "cmsis_os.h"
#include "sh_z_004.h"

#define GET_DI_BYTE_NUM(x)			((((x) % BITS_NUM_PER_BYTE) == 0) ? \
																	((x) / BITS_NUM_PER_BYTE): \
																	((x) / BITS_NUM_PER_BYTE + 1))
#define SH_Z_004_DI_BYTE_NUM		GET_DI_BYTE_NUM(SH_Z_004_DI_NUM)
#define IS_DI_PIN_NUM(NUM)			(((NUM) <= 32 ))
//#define SPIFFS_READY_EVENT_BIT		0x00010000

//typedef struct {
//	uint8_t bEnableCNT : 1;
//	uint8_t bClearCNT : 1;
//	uint8_t bClearOverflow : 1;
//	uint8_t bLatchStatus : 1;
//	uint8_t bLatchSet : 1;	// 1 means latch on rising edge, 0 on falling
//	uint8_t NA : 3;
//}DI_ConfTypeDef;

uint32_t DI_get_DI_values(void);
void DI_get_DI_cnt_freq(uint32_t* pTarget, uint8_t unDI_Num);
uint32_t DI_get_DI_enable_CNT(void);
uint32_t DI_get_DI_CNT_overflow(void);
uint32_t DI_get_DI_latch_set(void);
uint32_t DI_get_DI_latch_status(void);
void DI_set_DI_enable_CNT(uint32_t unValue);
void DI_clear_DI_CNT(uint8_t unDI_Index);
void DI_clear_DI_latch(uint8_t unDI_Index);
void DI_clear_DI_CNT_oveflow(uint8_t unDI_Index);
void DI_set_DI_latch_set(uint32_t unValue);
void DI_set_DI_latch_status(uint32_t unValue);

void start_di_monitor(void const * argument);
#endif
