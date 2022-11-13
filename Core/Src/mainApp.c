#include <sniffer.h>
#include "mainApp.h"

#include "stm32f3xx_hal.h"
#include "timeStamp.h"
#include "cmds.h"


void mainApp()
{
	sniffer_init();
	timeStamp_start();
	cmds_init();

	while(1)
	{
		cmds_process();
	}
}

