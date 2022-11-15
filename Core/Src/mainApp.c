#include <sniffer.h>
#include "mainApp.h"

#include "stm32f3xx_hal.h"
#include "timeStamp.h"
#include "cmds.h"


void mainApp()
{
	// in "main.c" CubeMx has already initialized UART2 and 3
	// deinit them at startup
	HAL_UART_DeInit(&huart2);
	HAL_UART_DeInit(&huart3);

	// initializations
	timeStamp_start();
	cmds_init();

	while(1)
	{
		cmds_process();
	}
}

