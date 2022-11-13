#ifndef CMDS_H
#define CMDS_H

#include "stm32f3xx_hal.h"
#include "main.h"
#include "circBuffer.h"

#define RXIT1_SIZE (10)

extern uint8_t rxIt1[RXIT1_SIZE];
extern cBuffHandle_t rxCirc1;

_Bool cmds_init();
void cmds_process();

static inline void cmds_UART1_HAL_UARTEx_RxEventCallback(
		UART_HandleTypeDef *huart, uint16_t Size)
{
	cBuff_write(&rxCirc1, rxIt1, Size);
	HAL_UARTEx_ReceiveToIdle_IT(huart, rxIt1, RXIT1_SIZE);
}

#endif
