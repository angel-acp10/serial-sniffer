/******************************************************************************
 @file sniffer.c
 @brief
 *****************************************************************************/

/******************************************************************************
 Includes
 *****************************************************************************/
#include "sniffer.h"
#include "main.h"
#include "timeStamp.h"
#include "circBuffer.h"
#include "statusCodes.h"


/******************************************************************************
 Constants and definitions
 *****************************************************************************/


/******************************************************************************
 Structures
 *****************************************************************************/


/******************************************************************************
 Global variables
 *****************************************************************************/
volatile fragmentStage_t fragStage_uart2;
volatile fragmentStage_t fragStage_uart3;
volatile fragment_t currFrag_uart2;
volatile fragment_t currFrag_uart3;

cBuffHandle_t rxCirc2;
cBuffHandle_t rxCirc3;

volatile _Bool rxCirc2_ovf = 0;
volatile _Bool rxCirc3_ovf = 0;

/******************************************************************************
 Local variables
 *****************************************************************************/
uint8_t rxBuff2[RXCIRC2_SIZE];
uint8_t rxBuff3[RXCIRC3_SIZE];

fragment_t fragBuff[QUEUE_SIZE];

queueFragHandle_t queueFrag;

/******************************************************************************
 Local Function Prototypes
 *****************************************************************************/
static uint16_t sniffer_getFragmentWithHeader(uint8_t *out);

/******************************************************************************
 Public Functions
 *****************************************************************************/
_Bool sniffer_init()
{
	cBuff_init(&rxCirc2, rxBuff2, RXCIRC2_SIZE);
	cBuff_init(&rxCirc3, rxBuff3, RXCIRC3_SIZE);

	fragsQueue_init(&queueFrag, fragBuff, QUEUE_SIZE);
	
	fragStage_uart2 = FRAGMENT_WAIT_FOR_FIRST_BYTE;
	currFrag_uart2.uartId = 0x02;
	currFrag_uart2.cBuff = &rxCirc2;
	currFrag_uart2.startIdx = 0;
	currFrag_uart2.length = 0;
	currFrag_uart2.startTime = 0;
	currFrag_uart2.endTime = 0;

	fragStage_uart3 = FRAGMENT_WAIT_FOR_FIRST_BYTE;
	currFrag_uart3.uartId = 0x03;
	currFrag_uart3.cBuff = &rxCirc3;
	currFrag_uart3.startIdx = 0;
	currFrag_uart3.length = 0;
	currFrag_uart3.startTime = 0;
	currFrag_uart3.endTime = 0;


	if( HAL_OK != HAL_UARTEx_ReceiveToIdle_IT(&huart2, rxCirc2.buff, 1) )
		return 0;

	if( HAL_OK != HAL_UARTEx_ReceiveToIdle_IT(&huart3, rxCirc3.buff, 1) )
		return 0;


	return 1;
}

uint16_t sniffer_fillOutputBuffer(uint8_t *out, uint16_t maxSize, uint8_t *status)
{
	uint16_t size = 0;
	uint16_t nextChunck;
	uint16_t fragsCnt = 0;

	if(rxCirc2_ovf)
	{
		*status = STATUS_RXCIRC2_OVF;
		return 0;
	}
	else if(rxCirc3_ovf)
	{
		*status = STATUS_RXCIRC3_OVF;
		return 0;
	}
	else if( fragsQueue_length(&queueFrag) == 0)
	{
		*status = STATUS_EMPTY_FRAGQUEUE;
		return 0;
	}

	*status = STATUS_OK;
	while( fragsQueue_length(&queueFrag)>0 )
	{
		nextChunck = 11 + fragsQueue_getLengthOfNextFragment(&queueFrag);
		if( (size + nextChunck) > maxSize)
		{
			if(fragsCnt == 0) // fragment is too long to fit into output buffer
				*status = STATUS_TXCIRC1_OVF;
			else
				break; // no more fragments fit into output buffer
		}
		sniffer_getFragmentWithHeader(&out[size]);
		fragsCnt++;
		size += nextChunck;
	}
	return size;
}


/******************************************************************************
 Local Functions
 *****************************************************************************/
/**
 * @brief This function returns a formated array of bytes ready to be sent
 * to the PC. Once the array is generated, removes the fragment from the
 * queue.
 */
static uint16_t sniffer_getFragmentWithHeader(uint8_t *out)
{
	uint16_t bytesToReturn;
	fragment_t f;

	fragsQueue_dequeue(&queueFrag, &f);

	// uartId - 1 byte
	out[0] = f.uartId;
	bytesToReturn = 11 + f.length;

	// num bytes - 2 bytes
	out[1] = (uint8_t)(((bytesToReturn-3)&0xFF00)>>8);
	out[2] = (uint8_t)((bytesToReturn-3)&0x00FF);

	// startTime - 4 bytes
	out[3] = (uint8_t)((0xFF000000&f.startTime) >> 24);
	out[4] = (uint8_t)((0x00FF0000&f.startTime) >> 16);
	out[5] = (uint8_t)((0x0000FF00&f.startTime) >> 8);
	out[6] = (uint8_t) (0x000000FF&f.startTime);

	// endTime - 4 bytes
	out[7] = (uint8_t)((0xFF000000&f.endTime) >> 24);
	out[8] = (uint8_t)((0x00FF0000&f.endTime) >> 16);
	out[9] = (uint8_t)((0x0000FF00&f.endTime) >> 8);
	out[10] = (uint8_t) (0x000000FF&f.endTime);

	/*
	uint8_t val = f.cBuff->buff[ f.startIdx ];
	if(val!= (uint8_t)'a' && val != (uint8_t)'0')
		HAL_GPIO_WritePin(YELLOW_LED_GPIO_Port, YELLOW_LED_Pin, GPIO_PIN_SET);
	*/

	// data of fragment
	cBuff_read(f.cBuff, &out[11], f.length);

	/*
	if(out[11] != (uint8_t)'a' && out[11] != (uint8_t)'0')
		HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET);
	*/

	return bytesToReturn;
}


