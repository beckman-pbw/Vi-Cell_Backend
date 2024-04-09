#ifndef _MACROS_H
#define _MACROS_H

#include "typedefs.h"

#define CLEARBIT(aa, bb)        ((aa) = (aa) & ~(bb))
#define SETBIT(aa, bb)          ((aa) = (aa) |  (bb))
#define TOGGLEBIT(aa, bb)       ((aa) = (aa) ^  (bb))

#define ISBITSET(aa, bb)        (((aa) & (bb)) == (bb))
#define ISBITCLEAR(aa, bb)      (((aa) & (bb)) == 0)

#define MAKEUWORD(ub, lb)       (UWord16)(((UWord16)(ub) << 8) | (UWord16)(lb))

#define MAKEULONG(uw, lw)       (UWord32)(((UWord32)(uw) << 16) | (UWord32)(lw))

#define LOWERBYTE(ww)           ( (ww) & 0x00FF)
#define UPPERBYTE(ww)           (((ww) & 0xFF00) >> 8)

#define LOWERWORD(dw)           (UWord16)( (dw) & 0x0000FFFF)
#define UPPERWORD(dw)           (UWord16)(((dw) & 0xFFFF0000) >> 16)

#define MIN(aa, bb)             ((aa) < (bb) ? (aa) : (bb))
#define MAX(aa, bb)             ((aa) > (bb) ? (aa) : (bb))

#define ISVALIDDATA( data, min, max )   ((data >= min) && (data <= max))
#define ISBETWEEN( data, min, max )     ((data >  min) && (data <  max))

#endif
