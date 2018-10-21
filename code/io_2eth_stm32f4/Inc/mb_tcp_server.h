#ifndef _MB_TCP_SERVER_H
#define _MB_TCP_SERVER_H

#include "cmsis_os.h"
#include "lwip.h"
#include "mb.h"

#define SH_Z_002_SLAVE_DSCRPT		""

typedef enum {
	MB_TCP_SVR_FUNC_RD_COLIS_BIT = 0x01,
    MB_TCP_SVR_FUNC_RD_INPUT_BIT = 0x02,           
    MB_TCP_SVR_FUNC_RD_HOLDING_BIT = 0x04,  
	MB_TCP_SVR_FUNC_WR_COLIS_BIT = 0x10,	
    MB_TCP_SVR_FUNC_WR_HOLDING_BIT = 0x20
} MB_TCP_ServerFuncBit;

typedef struct {
	uint16_t unRegAddr;
	uint16_t unRegByteLen;
	void* pRegValue;
	uint32_t unMB_RegFuncEnBits;
	eMBErrorCode (*preReadF) (UCHAR * pucRegBuffer, USHORT usAddrOffset, USHORT usNBytes);
	eMBErrorCode (*postReadF) (UCHAR * pucRegBuffer, USHORT usAddrOffset, USHORT usNBytes);
	eMBErrorCode (*preWriteF) (UCHAR * pucRegBuffer, USHORT usAddrOffset, USHORT usNBytes);
	eMBErrorCode (*postWriteF) (UCHAR * pucRegBuffer, USHORT usAddrOffset, USHORT usNBytes);
} MB_RegAccessTypeDef;

#endif
