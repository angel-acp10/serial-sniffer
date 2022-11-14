/******************************************************************************
 @file sniffer.h
 @brief
 *****************************************************************************/
#ifndef SNIFFER_H
#define SNIFFER_H

/******************************************************************************
 Includes
 *****************************************************************************/
#include "stm32f3xx_hal.h"
#include "timeStamp.h"
#include "fragmentsQueue.h"
#include <string.h>
#include "circBuffer.h"

/******************************************************************************
 Constants and definitions
 *****************************************************************************/
#define QUEUE_SIZE (20) // fragments

#define RXCIRC2_SIZE (4000)
#define RXCIRC3_SIZE (RXCIRC2_SIZE)

typedef enum{
	FRAGMENT_WAIT_FOR_FIRST_BYTE=0,
	FRAGMENT_WAIT_FOR_LAST_BYTE
}fragmentStage_t;

/******************************************************************************
 Global Variables
 *****************************************************************************/

extern cBuffHandle_t rxCirc2;
extern cBuffHandle_t rxCirc3;
extern uint8_t rxBuff2[RXCIRC2_SIZE];
extern uint8_t rxBuff3[RXCIRC3_SIZE];
extern volatile _Bool rxBuff2_ovf;
extern volatile _Bool rxBuff3_ovf;


extern volatile fragmentStage_t fragStage_uart2;
extern volatile fragmentStage_t fragStage_uart3;
extern volatile fragment_t currFrag_uart2;
extern volatile fragment_t currFrag_uart3;
extern queueFragHandle_t queueFrag;

/******************************************************************************
 Function Prototypes
 *****************************************************************************/
_Bool sniffer_init();
uint16_t sniffer_fillOutputBuffer(uint8_t *out, uint16_t maxSize, uint8_t *status);

/******************************************************************************
 Inlined Functions
 *****************************************************************************/
static inline void sniffer_HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart,
													volatile fragment_t *fr,
													volatile fragmentStage_t *stage,
													volatile _Bool *ovfFlag)
{
	if(*stage == FRAGMENT_WAIT_FOR_FIRST_BYTE)
	{
		*stage = FRAGMENT_WAIT_FOR_LAST_BYTE;
		fr->startTime = timeStamp_getValue();
		fr->startIdx = fr->cBuff->wIdx;
	}

	fr->cBuff->wIdx++;
	if(fr->cBuff->wIdx == fr->cBuff->maxLength)
		fr->cBuff->wIdx = 0;
	fr->cBuff->length++;

	if(fr->cBuff->length == fr->cBuff->maxLength)
		*ovfFlag = 1;

	uint8_t * ptr = &fr->cBuff->buff[fr->cBuff->wIdx];
	HAL_UARTEx_ReceiveToIdle_IT(huart, ptr, 1);
}

static inline void sniffer_USART_IRQHandler(UART_HandleTypeDef *huart,
											volatile fragment_t *fr,
											volatile fragmentStage_t *stage)
{
	uint16_t endIdx;

	if( __HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE)
			&& *stage == FRAGMENT_WAIT_FOR_LAST_BYTE )
	{
		*stage = FRAGMENT_WAIT_FOR_FIRST_BYTE;

		fr->endTime = timeStamp_getValue();

		if(fr->cBuff->wIdx == 0)
			endIdx = fr->cBuff->maxLength - 1;
		else
			endIdx = fr->cBuff->wIdx - 1;

		if(endIdx > fr->startIdx)
		{
			fr->length = (endIdx - fr->startIdx) + 1;
		}
		else
		{
			fr->length = (fr->cBuff->maxLength - fr->startIdx);
			fr->length += (endIdx + 1);
		}

		fragsQueue_enqueue(&queueFrag, (fragment_t *)fr);
	}
}

#endif
