#ifndef SREC_H
#define SREC_H

#include "typedefs.h"
#include <fstream>

using namespace std;

#define SREC_MAXCOUNT   (256)

typedef struct 
{
    UWord16 wNumElements;
    UWord32 dwAddress;
    union
    {
        unsigned char   by[SREC_MAXCOUNT];
        UWord16          w[SREC_MAXCOUNT/2];
        UWord32         dw[SREC_MAXCOUNT/4];
    }Data;
}SREC_t;


typedef struct 
{
	UWord32 ProgramLength;
	UWord32 ProgramCRC;
	UWord32 SwVersion;
	UWord32 HeaderCRC;
} PROG_HEADER_T;

class SRecord
{
public:
    SRecord(void);

    SRecord(	const char    *filename,
				int           data_width,
				unsigned int  start_addr,
				unsigned int  end_addr,
				unsigned int  header_addr);

    ~SRecord();

private:
    fstream         *m_fsSRecord;
    SREC_t          m_pSRecDecoded;
	PROG_HEADER_T   m_ProgramHeader;
    unsigned int    *m_pMemorySegment;

    int             m_wDataWidth;
	unsigned int    m_wSegLen;
	unsigned int    m_wSegStartAddress;
	unsigned int    m_wSegEndAddress;
	unsigned int    m_wHeaderAddr;
    unsigned char   m_byLineChkSum;
	unsigned int    m_wTotalElements;
    unsigned int    m_wError;


public:
    bool OpenSRecord(	const char    *filename,
                        int           data_width,
						int           start_addr,
						int           end_addr,
						int           header_addr);

    unsigned int CalculateCRC(unsigned int *crc, unsigned int *size);

    UWord16 SRec_Decode(void);

private:

    unsigned char SRec_NextDecodedByte(void);

    char SRec_NextChar(void);

    unsigned char ctoh(char cChar);

    void PlaceBytes(const SREC_t *pSRec);
};

#endif

