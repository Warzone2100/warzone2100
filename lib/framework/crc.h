#ifndef _CRC_H_
#define _CRC_H_

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

uint32_t crcSum(uint32_t crc, const void *data, size_t dataLen);
uint32_t crcSumU16(uint32_t crc, const uint16_t *data, size_t dataLen);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_CRC_H_
