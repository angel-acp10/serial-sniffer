#ifndef STATUSCODES_H
#define STATUSCODES_H

// this is the only response where cmd = status = ERROR_CHECKSUM
#define ERROR_CHECKSUM 		(0xFF)

// cmd byte + status
#define STATUS_OK 				(0x00)
#define STATUS_EMPTY_FRAGQUEUE	(0x01)
#define STATUS_FAIL				(0x10)
#define STATUS_TXBUFF1_OVF		(0x11)
#define STATUS_RXBUFF2_OVF		(0x12)
#define STATUS_RXBUFF3_OVF		(0x13)

#define STATUS_MCU_NAME		("STM32F302R8")

#endif
