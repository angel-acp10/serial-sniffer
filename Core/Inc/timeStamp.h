/******************************************************************************
 @file timeStamp.h
 @brief
 *****************************************************************************/
#ifndef TIMESTAMP_H
#define TIMESTAMP_H

/******************************************************************************
 Includes
 *****************************************************************************/
#include "stm32f3xx_hal.h"
#include "main.h"

/******************************************************************************
 Constants and definitions
 *****************************************************************************/


/******************************************************************************
 Global Variables
 *****************************************************************************/
extern volatile uint16_t tenthsOfSeconds;

/******************************************************************************
 Function Prototypes
 *****************************************************************************/
_Bool timeStamp_start();
_Bool timeStamp_stopAndReset();
_Bool timeStamp_timeout(const uint32_t prevTStamp, const uint16_t ms_tout);

uint32_t timeStamp_getValueMs();
_Bool timeStamp_timeoutMs(const uint32_t prevTStamp_ms, const uint16_t tout_ms);

/******************************************************************************
 Inlined Functions
 *****************************************************************************/
static inline void timerIrq_updateTimeStamp()
{
	tenthsOfSeconds++;
}

static inline uint32_t timeStamp_getValue()
{
	uint32_t tStamp;

	uint16_t timCnt = htim6.Instance->CNT;
	tStamp = ((uint32_t)tenthsOfSeconds)<<16;
	tStamp |= (uint32_t)timCnt;

	return tStamp;
}

#endif
