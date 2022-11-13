#include "checksum.h"

uint8_t checksum_calc(uint8_t *arr, uint16_t len)
{
	uint8_t csum = 0;

	for(uint16_t i=0; i<len; i++)
		csum += arr[i];

	return ~csum;
}
