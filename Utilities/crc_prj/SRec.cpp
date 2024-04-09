#include <stdlib.h>
#include "stdafx.h"
#include "SRec.h"
#include <intrin.h>
#include "CRC.h"
#include "macros.h"
#include "typedefs.h"

SRecord::SRecord()
{
    // Do nothing
    m_fsSRecord = NULL;
    m_pMemorySegment = NULL;
}

SRecord::SRecord(const char *filename, int data_width, unsigned int  start_addr, unsigned int  end_addr, unsigned int  header_addr)
{
    m_fsSRecord = new fstream(filename);

    m_wDataWidth = data_width;
	m_wSegStartAddress = start_addr;
	m_wSegEndAddress = end_addr;
	m_wHeaderAddr = header_addr;
	m_wSegLen = end_addr - start_addr + 1;

	m_pMemorySegment = new unsigned int[m_wSegLen];
    memset(m_pMemorySegment, 0xFF, m_wSegLen * sizeof(m_pMemorySegment[0]));

	m_wError = ERROR_OK;
	m_byLineChkSum = 0;
	m_wTotalElements = 0;
	m_ProgramHeader.HeaderCRC = 0;
	m_ProgramHeader.ProgramCRC = 0;
	m_ProgramHeader.ProgramLength = 0;
	m_ProgramHeader.SwVersion = 0;
}


SRecord::~SRecord()
{
    if (NULL != m_fsSRecord)
    {
        m_fsSRecord->close();
        delete m_fsSRecord;
    }
    if (NULL != m_pMemorySegment)
    {
        delete m_pMemorySegment;
    }
}

bool SRecord::OpenSRecord(const char *filename, int data_width, int start_addr, int  end_addr, int  header_addr)
{
    return false;
}

unsigned int SRecord::CalculateCRC(unsigned int *crc, unsigned int *size)
{
	m_ProgramHeader.ProgramLength = (m_wTotalElements - 4u) * (m_wDataWidth / 8u); // Total elements counted includes Program header, discard for storing in flash.
	*size = m_ProgramHeader.ProgramLength;

	m_ProgramHeader.ProgramCRC = 0xFFFFFFFFu;
	m_ProgramHeader.ProgramCRC = CRC_ComputeCRC((unsigned char *)m_pMemorySegment, m_ProgramHeader.ProgramLength, m_ProgramHeader.ProgramCRC);

    if (NULL != crc)
    {
        char         sHeaderLine[SREC_MAXCOUNT];
		char         sSrecLine[SREC_MAXCOUNT];
        char         curr_char = 0;
        int          last_line = -1;
		unsigned int recAddress = 0;
		unsigned int addrSize = 2;

		*crc = m_ProgramHeader.ProgramCRC;

		m_fsSRecord->clear();
		m_fsSRecord->seekg(0, ios::beg);

		while (1)
		{
			addrSize = 2;
			recAddress = 0;
			last_line = (int)m_fsSRecord->tellg();
			m_fsSRecord->getline(sSrecLine, SREC_MAXCOUNT);
			if ((sSrecLine[1] != '1') && (sSrecLine[1] != '2') && (sSrecLine[1] != '3'))
			{
				continue;
			}

			addrSize += (sSrecLine[1] - '1');

			for (unsigned int i = 0; i < (addrSize * 2u); i += 2)
			{
				recAddress <<= 8u;
				recAddress |= ((ctoh(sSrecLine[4u + i]) << 4u) | ctoh(sSrecLine[5u + i]));
			}

			if (recAddress != m_wHeaderAddr)
			{
				continue;
			}

			m_ProgramHeader.SwVersion = 0;
			unsigned char swVerOffset = (unsigned char)(offsetof(PROG_HEADER_T, SwVersion) * 2u) + 4u + (addrSize * 2u);
			for (unsigned int i = 0; i < 8u; i += 2u)
			{

				m_ProgramHeader.SwVersion <<= 8u;
				m_ProgramHeader.SwVersion |= ((ctoh(sSrecLine[swVerOffset + i]) << 4u) | ctoh(sSrecLine[swVerOffset + 1u + i]));
			}

			m_ProgramHeader.HeaderCRC = 0xFFFFFFFFu;
			m_ProgramHeader.HeaderCRC = CRC_ComputeCRC((unsigned char *)&m_ProgramHeader, 12u, m_ProgramHeader.HeaderCRC);
			unsigned char chksum = (unsigned char)((   0x15u
													+ UPPERBYTE(UPPERWORD(recAddress))
													+ LOWERBYTE(UPPERWORD(recAddress))
													+ UPPERBYTE(LOWERWORD(recAddress))
													+ LOWERBYTE(LOWERWORD(recAddress))
													+ UPPERBYTE(UPPERWORD(m_ProgramHeader.ProgramCRC))
													+ LOWERBYTE(UPPERWORD(m_ProgramHeader.ProgramCRC))
													+ UPPERBYTE(LOWERWORD(m_ProgramHeader.ProgramCRC))
													+ LOWERBYTE(LOWERWORD(m_ProgramHeader.ProgramCRC))
													+ UPPERBYTE(UPPERWORD(m_ProgramHeader.ProgramLength))
													+ LOWERBYTE(UPPERWORD(m_ProgramHeader.ProgramLength))
													+ UPPERBYTE(LOWERWORD(m_ProgramHeader.ProgramLength))
													+ LOWERBYTE(LOWERWORD(m_ProgramHeader.ProgramLength))
													+ UPPERBYTE(UPPERWORD(m_ProgramHeader.SwVersion))
													+ LOWERBYTE(UPPERWORD(m_ProgramHeader.SwVersion))
													+ UPPERBYTE(LOWERWORD(m_ProgramHeader.SwVersion))
													+ LOWERBYTE(LOWERWORD(m_ProgramHeader.SwVersion))
													+ UPPERBYTE(UPPERWORD(m_ProgramHeader.HeaderCRC))
													+ LOWERBYTE(UPPERWORD(m_ProgramHeader.HeaderCRC))
													+ UPPERBYTE(LOWERWORD(m_ProgramHeader.HeaderCRC))
													+ LOWERBYTE(LOWERWORD(m_ProgramHeader.HeaderCRC))
													) & 0x00FF);
			chksum = (unsigned char)0xFF - chksum;

			sprintf_s(sHeaderLine, SREC_MAXCOUNT, "S315%08lX%08lX%08lX%08lX%08lX%02X\r\n",
					  recAddress, _byteswap_ulong(m_ProgramHeader.ProgramLength), _byteswap_ulong(m_ProgramHeader.ProgramCRC), _byteswap_ulong(m_ProgramHeader.SwVersion), _byteswap_ulong(m_ProgramHeader.HeaderCRC), chksum);

			m_fsSRecord->seekg(last_line);
			m_fsSRecord->write(sHeaderLine, strlen(sHeaderLine) - 1u);

			if (!m_fsSRecord->good())
			{
				return ERROR_INTERNAL_ERROR;
			}

			break;
		}
    }
    else
    {
        return ERROR_INTERNAL_ERROR;
    }

    return ERROR_OK;
}

UWord16 SRecord::SRec_Decode(void)
{
    bool          bS0Rec = false;
    bool          bS9Rec = false;
    char          cType = 0;
    char          cChar = 0;
    unsigned char byAddrSize = 2;
    unsigned char byOddBytes = 0;
    unsigned char byByte0, byByte1, byByte2, byByte3;
    UWord16       ii;

    m_wError = ERROR_OK;
    m_byLineChkSum = 0;
	m_wTotalElements = 0;

	if (m_fsSRecord == NULL)
	{
		printf("Failed to create file stream\n");
		return ERROR_CANNOT_OPEN_SRECORD;
	}

	int filSize;
	filSize = (int)m_fsSRecord->tellg();
	m_fsSRecord->seekg(0, ios::end);
	filSize = ((int)m_fsSRecord->tellg()) - filSize;
	m_fsSRecord->seekg(0, ios::beg);

    while (0 == m_wError)
    {
        // Read in 1 char after 'S' to find type.  1 -- (addr is 2 bytes),
        // 2 -- (addr is 3 bytes), 3 -- (addr is 4 bytes).
        bS9Rec = false;
        bS0Rec = false;
        m_byLineChkSum = 0;
        cChar = 0;
		byOddBytes = 0;

        while(!m_wError && (cChar != 'S'))
        {
            cChar = SRec_NextChar();
        }

        if(!m_wError)
        {
            cType = SRec_NextChar();
        }

        // Decode the address field width from the record type.
        if(!m_wError)
        {
            switch(cType)
            {
            case '0':
                byAddrSize = 2;
                bS0Rec = true;
                break;

            case '1':
            case '2':
            case '3':
                byAddrSize = 1 + (BYTE)cType - '0';
                break;

            case '7':
            case '8':
            case '9':
                byAddrSize = 11 - ((BYTE)cType - '0');
                bS9Rec = true;
                break;

            default:
                m_wError = ERROR_SREC_FORMAT;
                break;
            }
        }

        // Read next 2 characters and convert them to hex for record length.
        // Subract off checkbyte and address count.  m_pSRecDecoded.wNumElements will 
        // then equal the number of data bytes in the S-Record.
        if(!m_wError)
        {

            m_pSRecDecoded.wNumElements  = SRec_NextDecodedByte();
            m_pSRecDecoded.wNumElements -= (byAddrSize + 1);

            // Right now, m_pSRecDecoded.wNumElements equals the number of data bytes in 
            // the S-Record...if there's 8 bits to an address, we're good to go. 
            // If there's 16 data bits to an address, the count should be 1/2.
            // Similarly, if there's 32 data bits to an address, the count should be 1/4.
            //
            if(m_wDataWidth == DATA_WIDTH_16_BITS)
            {
                // If the number of bytes is *odd*, we have a special case.  We'll want
                // to read in INT(wNumElements/2) words and then one additional byte.
                if(m_pSRecDecoded.wNumElements & 0x01)
                {
                    byOddBytes = 1;
                }

                m_pSRecDecoded.wNumElements /= 2;
            }
            else if(m_wDataWidth == DATA_WIDTH_32_BITS)
            {
                // If the number of bytes is *odd*, we have a special case.  We'll want
                // to read in INT(wNumElements/4) dwords and then the additional bytes.
                if(m_pSRecDecoded.wNumElements & 0x03)
                {
                    byOddBytes = m_pSRecDecoded.wNumElements % 4;
                }

                m_pSRecDecoded.wNumElements /= 4;
            }

						// No need to check to see if the length is small enough for us.  An
            // S-Record's length byte is only 8-bits...thus 256 bytes is max...
            // Our data array is a union of 256 bytes, 128 words, and 64 dwords.

            // Read next (wAddrSize) bytes and rotate to become hex.
            m_pSRecDecoded.dwAddress = 0;
            for(ii = 0; !m_wError && (ii < byAddrSize); ii++)
            {
                m_pSRecDecoded.dwAddress <<= 8;
                m_pSRecDecoded.dwAddress |= SRec_NextDecodedByte();
            }

        }

				// Get data into m_pSRecDecoded.Data. Two ascii characters convert to a single
        // byte for each array element.  
        for(ii = 0; !m_wError && (ii < m_pSRecDecoded.wNumElements); ii++)
        {
            if(m_wDataWidth == DATA_WIDTH_8_BITS)
            {
                // Store next byte.
                m_pSRecDecoded.Data.by[ii] = SRec_NextDecodedByte();
            }
            else if(m_wDataWidth == DATA_WIDTH_16_BITS)
            {
                // The data is stored low-byte, high-byte...
                byByte0 = SRec_NextDecodedByte();
                byByte1 = SRec_NextDecodedByte();
                m_pSRecDecoded.Data.w[ii] = MAKEUWORD(byByte1, byByte0);
            }
            else if(m_wDataWidth == DATA_WIDTH_32_BITS)
            {
                // The data is stored low-byte, high-byte...
                byByte0 = SRec_NextDecodedByte();
                byByte1 = SRec_NextDecodedByte();
                byByte2 = SRec_NextDecodedByte();
                byByte3 = SRec_NextDecodedByte();
                m_pSRecDecoded.Data.dw[ii] = MAKEULONG(MAKEUWORD(byByte3, byByte2), MAKEUWORD(byByte1, byByte0));
            }
        }

        // If we had an odd number of bytes, we'll read in the last one(s).  One would
        // assume that the next byte (the high byte) should be 0xFF....but comparing
        // with what the debugger does, it's a x00....  Lastly, we'll also want to 
        // incrase the SREC_t by one word then as well.
        if(byOddBytes)
        {
            m_pSRecDecoded.wNumElements++;

            if(m_wDataWidth == DATA_WIDTH_16_BITS)
            {
                m_pSRecDecoded.Data.w[ii] = SRec_NextDecodedByte();
            }
            else if(m_wDataWidth == DATA_WIDTH_32_BITS)
            {
                byByte0 = (byOddBytes > 0) ? SRec_NextDecodedByte() : 0;
                byByte1 = (byOddBytes > 1) ? SRec_NextDecodedByte() : 0;
                byByte2 = (byOddBytes > 2) ? SRec_NextDecodedByte() : 0;
                byByte3 = (byOddBytes > 3) ? SRec_NextDecodedByte() : 0;
                m_pSRecDecoded.Data.dw[ii] = MAKEULONG(MAKEUWORD(byByte3, byByte2), MAKEUWORD(byByte1, byByte0));
            }
        }

        // Add check byte into linewise LineChkSum to verify line.
        (void)SRec_NextDecodedByte();
        if(!m_wError && (m_byLineChkSum != 0xFF))
        {
            m_wError = ERROR_SREC_CHECKSUM;
        }

        // If it was the last record, send back a flag.
        if(!m_wError && bS9Rec)
        {
            m_wError = ERROR_SREC_FLAG_LASTREC;
        }
        if(!m_wError && bS0Rec)
        {
            m_wError = ERROR_SREC_FLAG_ZEROREC;
        }

        if (ERROR_OK == m_wError)
        {
			m_wTotalElements += m_pSRecDecoded.wNumElements;
            PlaceBytes(&m_pSRecDecoded);
        }
        else if (ERROR_SREC_FLAG_ZEROREC == m_wError)
        {
            // not an error
            m_wError = ERROR_OK;
        }
    }

    if (ERROR_SREC_FLAG_LASTREC == m_wError)
    {
        // This is the right place to be
        m_wError = ERROR_OK;
    }

    return(m_wError);
}

unsigned char SRecord::SRec_NextDecodedByte(void)
{
    char cChar = '0';
    unsigned char byData = 0;

    if(!m_wError)
    {
        // Get the first character from the stream.
        cChar = SRec_NextChar();
        
        // Change it to hex and shift it to the top nibble.
        byData = ctoh(cChar);
        byData <<= 4;

        // Get the second character from the stream.
        cChar = SRec_NextChar();

        // Mask it in.
        byData |= ctoh(cChar);

        // Add the new byte's value to the running line checksum.
        m_byLineChkSum += byData;
    }

    return(byData);
}


char SRecord::SRec_NextChar(void)
{
    char cChar = 0;
    bool bUseful = false;

    if(!m_wError)
    {
        // Start pulling out characters until we find a useful one.
        while(!bUseful)
        {
            // Next.
            *m_fsSRecord >> cChar;
            
            // Check to see that  it's not a linefeed or space.  If it is,
            // ignore it and go get another.
            if (cChar == 0x0A) continue;
            if (cChar == 0x0D) continue;
            if (cChar == 0x20) continue;
            
            // It's useful.
            bUseful = true;
        }

        // If it's a null, we've reached the end of the record.
        if(!cChar)
        {
			m_wError = ERROR_SREC_EOR;
        }
    }

    return(cChar);
}

unsigned char SRecord::ctoh(char cChar)
{
    if((cChar >= '0') && (cChar <= '9'))
    {
        cChar -= '0';
    }
    else if((cChar >= 'A') && (cChar <= 'F'))
    {
        cChar -= ('A' - 10);
    }
    else if((cChar >= 'a') && (cChar <= 'f'))
    {
        cChar -= ('a' - 10);
    }
    else
    {
        // Must be an illegal value.
        m_wError = ERROR_SREC_FORMAT;
    }
    
    return((unsigned char)cChar);
}

void SRecord::PlaceBytes(const SREC_t *pSRec)
{
    UWord32 dwAddr;
    int ii;
		unsigned int index;

    // Loop thru all the bytes.
    for(ii = 0, dwAddr = pSRec->dwAddress; ii < pSRec->wNumElements; ii++, dwAddr+=sizeof(unsigned int))
    {
        if((dwAddr >= m_wSegStartAddress) && (dwAddr <= m_wSegEndAddress))
        {
			index = (dwAddr - m_wSegStartAddress) / (sizeof(unsigned int));
            if (DATA_WIDTH_8_BITS == m_wDataWidth)
            {
                m_pMemorySegment[index] = (unsigned int)pSRec->Data.by[ii];
            }
            else if (DATA_WIDTH_16_BITS == m_wDataWidth)
            {
                m_pMemorySegment[index] = (unsigned int)pSRec->Data.w[ii];
            }
            else if (DATA_WIDTH_32_BITS == m_wDataWidth)
            {
                m_pMemorySegment[index] = (unsigned int)pSRec->Data.dw[ii];
            }
        }
    }
}

