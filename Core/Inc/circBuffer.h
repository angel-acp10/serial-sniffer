/******************************************************************************
 @file circBuffer.c
 @brief
 *****************************************************************************/

#ifndef CIRCBUFFER_H
#define CIRCBUFFER_H

#include "stm32f3xx_hal.h"
#include <string.h>

typedef struct{
	uint8_t *buff;
	volatile uint16_t wIdx;
	volatile uint16_t rIdx;
	volatile uint16_t length; // current length
	uint16_t maxLength;
}cBuffHandle_t;

static inline void cBuff_init(cBuffHandle_t *h, uint8_t *array, uint16_t maxLength)
{
	h->buff = array;
	h->wIdx = 0;
	h->rIdx = 0;
	h->length = 0;
	h->maxLength = maxLength;
}

static inline uint16_t cBuff_length(cBuffHandle_t * h)
{
	return h->length;
}

static inline uint16_t cBuff_availBytes(cBuffHandle_t * h)
{
	return (h->maxLength - h->length);
}

/**
 * @brief Returns the number of bytes between the current write index and the
 * end of the array (h->buff). Prevents array overflows.
 * 
 * @param h 
 * @return uint16_t 
 */
static inline uint16_t cBuff_maxBytesUntilArrayOvf(cBuffHandle_t * h)
{
	return (h->maxLength - h->wIdx);
}

static inline void cBuff_clear(cBuffHandle_t * h)
{
	h->rIdx = h->wIdx;
	h->length = 0;
}

static inline uint16_t cBuff_write(cBuffHandle_t * h, uint8_t *data, uint16_t size)
{
	uint16_t size1, size2;

	if( size > (h->maxLength - h->length) )
		size = h->maxLength - h->length; // there is not enough space, only copy what fits

	if( (h->wIdx + size) > h->maxLength )
	{
		size1 = h->maxLength - h->wIdx;
		memcpy( &h->buff[h->wIdx], data, size1 );

		size2 = (size - size1);
		h->wIdx = 0;
		memcpy( &h->buff[h->wIdx], &data[size1], size2 );

		h->wIdx += size2;
		h->length += size;
	}
	else
	{
		memcpy( &h->buff[h->wIdx], data, size );
		h->wIdx += size;
		if(h->wIdx >= h->maxLength)
			h->wIdx = 0;
		h->length += size;
	}
	return size;
}

/**
 * @brief Same as cBuff_write but without modifying the buffer content
 * Useful when buffer is modified by DMA but indexes are not.
 */
static inline uint16_t cBuff_incrementWriteIdx(cBuffHandle_t * h, uint16_t size)
{
	uint16_t size1, size2;

	if( size > (h->maxLength - h->length) )
		size = h->maxLength - h->length; // there is not enough space, only copy what fits

	if( (h->wIdx + size) > h->maxLength )
	{
		size1 = h->maxLength - h->wIdx;
		size2 = (size - size1);
		h->wIdx = 0;
		h->wIdx += size2;
		h->length += size;
	}
	else
	{
		h->wIdx += size;
		if(h->wIdx >= h->maxLength)
			h->wIdx = 0;
		h->length += size;
	}
	return size;	
}

static inline uint16_t cBuff_read(cBuffHandle_t * h, uint8_t *data, uint16_t size)
{
	uint16_t size1, size2;

	if( size > h->length )
		size = h->length; // user is requesting more data than the available

	if( (h->rIdx + size) > h->maxLength)
	{
		size1 = h->maxLength - h->rIdx;
		memcpy( data, &h->buff[h->rIdx], size1 );

		size2 = (size - size1);
		h->rIdx = 0;
		memcpy( &data[size1], &h->buff[h->rIdx], size2 );

		h->rIdx += size2;
		h->length -= size;
	}
	else
	{
		memcpy( data, &h->buff[h->rIdx], size );
		h->rIdx += size;
		if(h->rIdx >= h->maxLength)
			h->rIdx = 0;
		h->length -= size;
	}
	return size;
}

#endif
