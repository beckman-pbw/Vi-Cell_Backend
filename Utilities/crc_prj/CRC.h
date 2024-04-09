#ifndef CRC_H
#define CRC_H

#include "stdafx.h"
#include "typedefs.h"

UWord32 CRC_ComputeCRC(unsigned char *data, unsigned int seg_len, unsigned int wCRC);

#endif
