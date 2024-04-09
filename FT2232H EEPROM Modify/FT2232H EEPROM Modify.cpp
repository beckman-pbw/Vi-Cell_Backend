// FT2232H EEPROM Modify.cpp : Defines the entry point for the console application.
//

// NOTE:	This code is provided as an example only and is not supported or guaranteed by FTDI.
//			It is the responsibility of the recipient/user to ensure the correct operation of 
//			any software which is created based upon this example.

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include "ftd2xx.h"
#include <string>

int main(int argc, char* argv[])
{
	/*
	 Calling convention: PROGRAMMER <SerialNumber> <Version (defaults to "3" if not specified>
	 Will program the board to:
		VID: 0x0403
		PID: 0x6010
		MFR: BeckmanCoulter
		DES: Hawkeye V<version>
		SER: <serial number>

		Port A:
			RS232 UART
			D2XX Direct
		Port B:
			RS232 UART
			D2XX Direct

		IO Pins
			Slow Slew: 0
			Drive: 4mA
			Schmitt: 0
	 */


	//********************************************************
	//Definitions
	//********************************************************

	FT_HANDLE fthandle;
	FT_STATUS status;
	
	BOOLEAN Dev_Found = FALSE;
	bool test_mode = false;

	
	char ManufacturerBuf[32];
	char ManufacturerIdBuf[16];
	char DescriptionBuf[64];
	char SerialNumberBuf[16];

	char NewManufacturerBuf[32];
	char NewDescriptionBuf[64];
	char NewSerialNumberBuf[16];
	char NewManufacturerIdBuf[16];

	if (argc == 1)
	{
		std::cout << "Usage: <thisprogram> [SerialNumber] [HWVersion] [TEST]" << std::endl;
		std::cout << "Example: \"<thisprogram> BEC1235 3\" programs with" << std::endl;
		std::cout << "\t\tSerial Number: BEC1235" << std::endl;
		std::cout << "\t\tHardware Ver : 3" << std::endl;
		return 0;
	}
	if (argc >= 2)
	{
		strcpy_s(NewSerialNumberBuf, 16, argv[1]);
	}
	if (argc >= 3)
	{
		std::string HWRevision = "Hawkeye V";
		HWRevision += argv[2];
		strcpy_s(NewDescriptionBuf, 64, HWRevision.c_str());
	}
	if (argc == 4)
	{
		test_mode = true;
	}
	else
	{
		std::string HWRevision = "Hawkeye V3";
		strcpy_s(NewDescriptionBuf, 64, HWRevision.c_str());
	}

	strcpy_s(NewManufacturerBuf, 32, "BeckmanCoulter");
	strcpy_s(NewManufacturerIdBuf, 16, "XX");// "BC");



	//********************************************************
	//List Devices
	//********************************************************

	FT_DEVICE_LIST_INFO_NODE *devInfo;
	DWORD numDevs;
	DWORD i;
	ULONG dev_0_ID = 0;
	// create the device information list 
	status = FT_CreateDeviceInfoList(&numDevs);

	if (status != FT_OK) {
		printf("ERROR: unable to get device list (FT_CreateDeviceInfoList status not ok %d\n)", status);
		getchar();
		return -1;
	}
	else
	{
		printf("Number of devices is %d\n", numDevs);
		if (numDevs > 0) {
			// allocate storage for list based on numDevs 
			devInfo =
				(FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*numDevs);
			// get the device information list 
			status = FT_GetDeviceInfoList(devInfo, &numDevs);
			if (status == FT_OK) {
				for (i = 0; i < numDevs; i++) {
					printf("Dev %d:\n", i);
					printf("Flags=0x%x\n", devInfo[i].Flags);
					printf("Type=0x%x\n", devInfo[i].Type);
					printf("ID=0x%x\n", devInfo[i].ID);
					printf("LocId=0x%x\n", devInfo[i].LocId);
					printf("SerialNumber=%s\n", devInfo[i].SerialNumber);
					printf("Description=%s\n", devInfo[i].Description);
					printf("\n");
				}
			}
			if (numDevs != 2)
			{
				std::cout << "ERROR: expected to see only 2 devices (Ports A and B)" << std::endl;
				getchar();
				free(devInfo);
				return -1;
			}

			if (devInfo[0].Type != FT_DEVICE_2232H)
			{
				std::cout << "ERROR: device does not identify as an FT_2232H part" << std::endl;
				getchar();
				free(devInfo);
				return -1;
			}


			if (test_mode)
			{
				// Ensure that we see two devices with description
				// and serial number as specified in the other parameters

				// 'A'
				bool A_found = false;
				bool B_found = false;
				for (i = 0; i < numDevs; i++)
				{
					std::string desc_dev = devInfo[i].Description;
					std::string desc_A = NewDescriptionBuf;
					desc_A += " A";
					
					if (desc_dev != desc_A)
						continue;
					std::string ser_dev = devInfo[i].SerialNumber;
					std::string ser_A = NewSerialNumberBuf;
					ser_A += "A";
					if (ser_dev != ser_A)
						continue;

					A_found = true;
					FT_HANDLE A_HANDLE;
					FT_STATUS fstat = FT_OpenEx((PVOID)desc_A.c_str(), FT_OPEN_BY_DESCRIPTION, &A_HANDLE);

					if (fstat != FT_OK)
						std::cout << "Failed to open device \"" << desc_A << "\"" << std::endl;
					FT_Close(A_HANDLE);

					break;
				}
				for (i = 0; i < numDevs; i++)
				{
					std::string desc_dev = devInfo[i].Description;
					std::string desc_B = NewDescriptionBuf;
					desc_B += " B";

					if (desc_dev != desc_B)
						continue;
					std::string ser_dev = devInfo[i].SerialNumber;
					std::string ser_B = NewSerialNumberBuf;
					ser_B += "B";
					if (ser_dev != ser_B)
						continue;

					B_found = true;
					FT_HANDLE B_HANDLE;
					FT_STATUS fstat = FT_OpenEx((PVOID)desc_B.c_str(), FT_OPEN_BY_DESCRIPTION, &B_HANDLE);

					if (fstat != FT_OK)
						std::cout << "Failed to open device \"" << desc_B << "\"" << std::endl;
					FT_Close(B_HANDLE);
					break;
				}

			
				if (!A_found || !B_found)
				{
					std::cout << "FAIL Devices matching description / serial numbers were not found!" << std::endl;
					free(devInfo);
					return -1;
				}
				else
				{
					std::cout << "PASS Devices matching description / serial numbers were found." << std::endl;
					free(devInfo);
					return 0;
				}
			}


			dev_0_ID = devInfo[0].ID;
			free(devInfo);
		}
	}

	

	//********************************************************
	//Open the port for the first device
	//********************************************************

	status = FT_Open(0, &fthandle);
	if (status != FT_OK)
	{
		printf("Error: Unable to open FTDI device 0 (Open status not ok %d)\n", status);
		printf("\n");
		getchar();
		return -1;
		
	}
	else
	{
		printf("Open status OK %d\n", status);
		printf("\n");
	}

	//********************************************************
	// Read the inital EEPROM data
	//********************************************************

	FT_PROGRAM_DATA ftData;
	ftData.Signature1 = 0x00000000;		// Always 0x00000000
	ftData.Signature2 = 0xffffffff;		// Always 0xFFFFFFFF
	ftData.Version = 3;	// Header - FT_PROGRAM_DATA version 0 = original (FT232B), 1 = FT2232 extensions, 2 = FT232R extensions, 3 = FT2232H extensions, 4 = FT4232H extensions, 5 = FT232H extensions

//	ftData.VendorId = VendorIdBuf;
//	ftData.ProductId = ProductIdBuf;
	ftData.Manufacturer = ManufacturerBuf;
	ftData.ManufacturerId = ManufacturerIdBuf;
	ftData.Description = DescriptionBuf;
	ftData.SerialNumber = SerialNumberBuf;

	status = FT_EE_Read(fthandle, &ftData);

	if (status != FT_OK)
	{
		printf("ERROR: Failed to read back EEPROM data (EE_Read status not ok %d)\n", status);
		printf("       Probably indicates a blank device.\n");

		// Set our initial vendor ID / PID
		// We'd have preferred to get this back from the device, but...

		ftData.VendorId = (dev_0_ID >> 16) & 0xFFFF;
		ftData.ProductId = (dev_0_ID >> 00) & 0xFFFF;
	}
	else
	{
		printf("Signature1 =  0x%04x\n", ftData.Signature1);
		printf("Signature2 =  0x%04x\n", ftData.Signature2);
		printf("Version =  0x%04x\n", ftData.Version);

		printf("VendorID =  0x%04x\n", ftData.VendorId);
		printf("ProductID =  0x%04x\n", ftData.ProductId);
		printf("Manufacturer =  %s\n", ftData.Manufacturer);
		printf("ManufacturerID =  %s\n", ftData.ManufacturerId);
		printf("Description =  %s\n", ftData.Description);
		printf("SerialNumber =  %s\n", ftData.SerialNumber);
		printf("MaxPower =  %d\n", ftData.MaxPower);
		printf("PnP =  %x\n", ftData.PnP);
		printf("SelfPowered =  %x\n", ftData.SelfPowered);
		printf("RemoteWakeup =  %x\n", ftData.RemoteWakeup);

		printf("PullDownEnable7 =  %x\n", ftData.PullDownEnable7);
		printf("SerNumEnable7 =  %x\n", ftData.SerNumEnable7);
		printf("ALSlowSlew =  %x\n", ftData.ALSlowSlew);
		printf("ALSchmittInput =  %x\n", ftData.ALSchmittInput);
		printf("ALDriveCurrent =  %x\n", ftData.ALDriveCurrent);
		printf("AHSlowSlew =  %x\n", ftData.AHSlowSlew);
		printf("AHSchmittInput =  %x\n", ftData.AHSchmittInput);
		printf("AHDriveCurrent =  %x\n", ftData.AHDriveCurrent);
		printf("BLSlowSlew =  %x\n", ftData.BLSlowSlew);
		printf("BLSchmittInput =  %x\n", ftData.BLSchmittInput);
		printf("BLDriveCurrent =  %x\n", ftData.BLDriveCurrent);
		printf("BHSlowSlew =  %x\n", ftData.BHSlowSlew);
		printf("BHSchmittInput =  %x\n", ftData.BHSchmittInput);
		printf("BHDriveCurrent =  %x\n", ftData.BHDriveCurrent);
		printf("IFAIsFifo7 =  %x\n", ftData.IFAIsFifo7);
		printf("IFAIsFifoTar7 =  %x\n", ftData.IFAIsFifoTar7);
		printf("IFAIsFastSer7 =  %x\n", ftData.IFAIsFastSer7);
		printf("AIsVCP7 =  %x\n", ftData.AIsVCP7);
		printf("IFBIsFifo7 =  %x\n", ftData.IFBIsFifo7);
		printf("IFBIsFifoTar7 =  %x\n", ftData.IFBIsFifoTar7);
		printf("IFBIsFastSer7 =  %x\n", ftData.IFBIsFastSer7);
		printf("BIsVCP7 =  %x\n", ftData.BIsVCP7);
		printf("PowerSaveEnable =  %x\n", ftData.PowerSaveEnable);
		printf("\n");
	}


	//********************************************************
	//Write the EEPROM
	//********************************************************

	
	ftData.Signature1 = 0x00000000;		// Always 0x00000000
	ftData.Signature2 = 0xffffffff;		// Always 0xFFFFFFFF
	ftData.Version = 3;	// Header - FT_PROGRAM_DATA version 0 = original (FT232B), 1 = FT2232 extensions, 2 = FT232R extensions, 3 = FT2232H extensions, 4 = FT4232H extensions, 5 = FT232H extensions

//	ftData.VendorId = VendorIdBuf;
//	ftData.ProductId = ProductIdBuf;
	ftData.Manufacturer = NewManufacturerBuf;
	ftData.ManufacturerId = NewManufacturerIdBuf;
	ftData.Description = NewDescriptionBuf;
	ftData.SerialNumber = NewSerialNumberBuf;

	ftData.MaxPower      = 100;
	ftData.PnP           = 1;
	ftData.SelfPowered   = 0;
	ftData.RemoteWakeup  = 0;

	////'FT2232H features require section below
	ftData.PullDownEnable7 = 0;		// non-zero if pull down enabled 
	ftData.SerNumEnable7   = 1;		// non-zero if serial number to be used 
	ftData.ALSlowSlew      = 0;		// non-zero if AL pins have slow slew 
	ftData.ALSchmittInput  = 0;		// non-zero if AL pins are Schmitt input 
	ftData.ALDriveCurrent  = 4;		// valid values are 4mA, 8mA, 12mA, 16mA 
	ftData.AHSlowSlew      = 0;		// non-zero if AH pins have slow slew 
	ftData.AHSchmittInput  = 0;		// non-zero if AH pins are Schmitt input 
	ftData.AHDriveCurrent  = 4;		// valid values are 4mA, 8mA, 12mA, 16mA 
	ftData.BLSlowSlew      = 0;		// non-zero if BL pins have slow slew 
	ftData.BLSchmittInput  = 0;		// non-zero if BL pins are Schmitt input 
	ftData.BLDriveCurrent  = 4;		// valid values are 4mA, 8mA, 12mA, 16mA 
	ftData.BHSlowSlew      = 0;		// non-zero if BH pins have slow slew 
	ftData.BHSchmittInput  = 0;		// non-zero if BH pins are Schmitt input 
	ftData.BHDriveCurrent  = 4;		// valid values are 4mA, 8mA, 12mA, 16mA 
	ftData.IFAIsFifo7      = 0;		// non-zero if interface is 245 FIFO 
	ftData.IFAIsFifoTar7   = 0;		// non-zero if interface is 245 FIFO CPU target 
	ftData.IFAIsFastSer7   = 0;		// non-zero if interface is Fast serial 
	ftData.AIsVCP7         = 0;		// non-zero if interface is to use VCP drivers 
	ftData.IFBIsFifo7      = 0;		// non-zero if interface is 245 FIFO 
	ftData.IFBIsFifoTar7   = 0;		// non-zero if interface is 245 FIFO CPU target 
	ftData.IFBIsFastSer7   = 0;		// non-zero if interface is Fast serial 
	ftData.BIsVCP7         = 0;		// non-zero if interface is to use VCP drivers 
	ftData.PowerSaveEnable = 0;		// non-zero if using BCBUS7 to save power for self-powered


	status = FT_EE_Program(fthandle, &ftData);
	if (status != FT_OK)
	{
		printf("ERROR: Failed to write EEPROM data (EE_Program status not ok %d)\n", status);
		getchar();
		return -1;
	}
	else
	{
		std::cout << "EEPROM data written successfully." << std::endl;
	}


	//********************************************************
	// Delay
	//********************************************************

	Sleep(1000);


	//********************************************************
	// Re - Read
	//********************************************************
	std::cout << "Reading EEPROM to check changed values..." << std::endl;

	status = FT_EE_Read(fthandle, &ftData);

	if (status != FT_OK)
	{
		printf("ERROR: Failed to read back EEPROM data (EE_Read status not ok %d)\n", status);
		getchar();
		return -1;
	}
	else
	{
		printf("Signature1 =  0x%04x\n", ftData.Signature1);
		printf("Signature2 =  0x%04x\n", ftData.Signature2);
		printf("Version =  0x%04x\n", ftData.Version);

		printf("VendorID =  0x%04x\n", ftData.VendorId);
		printf("ProductID =  0x%04x\n", ftData.ProductId);
		printf("Manufacturer =  %s\n", ftData.Manufacturer);
		printf("ManufacturerID =  %s\n", ftData.ManufacturerId);
		printf("Description =  %s\n", ftData.Description);
		printf("SerialNumber =  %s\n", ftData.SerialNumber);
		printf("MaxPower =  %d\n", ftData.MaxPower);
		printf("PnP =  %x\n", ftData.PnP);
		printf("SelfPowered =  %x\n", ftData.SelfPowered);
		printf("RemoteWakeup =  %x\n", ftData.RemoteWakeup);

		printf("PullDownEnable7 =  %x\n", ftData.PullDownEnable7);
		printf("SerNumEnable7 =  %x\n", ftData.SerNumEnable7);
		printf("ALSlowSlew =  %x\n", ftData.ALSlowSlew);
		printf("ALSchmittInput =  %x\n", ftData.ALSchmittInput);
		printf("ALDriveCurrent =  %x\n", ftData.ALDriveCurrent);
		printf("AHSlowSlew =  %x\n", ftData.AHSlowSlew);
		printf("AHSchmittInput =  %x\n", ftData.AHSchmittInput);
		printf("AHDriveCurrent =  %x\n", ftData.AHDriveCurrent);
		printf("BLSlowSlew =  %x\n", ftData.BLSlowSlew);
		printf("BLSchmittInput =  %x\n", ftData.BLSchmittInput);
		printf("BLDriveCurrent =  %x\n", ftData.BLDriveCurrent);
		printf("BHSlowSlew =  %x\n", ftData.BHSlowSlew);
		printf("BHSchmittInput =  %x\n", ftData.BHSchmittInput);
		printf("BHDriveCurrent =  %x\n", ftData.BHDriveCurrent);
		printf("IFAIsFifo7 =  %x\n", ftData.IFAIsFifo7);
		printf("IFAIsFifoTar7 =  %x\n", ftData.IFAIsFifoTar7);
		printf("IFAIsFastSer7 =  %x\n", ftData.IFAIsFastSer7);
		printf("AIsVCP7 =  %x\n", ftData.AIsVCP7);
		printf("IFBIsFifo7 =  %x\n", ftData.IFBIsFifo7);
		printf("IFBIsFifoTar7 =  %x\n", ftData.IFBIsFifoTar7);
		printf("IFBIsFastSer7 =  %x\n", ftData.IFBIsFastSer7);
		printf("BIsVCP7 =  %x\n", ftData.BIsVCP7);
		printf("PowerSaveEnable =  %x\n", ftData.PowerSaveEnable);
		printf("\n");
	}

	//*****************************************************
	//Recycle the port
	//*****************************************************

	std::cout << "Resetting the FTDI device..." << std::endl;

	// Close the device
//	status = FT_CyclePort(fthandle);
	status = FT_Close(fthandle);

	status = FT_Rescan();
	Sleep(1000);

	//*****************************************************
	//Reload the device list for confirmations
	//*****************************************************
	// create the device information list 
	status = FT_CreateDeviceInfoList(&numDevs);

	if (status != FT_OK)
	{
		printf("ERROR: unable to get device list (FT_CreateDeviceInfoList status not ok %d\n)", status);
		getchar();
		return -1;
	}
	else
	{
		printf("Number of devices is %d\n", numDevs);
		if (numDevs > 0)
		{
			// allocate storage for list based on numDevs 
			devInfo =
				(FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*numDevs);
			// get the device information list 
			status = FT_GetDeviceInfoList(devInfo, &numDevs);
			if (status == FT_OK)
			{
				for (i = 0; i < numDevs; i++)
				{
					printf("Dev %d:\n", i);
					printf("Flags=0x%x\n", devInfo[i].Flags);
					printf("Type=0x%x\n", devInfo[i].Type);
					printf("ID=0x%x\n", devInfo[i].ID);
					printf("LocId=0x%x\n", devInfo[i].LocId);
					printf("SerialNumber=%s\n", devInfo[i].SerialNumber);
					printf("Description=%s\n", devInfo[i].Description);
					printf("\n");
				}
			}
			if (numDevs != 2)
			{
				std::cout << "ERROR: expected to see only 2 devices (Ports A and B)" << std::endl;
				getchar();
				free(devInfo);
				return -1;
			}

			if (devInfo[0].Type != FT_DEVICE_2232H)
			{
				std::cout << "ERROR: device does not identify as an FT_2232H part" << std::endl;
				getchar();
				free(devInfo);
				return -1;
			}

			free(devInfo);
		}
	}

	printf("Press Return To End Program");
	getchar();
	printf("closed \n");

	return 0;
}

