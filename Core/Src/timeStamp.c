/******************************************************************************
 Includes
 *****************************************************************************/
#include "timeStamp.h"

/******************************************************************************
 Constants and definitions
 *****************************************************************************/
#define MAX_TSTAMP_MS ((50000/500) + (65535*100))

/******************************************************************************
 Structures
 *****************************************************************************/


/******************************************************************************
 Global variables
 *****************************************************************************/
volatile uint16_t tenthsOfSeconds;

/******************************************************************************
 Local variables
 *****************************************************************************/


/******************************************************************************
 Local Function Prototypes
 *****************************************************************************/


/******************************************************************************
 Public Functions
 *****************************************************************************/
_Bool timeStamp_start()
{
	if(HAL_OK != HAL_TIM_Base_Start_IT(&htim6))
		return 0;
	return 1;
}

_Bool timeStamp_stopAndReset()
{
	if(HAL_OK != HAL_TIM_Base_Stop_IT(&htim6))
		return 0;

	tenthsOfSeconds = 0;
	return 1;
}

uint32_t timeStamp_getValueMs()
{
	// timer steps of 2 us
	// 1ms = 1000us = 500*2us
	uint32_t tStamp_ms = htim6.Instance->CNT/500;

	// tenthsOfSeconds
	// 0.1s = 100ms
	tStamp_ms += tenthsOfSeconds*100;

	return tStamp_ms;

	// MAX_TSTAMP_MS
	// tStamp_ms = 50000/500 + 65535*100;
	// tStamp_ms = 6553600
}

_Bool timeStamp_timeoutMs(const uint32_t prevTStamp_ms, const uint16_t tout_ms)
{
	uint32_t currTStamp_ms = timeStamp_getValueMs();
	uint32_t diff;

	if(prevTStamp_ms > currTStamp_ms)
	{
		diff = MAX_TSTAMP_MS - prevTStamp_ms;
		diff += (currTStamp_ms + 1);
	}
	else
		diff = currTStamp_ms - prevTStamp_ms;

	if(diff >= tout_ms)
		return 1;
	return 0;
}

/******************************************************************************
 Local Functions
 *****************************************************************************/

