#include <stdlib.h>
#include <string>
#include <tchar.h>
#include "typedefs.h"
#include "stdafx.h"
#include "SRec.h"

#define ARGC_SRECORD                            (1)
#define ARGC_DATA_WIDTH                         (2)
#define ARGC_START_ADDR                         (3)
#define ARGC_END_ADDR                           (4)
#define ARGC_HEADER_ADDR                        (5)

#define MAX_FILE_NAME_LENGTH                    (260)   // Max Windows path length

int _tmain(int argc, _TCHAR* argv[])
{
    // Calling format:
    // <command> <S-Record path> <data width> <start addess> <end address> <header address>
    int             data_width = 0;
	unsigned int    start_addr = 0;
	unsigned int    end_addr = 0;
	unsigned int    header_addr = 0;
    char            filename[MAX_FILE_NAME_LENGTH];
    bool            error = false;
    SRecord         *sRecordIO = NULL;

    // If there aren't enough arguments to get the segment count - error out
    if (argc <= ARGC_HEADER_ADDR)
    {
        return ERROR_TOO_FEW_ARGUMENTS;
    }

    int filename_len = (int)_tcslen(argv[ARGC_SRECORD]);
    if (MAX_FILE_NAME_LENGTH <= filename_len)
    {
        return ERROR_INVALID_SRECORD;
    }

    // Get the first two arguments
    (void)strcpy_s(filename, argv[ARGC_SRECORD]);
	//printf("File name: %s\n", filename);
    data_width = atoi((char *)argv[ARGC_DATA_WIDTH]);
	//printf("Data width: %d\n", data_width);

    // Check that the data_width is valid
    if (data_width != DATA_WIDTH_8_BITS && data_width != DATA_WIDTH_16_BITS && data_width != DATA_WIDTH_32_BITS)
    {
        return ERROR_INVALID_ARG;
    }

		// Get addresses
		sscanf_s((char *)argv[ARGC_START_ADDR], "%x", &start_addr);
		sscanf_s((char *)argv[ARGC_END_ADDR], "%x", &end_addr);
		sscanf_s((char *)argv[ARGC_HEADER_ADDR], "%x", &header_addr);
		//printf("Start addr: %08X, End addr: %08X, Header addr: %08X\n", start_addr, end_addr, header_addr);

		if (end_addr < start_addr)
		{
			printf("Start address cannot be larger than end address\n");
			error = true;
		}

		if (header_addr < end_addr)
		{
			printf("Header start address cannot be larger than image end address\n");
			error = true;
		}

    if (false == error)
    {
        unsigned int calculated_crc = 0;
		unsigned int calculated_size = 0;
        
        // Decode the hex
        try
        {
            sRecordIO = new SRecord(filename, data_width, start_addr, end_addr, header_addr);
        }
        catch (exception exp)
        {
            printf_s("Exception occured: %s\n", exp.what());
        }

        if (NULL != sRecordIO)
        {
            if (ERROR_OK != sRecordIO->SRec_Decode())
            {
                printf("Internal Error: Failed to decode s-record\n");
                error = true;
            }
            else if (ERROR_OK != sRecordIO->CalculateCRC(&calculated_crc, &calculated_size))
            {
                printf("Internal Error: Failed to calculate CRC & size\n");
                error = true;
            }
            else
            {
				printf("Size Calculated: %00000000X. Inserted at %000000X\n", calculated_size, header_addr);
				printf("CRC Calculated: %00000000X. Inserted at %000000X\n", calculated_crc, header_addr + 4u);
            }
        }
    }

    if (NULL != sRecordIO)
    {
        delete sRecordIO;
    }

    if (true == error)
    {
        return ERROR_INTERNAL_ERROR;
    }

    return ERROR_OK;
}

