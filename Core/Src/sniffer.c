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
volatile int8_t fragmentStage_uart2;
volatile int8_t fragmentStage_uart3;
volatile fragment_t currFragment_uart2;
volatile fragment_t currFragment_uart3;

cBuffHandle_t rxCirc2;
cBuffHandle_t rxCirc3;

volatile _Bool rxBuff2_ovf = 0;
volatile _Bool rxBuff3_ovf = 0;

/******************************************************************************
 Local variables
 *****************************************************************************/
uint8_t rxDma2[RXDMA2_SIZE]; // small buffers where DMA will write on
uint8_t rxDma3[RXDMA3_SIZE];

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
	
	fragmentStage_uart2 = -1;
	currFragment_uart2.uartId = 0x02;
	currFragment_uart2.cBuff = &rxCirc2;
	currFragment_uart2.startIdx = 0;
	currFragment_uart2.length = 0;
	currFragment_uart2.startTime = 0;
	currFragment_uart2.endTime = 0;

	fragmentStage_uart3 = -1;
	currFragment_uart3.uartId = 0x03;
	currFragment_uart3.cBuff = &rxCirc3;
	currFragment_uart3.startIdx = 0;
	currFragment_uart3.length = 0;
	currFragment_uart3.startTime = 0;
	currFragment_uart3.endTime = 0;

	if( HAL_OK != HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rxDma2, RXDMA2_SIZE) )
		return 0;

	if( HAL_OK != HAL_UARTEx_ReceiveToIdle_DMA(&huart3, rxDma3, RXDMA3_SIZE) )
		return 0;

	__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);
	__HAL_DMA_DISABLE_IT(&hdma_usart3_rx, DMA_IT_HT);

	return 1;
}

uint16_t sniffer_fillOutputBuffer(uint8_t *out, uint16_t maxSize, uint8_t *status)
{
	uint16_t size = 0;
	uint16_t nextChunck;
	uint16_t fragsCnt = 0;

	if(rxBuff2_ovf)
	{
		*status = STATUS_RXBUFF2_OVF;
		return 0;
	}
	else if(rxBuff3_ovf)
	{
		*status = STATUS_RXBUFF3_OVF;
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
				*status = STATUS_TXBUFF1_OVF;
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

	// data of fragment
	cBuff_read(&f.cBuff[f.startIdx], &out[11], f.length);

	return bytesToReturn;
}


