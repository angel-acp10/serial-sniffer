#ifndef CMDS_H
#define CMDS_H

#include "stm32f3xx_hal.h"
#include "main.h"
#include "circBuffer.h"

#define MIN(A,B) ((A)<(B) ? (A) : (B))

extern cBuffHandle_t rxCirc1;
extern volatile _Bool rxCirc1_ovf;

_Bool cmds_init();
void cmds_process();

static inline void cmds_UART1_HAL_UARTEx_RxEventCallback(
		UART_HandleTypeDef *huart, uint16_t Size)
{
	uint16_t availBytes;
	uint16_t dmaMaxBytes;


	cBuff_incrementWriteIdx(&rxCirc1, Size);

	availBytes = cBuff_availBytes(&rxCirc1);
	dmaMaxBytes = cBuff_maxBytesUntilArrayOvf(&rxCirc1);

	if(availBytes == 0)
	{
		rxCirc1_ovf = 1;
		while(1);
	}

	HAL_UARTEx_ReceiveToIdle_DMA(huart,
			&rxCirc1.buff[rxCirc1.wIdx],
			MIN(availBytes, dmaMaxBytes));
	__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
}

#endif
