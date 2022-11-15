#ifndef STATUSCODES_H
#define STATUSCODES_H

// this is the only response where cmd = status = ERROR_CHECKSUM
#define ERROR_CHECKSUM 		(0xFF)

// cmd byte + status
#define STATUS_OK 				(0x55)
#define STATUS_EMPTY_FRAGQUEUE	(0x56)
#define STATUS_FAIL				(0xAA)
#define STATUS_TXCIRC1_OVF		(0xAB)
#define STATUS_RXCIRC2_OVF		(0xAC)
#define STATUS_RXCIRC3_OVF		(0xAD)

#define STATUS_MCU_NAME		("STM32F302R8")

#endif
