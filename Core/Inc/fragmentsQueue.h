#ifndef FRAGMENTSQUEUE_H
#define FRAGMENTSQUEUE_H

#include "circBuffer.h"
#include <string.h>

typedef struct{
	// this struct contains all necessary information about a fragment read by USART2 o USART3

	uint8_t uartId; // uart where the data comes. It can be USART2 (0x02) or USART3 (0x03)

	cBuffHandle_t *cBuff; // circular buffer where the data resides
	uint16_t startIdx; // index of the "circBuffPtr->buff" where the fragment starts
	uint16_t length; // length of the data fragment

	uint32_t startTime; // start time stamp based on timer
	uint32_t endTime; // end time stamp based on timer
}fragment_t;

typedef struct{
	fragment_t *buff;
	volatile uint16_t wIdx;
	uint16_t rIdx;
	volatile uint16_t length;
	uint16_t maxLength;
}queueFragHandle_t;

static inline void fragsQueue_init(queueFragHandle_t *h, fragment_t *array, uint16_t maxLength)
{
	h->buff = array;
	h->wIdx = 0;
	h->rIdx = 0;
	h->length = 0;
	h->maxLength = maxLength;
}

/**
 * @brief Returns the number of queued fragments
 */
static inline uint16_t fragsQueue_length(queueFragHandle_t * h)
{
	return h->length;
}

/**
 * @brief Returns the length of the next fragment (without altering the queue indexes)
 */
static inline uint16_t fragsQueue_getLengthOfNextFragment(queueFragHandle_t * h)
{
	return h->buff[h->rIdx].length;
}

static inline void fragsQueue_clear(queueFragHandle_t * h)
{
	h->rIdx = h->wIdx;
	h->length = 0;
}

static inline _Bool fragsQueue_enqueue(queueFragHandle_t * h, fragment_t *frag)
{
	if(h->length >= h->maxLength)
		return 0;

	memcpy( &h->buff[h->wIdx], frag, sizeof(fragment_t));
	h->wIdx++;
	if(h->wIdx >= h->maxLength)
		h->wIdx = 0;
	h->length++;

	return 1;
}

static inline _Bool fragsQueue_dequeue(queueFragHandle_t * h, fragment_t *frag)
{
	if(h->length == 0)
		return 0;

	memcpy( frag, &h->buff[h->rIdx], sizeof(fragment_t));
	h->rIdx++;
	if(h->rIdx >= h->maxLength)
		h->rIdx = 0;
	h->length--;

	return 1;
}

#endif
