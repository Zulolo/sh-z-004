#ifndef _AI_MONITOR_H
#define _AI_MONITOR_H

#include "cmsis_os.h"
#include "sh_z_002.h"

void AI_get_AI_values(uint16_t* pAI_Values);
void AI_get_AI_current(int32_t* pAI_Current);
void AI_get_AI_current_high_thrld(int32_t* pAI_Current);
void AI_get_AI_current_low_thrld(int32_t* pAI_Current);
void AI_get_AI_current_hstrcl_max(int32_t* pAI_Current);
void AI_get_AI_current_hstrcl_min(int32_t* pAI_Current);
void AI_set_AI_current_high_thrld(int32_t* pAI_Current);
void AI_set_AI_current_low_thrld(int32_t* pAI_Current);
void AI_set_AI_low_alarm(uint32_t unValue);
void AI_set_AI_high_alarm(uint32_t unValue);
uint32_t AI_get_AI_low_alarm(void);
uint32_t AI_get_AI_high_alarm(void);

#endif
