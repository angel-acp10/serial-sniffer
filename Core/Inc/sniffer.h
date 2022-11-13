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
#define RXDMA2_SIZE (50)
#define RXDMA3_SIZE (50)

/******************************************************************************
 Global Variables
 *****************************************************************************/
extern volatile int8_t fragmentStage_uart2;
extern volatile int8_t fragmentStage_uart3;
extern volatile fragment_t currFragment_uart2;
extern volatile fragment_t currFragment_uart3;

extern cBuffHandle_t rxCirc2;
extern cBuffHandle_t rxCirc3;

extern uint8_t rxDma2[RXDMA2_SIZE]; // small buffers where DMA will write on
extern uint8_t rxDma3[RXDMA3_SIZE];

extern uint8_t rxBuff2[RXCIRC2_SIZE];
extern uint8_t rxBuff3[RXCIRC3_SIZE];
extern volatile _Bool rxBuff2_ovf;
extern volatile _Bool rxBuff3_ovf;

extern fragment_t fragBuff[QUEUE_SIZE];

extern queueFragHandle_t queueFrag;

/******************************************************************************
 Function Prototypes
 *****************************************************************************/
_Bool sniffer_init();
uint16_t sniffer_fillOutputBuffer(uint8_t *out, uint16_t maxSize, uint8_t *status);

/******************************************************************************
 Inlined Functions
 *****************************************************************************/
static inline void sniffer_UART2_HAL_UARTEx_RxEventCallback(
		UART_HandleTypeDef *huart, uint16_t Size)
{
	if(Size != cBuff_write(&rxCirc2, rxDma2, Size))
		rxBuff2_ovf = 1;
	HAL_UARTEx_ReceiveToIdle_DMA(huart, rxDma2, RXDMA2_SIZE);
	__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);
}

static inline void sniffer_UART3_HAL_UARTEx_RxEventCallback(
		UART_HandleTypeDef *huart, uint16_t Size)
{
	if(Size != cBuff_write(&rxCirc3, rxDma3, Size))
		rxBuff3_ovf = 1;
	HAL_UARTEx_ReceiveToIdle_DMA(huart, rxDma3, RXDMA3_SIZE);
	__HAL_DMA_DISABLE_IT(&hdma_usart3_rx, DMA_IT_HT);
}

static inline void uartIrq_startEndMessageDetection(UART_HandleTypeDef *huart,
													volatile fragment_t *fr,
													volatile int8_t *stage)
{
	uint16_t endIdx;

	switch(*stage)
	{
	case -1: // peripheral just initialized
		// when peripheral is initialized this function will be called
		// Just ignore this case
		*stage = 0;
		break;

	case 0: // first byte detection
		fr->startTime = timeStamp_getValue();
		fr->startIdx = fr->cBuff->wIdx;

		//prepare buffer for new entry
		*stage = 1;
		break;

	case 1: // end byte detection (idle line detection)
		if(__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE))
		{
			fr->endTime = timeStamp_getValue();

			if(fr->cBuff->wIdx == 0)
				endIdx = fr->cBuff->maxLength;
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
			*stage = 0;
		}
		break;
	default:
		break;
	}
}

#endif
