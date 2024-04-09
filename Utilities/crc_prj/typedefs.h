#ifndef _TYPEDEFS_H
#define _TYPEDEFS_H

typedef unsigned __int16    UWord16;
typedef __int16             Word16;
typedef unsigned __int32    UWord32;
typedef __int32             Word32;
typedef unsigned char       BYTE;

#define ERROR_OK                                (0)
#define ERROR_TOO_FEW_ARGUMENTS                 (1)
#define ERROR_INCORRECT_NUMBER_OR_ARGS          (2)
#define ERROR_INVALID_ARG                       (3)
#define ERROR_CANNOT_OPEN_SRECORD               (4)
#define ERROR_INVALID_SRECORD                   (5)
#define ERROR_INTERNAL_ERROR                    (6)

#define ERROR_SREC_FORMAT                       (1000)
#define ERROR_SREC_CHECKSUM                     (1001)
#define ERROR_SREC_FLAG_LASTREC                 (1002)
#define ERROR_SREC_FLAG_ZEROREC                 (1003)
#define ERROR_SREC_EOR                          (1004)

#define DATA_WIDTH_8_BITS                       (8)
#define DATA_WIDTH_16_BITS                      (16)
#define DATA_WIDTH_32_BITS                      (32)

#endif
