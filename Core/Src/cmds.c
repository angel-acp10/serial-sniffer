/******************************************************************************
 Includes
 *****************************************************************************/
#include "cmds.h"
#include "sniffer.h"
#include "fragmentsQueue.h"
#include "statusCodes.h"
#include <string.h>
#include <stdio.h>

/******************************************************************************
 Constants and definitions
 *****************************************************************************/
#define CMDS_NUM (4)

// the maximum data that will be placed on TX_SIZE is RXCIRC2_SIZE + header data
#define TX_SIZE (RXCIRC2_SIZE + 50)
#define RX_SIZE (20)

#define RXCIRC1_SIZE (100)

/******************************************************************************
 Structures
 *****************************************************************************/
typedef struct{
	uint16_t len; // expected length of the command

	/**
	 * @brief Function pointer that executes a specific command and builds the
	 * response.
	 * @param in Buffer with the incoming message
	 * @param out Buffer where the response will be written
	 * @return Length of the written response
	 */
	uint16_t (*func)(const uint8_t *in, uint8_t *out);
}rxCmd_t;

/******************************************************************************
 Local Function Prototypes
 *****************************************************************************/
static uint16_t cmd_getId(const uint8_t *in, uint8_t *out);
static uint16_t cmd_initUart(const uint8_t *in, uint8_t *out);
static uint16_t cmd_deInitUart(const uint8_t *in, uint8_t *out);
static uint16_t cmd_getAllQueue(const uint8_t *in, uint8_t *out);

static void setUartConfig(UART_HandleTypeDef * h, const uint8_t * in);
static void writeToUart(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
static uint16_t shortReply(uint8_t *out, uint8_t cmd, uint8_t status);
static uint8_t calcChecksum(uint8_t *arr, uint16_t len);

/******************************************************************************
 Global variables
 *****************************************************************************/


// circular buffer
volatile _Bool rxCirc1_ovf = 0;
uint8_t rxBuff1[RXCIRC1_SIZE];
cBuffHandle_t rxCirc1;

/******************************************************************************
 Local variables
 *****************************************************************************/
static rxCmd_t cmdsMap[CMDS_NUM+1] = {
		{0, 	NULL},
		{4, 	&cmd_getId},		// 0x01
		{10, 	&cmd_initUart},		// 0x02
		{4, 	&cmd_deInitUart},	// 0x03
		{4,		&cmd_getAllQueue} 	// 0x04
};

/******************************************************************************
 Public Functions
 *****************************************************************************/
_Bool cmds_init()
{
	cBuff_init(&rxCirc1, rxBuff1, RXCIRC1_SIZE);

	if(HAL_OK != HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rxCirc1.buff, rxCirc1.maxLength) )
		return 0;
	__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);

	return 1;
}

void cmds_process()
{
	// read variables
	static uint8_t rx[RX_SIZE]; // rx buffer
	static uint8_t stage = 0;
	static uint8_t cmd;
	static uint16_t currLen = 0;
	static uint16_t expectedLen = 0;
	static uint32_t prevTStamp;
	uint16_t toRead;

	// write variables
	static uint8_t tx[TX_SIZE];
	uint16_t toWrite;

	switch(stage)
	{
	case 0: // get the command byte
		currLen = 0;
		toRead = cBuff_length(&rxCirc1);
		if( toRead>=1 )
		{
			cBuff_read(&rxCirc1, rx, 1);
			currLen++;

			cmd = rx[0];
			if( cmd!=0x00 && cmd<=CMDS_NUM )
			{
				prevTStamp = timeStamp_getValueMs();
				stage = 1;
				goto stage1;
			}
			// unknown command
		}
		break;

	case 1:
	stage1: // get the expected length of the command
		toRead = cBuff_length(&rxCirc1);
		if( toRead>=2 )
		{
			prevTStamp = timeStamp_getValueMs();

			cBuff_read(&rxCirc1, &rx[currLen], 2);
			currLen += 2;

			expectedLen = (((uint16_t)rx[1])<<8) | ((uint16_t)rx[2]);
			expectedLen += 3; // cmd (1 byte) + num bytes (2 bytes)

			if(expectedLen == cmdsMap[cmd].len)
			{
				stage = 2;
				goto stage2;
			}
			// unknown command
			stage = 0;
		}
		else if(timeStamp_timeoutMs(prevTStamp, 10))
			stage = 0;

		break;

	case 2:
	stage2: // get the full command
		toRead = cBuff_length(&rxCirc1);
		if( toRead>0 )
		{
			prevTStamp = timeStamp_getValueMs();

			// only read the remaining bytes to get the full command
			if(toRead > (expectedLen - currLen) )
				toRead = (expectedLen - currLen);

			cBuff_read(&rxCirc1, &rx[currLen], toRead);
			currLen += toRead;

			if(currLen == expectedLen)
			{
				stage = 3;
				goto stage3;
			}
		}
		else if(timeStamp_timeoutMs(prevTStamp, 10))
			stage = 0;

		break;

	case 3:
	stage3: // check the checksum, execute the command and send a reply
		if( rx[currLen-1] != calcChecksum(rx, currLen-1))
			toWrite = shortReply(tx, ERROR_CHECKSUM, ERROR_CHECKSUM);
		else if( cmdsMap[cmd].func == NULL )
			toWrite = shortReply(tx, STATUS_FAIL, STATUS_FAIL);
		else
			toWrite = cmdsMap[cmd].func(rx, tx);

		tx[toWrite-1] = calcChecksum(tx, toWrite-1);
		writeToUart(&huart1, tx, toWrite);
		stage = 0;
		break;

	default:
		break;
	}
}

/******************************************************************************
 Local Functions
 *****************************************************************************/
static uint16_t cmd_getId(const uint8_t *in, uint8_t *out)
{
	uint16_t maxNameLen = 25;
	uint16_t bytesToSend = 5 + maxNameLen;

	out[0] = in[0];
	out[1] = (uint8_t)(((bytesToSend-3)&0xFF00)>>8);
	out[2] = (uint8_t)((bytesToSend-3)&0x00FF);
	out[3] = STATUS_OK;
	memset(&out[4], '\0', maxNameLen);
	snprintf((char*)&out[4], maxNameLen, STATUS_MCU_NAME);
	out[bytesToSend-1] = calcChecksum(out, bytesToSend-1);
	return bytesToSend;
}

static uint16_t cmd_initUart(const uint8_t *in, uint8_t *out)
{
	huart2.Instance = USART2;
	huart3.Instance = USART3;
	setUartConfig(&huart2, in);
	setUartConfig(&huart3, in);

	if (HAL_OK==HAL_UART_Init(&huart2) && HAL_OK==HAL_UART_Init(&huart3))
	{
		sniffer_init();
		return shortReply(out, in[0], STATUS_OK);
	}
	return shortReply(out, in[0], STATUS_FAIL);
}

static uint16_t cmd_deInitUart(const uint8_t *in, uint8_t *out)
{
	if(HAL_OK==HAL_UART_DeInit(&huart2) && HAL_OK==HAL_UART_DeInit(&huart3))
		return shortReply(out, in[0], STATUS_OK);

	return shortReply(out, in[0], STATUS_FAIL);
}

static uint16_t cmd_getAllQueue(const uint8_t *in, uint8_t *out)
{
	uint16_t bytesToSend;
	uint8_t status;
	uint32_t tStamp;

	bytesToSend = 5 + sniffer_fillOutputBuffer(&out[4], TX_SIZE-5, &status);
	switch(status)
	{
	case STATUS_OK:
		out[0] = in[0];
		out[1] = (uint8_t)(((bytesToSend-3)&0xFF00)>>8);
		out[2] = (uint8_t)((bytesToSend-3)&0x00FF);
		out[3] = status;
		// out[bytesToSend-1] = calcChecksum(out, bytesToSend-1);
		return bytesToSend;

	case STATUS_EMPTY_FRAGQUEUE:
		bytesToSend = 9;
		tStamp = timeStamp_getValue();
		out[0] = in[0];
		out[1] = (uint8_t)(((bytesToSend-3)&0xFF00)>>8);
		out[2] = (uint8_t)((bytesToSend-3)&0x00FF);
		out[3] = status;
		out[4] = (uint8_t)((0xFF000000&tStamp) >> 24);
		out[5] = (uint8_t)((0x00FF0000&tStamp) >> 16);
		out[6] = (uint8_t)((0x0000FF00&tStamp) >> 8);
		out[7] = (uint8_t) (0x000000FF&tStamp);
		// out[bytesToSend-1] = calcChecksum(out, bytesToSend-1);
		return bytesToSend;

	case STATUS_TXCIRC1_OVF:
	case STATUS_RXCIRC2_OVF:
	case STATUS_RXCIRC3_OVF:
		return shortReply(out, in[0], status);

	default:
		return 0;
	}
}

static void setUartConfig(UART_HandleTypeDef * h, const uint8_t * in)
{
	uint32_t bauds;
	uint8_t mode;
	uint8_t wordLength, parity, stopBits;

	mode = in[3];
	bauds = ((uint32_t)in[4])<<16;
	bauds |= ((uint32_t)in[5])<<8;
	bauds |= ((uint32_t)in[6]);

	wordLength = (in[7] & (1<<7))>>7;
	parity = (in[7] & (0b11<<5))>>5;
	stopBits = (in[7] & (0b11<<3))>>3;

	h->Init.Mode = UART_MODE_TX_RX;

	switch(mode)
	{
	case 3:	h->Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;	break;
	case 2:	h->Init.HwFlowCtl = UART_HWCONTROL_RTS;		break;
	case 1:	h->Init.HwFlowCtl = UART_HWCONTROL_CTS;		break;
	default:
	case 0:	h->Init.HwFlowCtl = UART_HWCONTROL_NONE;	break;
	}
	huart2.Init.BaudRate = bauds;
	h->Init.BaudRate = bauds;

	switch(wordLength)
	{
	case 1:	h->Init.WordLength = UART_WORDLENGTH_9B;	break;
	default:
	case 0:	h->Init.WordLength = UART_WORDLENGTH_8B;	break;
	}

	switch(parity)
	{
	case 2:	h->Init.Parity = UART_PARITY_ODD;	break;
	case 1:	h->Init.Parity = UART_PARITY_EVEN;	break;
	default:
	case 0:	h->Init.Parity = UART_PARITY_NONE;	break;
	}

	switch(stopBits)
	{
	case 1:	h->Init.StopBits = UART_STOPBITS_2;	break;
	default:
	case 0:	h->Init.StopBits = UART_STOPBITS_1;	break;
	}

	h->Init.OverSampling = UART_OVERSAMPLING_16;
	h->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	h->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
}

static void writeToUart(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
	HAL_StatusTypeDef txStatus = HAL_BUSY;
	while(txStatus == HAL_BUSY)
	{
		txStatus = HAL_UART_Transmit_DMA(huart, pData, Size);
	}
	if(txStatus == HAL_OK)
		return;
	else
		while(1);
}

static uint16_t shortReply(uint8_t *out, uint8_t cmd, uint8_t status)
{
	uint16_t bytesToSend = 5;
	out[0] = cmd;
	out[1] = (uint8_t)(((bytesToSend-3)&0xFF00)>>8);
	out[2] = (uint8_t)((bytesToSend-3)&0x00FF);
	out[3] = status;
	// out[4] checksum
	return bytesToSend;
}

static uint8_t calcChecksum(uint8_t *arr, uint16_t len)
{
	uint8_t csum = 0;

	for(uint16_t i=0; i<len; i++)
		csum += arr[i];

	return ~csum;
}
