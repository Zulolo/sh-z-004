#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "sh_z_004.h"
#include "main.h"

GPIO_TypeDef* pDO_Ports[SH_Z_004_DO_NUM] = {DO_0_GPIO_Port, DO_1_GPIO_Port, DO_2_GPIO_Port, DO_3_GPIO_Port};
uint16_t uDO_Pins[SH_Z_004_DO_NUM] = {DO_0_Pin, DO_1_Pin, DO_2_Pin, DO_3_Pin};	






