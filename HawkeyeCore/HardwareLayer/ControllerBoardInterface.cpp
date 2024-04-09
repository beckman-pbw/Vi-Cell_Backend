#include "stdafx.h"

#include <iostream>
#include <ftd2xx.h>
#include "inttypes.h"

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "ControllerBoardInterface.hpp"
#include "ErrcHelpers.hpp"
#include "FTDI_Error_To_Boost.hpp"
#include "Logger.hpp"

static const char MODULENAME[] = "ControllerBoardInterface";

ControllerBoardInterface::ControllerBoardInterface (std::shared_ptr<boost::asio::io_context> upstream,
	                                                const std::string& _serialDevIdentifier,
	                                                const std::string& _dataDevIdentifier)
	:_pUpstream(upstream),
	deviceIDSerial_(_serialDevIdentifier), deviceIsOpen_Serial_(false),
	deviceIDGPIO_(_dataDevIdentifier), deviceIsOpen_GPIO_(false),
	deviceSerialNumber_(""), taskindex(0),
	firmwareVersion_("00.00.00.00"),
	errorStatus_(0),
	ez_port_(nullptr),
	hardware_level_(0),
	isReleasedVersion_(true),
	task_transaction_log(nullptr)
{
	boost::trim(deviceIDSerial_);
	boost::trim(deviceIDGPIO_);

	if (Logger::L().IsOfInterest(severity_level::debug3))
		fopen_s(&task_transaction_log, "TaskTransactionLog.csv", "wt");

	if (task_transaction_log)
	{
		std::string ttl = "Task ID,RW,Address,Enqueueued,ProcessingStart,ProcessingComplete,CallbackEnqueue,CallbackStart,CallbackComplete\n";
		fputs(ttl.c_str(), task_transaction_log);
	}

	hasOmicronLed_ = false;
}

ControllerBoardInterface::~ControllerBoardInterface()
{
	Close();
	
	if (task_transaction_log)
		fclose(task_transaction_log);
}

//*****************************************************************************
void ControllerBoardInterface::Initialize(std::function<void(bool)> completeCallback) 
{
	DWORD numFTDIDevices;

	FT_STATUS fts = FT_CreateDeviceInfoList (&numFTDIDevices);
	if (numFTDIDevices > 0)
	{
		Logger::L().Log (MODULENAME, severity_level::normal, std::to_string(numFTDIDevices) + " FTDI devices attached");

		for (int i = 0; i < (int)numFTDIDevices; i++)
		{
			DWORD flags;
			DWORD id;
			DWORD type;
			DWORD locId;
			char serialNumber[16];
			char description[64];
			FT_HANDLE ftHandleTemp;

			// Get information for device.
			fts = FT_GetDeviceInfoDetail ((DWORD)i, &flags, &type, &id, &locId, serialNumber, description, &ftHandleTemp);

			Logger::L().Log (MODULENAME, severity_level::normal, boost::str (
				boost::format ("dev #: %d, flags: 0x%04X, type: 0x%04X, locId: 0x%04X, sn: %s, desc: %s" ) 
					% i 
					% flags 
					% type 
					% locId
					% serialNumber
					% description));
		
			if (std::string(description) == "LEDMOD") {
				hasOmicronLed_ = true;
			}

			// Check to skip devices that do not match the type of interest.
			if (type != FT_DEVICE_2232H) {
				continue;
			}

			// Get hardware level for controller board by reading the EEPROM description. 
			std::string hwki_desscrip;
			std::vector<std::string> hwki_descript_details;
			if (readEEPROM_Description(i, hwki_desscrip))	//if we failed, the following code will catch it. 
			{
//NOTE: test code...
				//hwki_desscrip = "Hawkeye V2";
				//Logger::L().Log (MODULENAME, severity_level::debug1,
				//	boost::str (boost::format ("[%d]: %s, len: %d") % i % hwki_desscrip % hwki_desscrip.length()));

				std::vector<std::string> outStrings;
				boost::algorithm::split (hwki_descript_details, hwki_desscrip, boost::algorithm::is_any_of("V"), boost::algorithm::token_compress_on);
				// The EEPROM description string should read: "Hawkeye V#" ,  
				// which `#` is a integer value for the controller board hardware level.
				if (hwki_descript_details.size() == 2 && hwki_descript_details[0] == "Hawkeye") 
				{
					uint16_t hw_level_descriptor = 0;

					/*
					 * Pre-production boards will have numeric versions.  Interpret these as straight base-10 integers.
					 * Production boards will have alpha versions ("AA", "CA"...) where the leading character is more or less the effective "important" revision
					 *   and the trailing character is the "non-functional-roll" revision.  We'll convert each character to a 1..26 value and pack them into a 
					 *   uint16 as HHLL.
					 */
					for (auto c : hwki_descript_details[1]) 
					{
						c = toupper(c);
						if (c >= '0' && c <= '9')
							hw_level_descriptor = (hw_level_descriptor * 10) | (c - '0');
						else if (c >= 'A' && c <= 'Z')
							hw_level_descriptor = (hw_level_descriptor << 8) | ((c - 'A') + 1);
						else
						{
							Logger::L().Log (MODULENAME, severity_level::error, boost::str(
								boost::format("Unexpected character\"%c\" (int: %d) found while parsing controller board revision \"%s\"") % c % ((int)c) % hwki_descript_details[1] ));
						}
					}

					// Convert the hw_level_descriptor to our functional hardware levels as far as the code is concerned
					if ((hw_level_descriptor >> 8) == 0x00)
						hardware_level_ = hw_level_descriptor;
					else
					{
						switch (hw_level_descriptor >> 8)
						{
							case 1:      hardware_level_ = 3; break;
							default:
							{
								Logger::L().Log (MODULENAME, severity_level::error, boost::str(
									boost::format("Unsupported hardware level %d-%d.  Defaulting to internal revision %d but success not guaranteed!") % (hw_level_descriptor >> 8) % (hw_level_descriptor & 0x00FF) % 3));

								hardware_level_ = 3;
								break;
							}
						}
					}
				} 
				else /* If no revision is specified, then revert to the compiler build switch */ 
				{
#ifdef REV2_HW
					hardware_level_ = 2;
#else
					hardware_level_ = 3;
#endif
				}
			}

		}

		std::string identifier = CNTLR_SN_A_STR;
		boost::system::error_code ec = OpenSerial (identifier);
		if (ec) 
		{
			Logger::L().Log (MODULENAME, severity_level::critical, "Unable to open controller board " + identifier + ": " + ec.message());
			// this workflow does not require the user id, so do not use the transient user technique
			_pUpstream->post(std::bind(completeCallback, false));
			return;
		}
		
#ifdef REV3_FW
        Logger::L().Log ( MODULENAME, severity_level::debug2, "GPIOs are configured for Rev3_FW" );
#else
        Logger::L().Log ( MODULENAME, severity_level::debug2, "GPIOs are configured for Rev2_FW" );
#endif
		ec = OpenGPIO();
		if (ec) 
		{
			Logger::L().Log (MODULENAME, severity_level::critical, "Unable to open GPIO on controller board: " + ec.message());
			// this workflow does not require the user id, so do not use the transient user technique
			_pUpstream->post(std::bind(completeCallback, false));
			return;
		}

	} 
	else 
	{
		Logger::L().Log (MODULENAME, severity_level::critical, "No FTDI devices found! ");
		// this workflow does not require the user id, so do not use the transient user technique
		_pUpstream->post(std::bind(completeCallback, false));
		return;
	}
	
	Logger::L().Log (MODULENAME, severity_level::debug1, "Toggling GPIOs");
	SetGPIO(true, true, false);
	::Sleep(100);
	SetGPIO(true, true, true);
	Logger::L().Log (MODULENAME, severity_level::debug2, "GPIO toggling done.");

	::Sleep(2000);

	StartLocalIOService();
	// this workflow does not require the user id, so do not use the transient user technique
	_pUpstream->post(std::bind(completeCallback, true));

	DWORD dllVersion;
	FT_GetLibraryVersion (&dllVersion);
	Logger::L().Log (MODULENAME, severity_level::normal,
					 boost::str (boost::format ("FTDI Library Version: %x.%x.%x")
								 % (int)((dllVersion & 0x00FF0000) >> 16)
								 % (int)((dllVersion & 0x0000FF00) >> 8)
								 % (int)((dllVersion & 0x000000FF))
					 ));

	return;
}

bool ControllerBoardInterface::isReleasedVersion() {

	return isReleasedVersion_;
}

boost::system::error_code ControllerBoardInterface::OpenHawkeyeDeviceByDescription(std::string identifier, FT_HANDLE& dev_handle)
{
	/*
	* Enumerate devices
	* Look for FTDI devices with Description beginning "Hawkeye V"
	* Look for FTDI device with Description ending in " deviceIDSerial_"
	*    There's usually a port A and a port B on the chip which will show up as different devices.
	*/

	DWORD numDevs;
	// create the device information list 
	FT_STATUS fstat = FT_CreateDeviceInfoList(&numDevs);

	if (fstat != FT_OK)
	{
		using namespace boost::system::errc;
		boost::system::error_code ec = FTDI_Error_To_Boost(fstat);
		return ec;
	}

	if (numDevs == 0)
	{
		return MAKE_ERRC(boost::system::errc::no_such_device);
	}

	FT_DEVICE_LIST_INFO_NODE *devInfo;
	devInfo = (FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*numDevs);
	// get the device information list 
	fstat = FT_GetDeviceInfoList(devInfo, &numDevs);

	if (fstat != FT_OK)
	{
		free(devInfo);
		using namespace boost::system::errc;
		boost::system::error_code ec = FTDI_Error_To_Boost(fstat);
		return ec;
	}


	std::string desc_lead("Hawkeye V");
    std::string dev_desc;
    std::size_t min_length = desc_lead.length() + 1 + identifier.length();
    bool found_it = false;
	std::string description = "";
	for (DWORD i = 0; i < numDevs; i++)
	{
		dev_desc = devInfo[i].Description;
		if (dev_desc.find(desc_lead, 0) != 0)
			continue;

		if (dev_desc.length() < min_length)
			continue;

		if (dev_desc.substr(dev_desc.length() - identifier.length()) != identifier)
			continue;

		// Starts with "Hawkeye V", ends with "deviceIDSerial_".  Must be it.
		found_it = true;
		description = dev_desc;

		// Due to a physical difference in EP and MP controller boards we need to 
		// determine if the controller board is an EP or MP (or later) version.
		// This is needed by the camera software to determine which trigger signal to use for triggering the camera.
		// Example EP controller board description: Hawkeye V3 A
		// Example MP controller board description: Hawkeye VAA A
		// If the character following the 'V' is a number then it is an EP controller board.
		// Otherwise, it is an MP controller board.
		// This is only checked for controller board channel 'A'.
		if (identifier.c_str()[0] == 'A') {
			size_t pos = description.find ('V');
			if (isdigit(description[pos + 1])) {
				isReleasedVersion_ = false;
				Logger::L().Log (MODULENAME, severity_level::normal, "Connected to an EP controller board");
			} else {
				Logger::L().Log (MODULENAME, severity_level::normal, "Connected to an MP (or later) controller board");
			}
		}
		break;
	}

    // check if an older system...
    if ( !found_it )
    {
        desc_lead = "Hawkeye";
        min_length = desc_lead.length() + 1 + identifier.length();
        for (DWORD i = 0; i < numDevs; i++)
        {
            dev_desc = devInfo[i].Description;
            if ( dev_desc.find( desc_lead, 0 ) != 0 )
                continue;

            if ( dev_desc.length() < min_length )
                continue;

            if ( dev_desc.substr( dev_desc.length() - identifier.length() ) != identifier )
                continue;

            // Starts with "Hawkeye", ends with "deviceIDSerial_".  Must be it.
            found_it = true;
            description = dev_desc;
            break;
        }
    }

    // last attempt... for development systems...
    if ( !found_it )
    {
        desc_lead = "Hawkeye A";
        std::string id_sn = CNTLR_PORT_A_SN_STR;
        min_length = desc_lead.length() + 1 + identifier.length();
        for ( DWORD i = 0; i < numDevs; i++ )
        {
            dev_desc = devInfo[i].Description;
            if ( dev_desc.find( desc_lead, 0 ) != 0 )
                continue;

            std::string dev_sn = devInfo[i].SerialNumber;
            if ( dev_sn.find( identifier, 0 ) != 0 )
            {
                id_sn = CNTLR_PORT_B_SN_STR;
                continue;
            }

            // Starts with "Hawkeye A", and uses old serial port designation for serial number...  must be a development system.
            found_it = true;
            description = dev_desc;
            break;
        }
    }

	// Done with this.
	free(devInfo);

	if (!found_it)
	{
		return MAKE_ERRC(boost::system::errc::no_such_device);
	}

	fstat = FT_OpenEx((PVOID)description.c_str(), FT_OPEN_BY_DESCRIPTION, &dev_handle);

	return FTDI_Error_To_Boost(fstat);
}

boost::system::error_code ControllerBoardInterface::OpenSerial(std::string identifier, uint32_t baudRate)
{
	/*
	* Attempt to locate and open the FTDI device whose name matches the provided identifier.
	*  If the identifier matches the tag provided in the constructor (or is blank):
	*    ...and the device is already open: return success immediately
	*    ...and the device is NOT open: attempt to open device.
	*  If the identfier does not match the device name already in use...
	*    ...and the device is already open: close the current device, reset the identifier and attempt to open the device
	*    ...and the device is NOT open: reset the identifier and attempt to open the device
	*  If both the identifier here and the current device name are blank
	*    ...immediately return an error condition.
	*/

	boost::trim(identifier);

	if (identifier.empty() && deviceIDSerial_.empty())
	{
		return MAKE_ERRC(boost::system::errc::invalid_argument);
	}

	if (IsSerialOpen())
	{
		// This device is already open?  All done, then.
		if (deviceIDSerial_ == identifier || identifier.empty())
			return MAKE_SUCCESS;

		// Otherwise, close out existing connection.
		CloseSerial();
	}

	if (!identifier.empty())
	{
		deviceIDSerial_ = identifier;
	}

	boost::system::error_code ec = OpenHawkeyeDeviceByDescription(deviceIDSerial_, deviceSerialHandle_);

	if (ec)
		return ec;

	
	FT_STATUS fstat = FT_ResetDevice(deviceSerialHandle_);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

	fstat = FT_SetBitMode(deviceSerialHandle_, 0, FT_BITMODE_RESET);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

	fstat = FT_Purge(deviceSerialHandle_, FT_PURGE_RX | FT_PURGE_TX);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

	fstat = FT_SetBaudRate(deviceSerialHandle_, baudRate);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

	fstat = FT_SetDataCharacteristics(deviceSerialHandle_, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

    fstat = FT_SetFlowControl( deviceSerialHandle_, FT_FLOW_RTS_CTS, 0, 0 );

    ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

	fstat = FT_SetTimeouts(deviceSerialHandle_, 1100, 1100);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

	unsigned char latency;
	fstat = FT_GetLatencyTimer (deviceSerialHandle_, &latency);
	ec = FTDI_Error_To_Boost (fstat);
	if (ec) return ec;
	Logger::L().Log (MODULENAME, severity_level::normal, "Original FTDI latency: " + std::to_string(latency));

	latency = 2;
	fstat = FT_SetLatencyTimer (deviceSerialHandle_, latency);
	ec = FTDI_Error_To_Boost (fstat);
	if (ec) return ec;

	fstat = FT_GetLatencyTimer (deviceSerialHandle_, &latency);
	ec = FTDI_Error_To_Boost (fstat);
	if (ec) return ec;
	Logger::L().Log (MODULENAME, severity_level::normal, "New FTDI latency: " + std::to_string(latency));

	ULONG inPacketSize = 20;
	ULONG outPacketSize = 20;
	fstat = FT_SetUSBParameters (deviceSerialHandle_, inPacketSize, outPacketSize);
	ec = FTDI_Error_To_Boost (fstat);
	if (ec) return ec;
	Logger::L().Log (MODULENAME, severity_level::normal, "FTDI input packet size: " + std::to_string(inPacketSize));
	Logger::L().Log (MODULENAME, severity_level::normal, "FTDI output packet size: " + std::to_string(outPacketSize));

	deviceIsOpen_Serial_ = true;
	
	DWORD driverVersion;
	FT_GetDriverVersion (deviceSerialHandle_, &driverVersion);
	Logger::L().Log (MODULENAME, severity_level::normal,
		boost::str (boost::format ("FTDI Driver Version (serial port): %x.%x.%x")
			% (int)((driverVersion & 0x00FF0000) >> 16)
			% (int)((driverVersion & 0x0000FF00) >>  8)
			% (int)((driverVersion & 0x000000FF))
		));

	return MAKE_SUCCESS;
}

boost::system::error_code ControllerBoardInterface::OpenGPIO(std::string identifier)
{
	/*
	* Attempt to locate and open the FTDI device whose name matches the provided identifier.
	*  If the identifier matches the tag provided in the constructor (or is blank):
	*    ...and the device is already open: return success immediately
	*    ...and the device is NOT open: attempt to open device.
	*  If the identfier does not match the device name already in use...
	*    ...and the device is already open: close the current device, reset the identifier and attempt to open the device
	*    ...and the device is NOT open: reset the identifier and attempt to open the device
	*  If both the identifier here and the current device name are blank
	*    ...immediately return an error condition.
	*/

	boost::trim(identifier);

	if (identifier.empty() && deviceIDGPIO_.empty())
	{
		return MAKE_ERRC(boost::system::errc::invalid_argument);
	}

	if (IsGPIOOpen())
	{
		// This device is already open?  All done, then.
		if (deviceIDGPIO_ == identifier || identifier.empty())
			return MAKE_SUCCESS;

		// Otherwise, close out existing connection.
		CloseGPIO();
	}

	if (!identifier.empty())
	{
		deviceIDGPIO_ = identifier;
	}

	boost::system::error_code ec = OpenHawkeyeDeviceByDescription(deviceIDGPIO_, deviceGPIOHandle_);

	if (ec)
		return ec;

	
	FT_STATUS fstat = FT_ResetDevice(deviceGPIOHandle_);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

	fstat = FT_Purge(deviceGPIOHandle_, FT_PURGE_RX | FT_PURGE_TX);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

	fstat = FT_SetUSBParameters(deviceGPIOHandle_, 4096, 0);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

	fstat = FT_SetBitMode(deviceGPIOHandle_, 0, FT_BITMODE_RESET);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

	fstat = FT_SetChars(deviceGPIOHandle_, 0, false, 0, false);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

	fstat = FT_SetTimeouts(deviceGPIOHandle_, 2000, 2000);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

	fstat = FT_SetLatencyTimer(deviceGPIOHandle_, 255);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

	fstat = FT_SetFlowControl(deviceGPIOHandle_, FT_FLOW_NONE, 0, 0);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

	fstat = FT_SetBitMode(deviceGPIOHandle_, 0, FT_BITMODE_RESET);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

	fstat = FT_SetBitMode(deviceGPIOHandle_, 0, FT_BITMODE_MPSSE);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

	fstat = FT_SetBaudRate(deviceGPIOHandle_, DefaultGpioBaudRate);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

	fstat = FT_SetDataCharacteristics(deviceGPIOHandle_, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

	fstat = FT_SetTimeouts(deviceGPIOHandle_, 1100, 1100);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return ec;

	Sleep(50);

	deviceIsOpen_GPIO_ = true;

	return MAKE_SUCCESS;
}

void ControllerBoardInterface::CloseSerial()
{
	/*
	* Stop queue processing (if any).
	* Flush queue and handle callbacks.
	* Close device.
	*/
	StopLocalIOService();

	if (deviceIsOpen_Serial_)
		FT_Close(deviceSerialHandle_);
	deviceIsOpen_Serial_ = false;
}

void ControllerBoardInterface::CloseGPIO()
{
	/*
	* Close device.
	*/
	if (deviceIsOpen_GPIO_)
		FT_Close(deviceGPIOHandle_);
	deviceIsOpen_GPIO_ = false;
}

boost::system::error_code ControllerBoardInterface::WriteRegister(uint32_t regAddr, t_ptxrxbuf tx, t_xferCallback Cb)
{
    std::string logStr = boost::str( boost::format( "WriteRegister:: regAddr: %0u" ) %regAddr );

    if ( !tx )
    {
        logStr.append( "   Error: invalid tx pointer" );
        Logger::L().Log ( MODULENAME, severity_level::error, logStr );
        return MAKE_ERRC( boost::system::errc::invalid_argument );
    }

    if ( tx->size() > MaxTxRxSize() )
    {
        logStr.append( boost::str( boost::format( "   Error: regAddr: %0u tx size %d > max %d" ) % regAddr % tx->size() % MaxTxRxSize() ) );
        Logger::L().Log ( MODULENAME, severity_level::error, logStr );
        return MAKE_ERRC( boost::system::errc::value_too_large );
    }

	t_pXferTask task(new XferTask);
	task->XType = eRegWrite;
	task->RegAddr = regAddr;
	task->Callback = Cb;
	task->TXBuf = tx;
	task->RXBuf = NULL;

    Logger::L().Log ( MODULENAME, severity_level::debug3, logStr );

	queue_mtx_.lock();
	boost::system::error_code retStat = Enqueue( task );
	queue_mtx_.unlock();

	if (retStat)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "WriteRegister: Failed to enqueue the task");
	}
	return retStat;
}

boost::system::error_code ControllerBoardInterface::ReadRegister(uint32_t regAddr, uint32_t rxLen, t_ptxrxbuf rx, t_xferCallback Cb)
{
    std::string logStr = boost::str( boost::format( "ReadRegister:: regAddr: %0u   rxLen: %d" ) % regAddr % rxLen );

//	if (!rx)
//	{
//		logStr.append( "   Error: invalid rx pointer" );
//		Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );
//		return MAKE_ERRC(boost::system::errc::invalid_argument);
//	}

    if ( rxLen > MaxTxRxSize() )
    {
        logStr.append( boost::str( boost::format( "   Error: rxLen > %d" ) % MaxTxRxSize() ) );
        Logger::L().Log ( MODULENAME, severity_level::error, logStr );
        return MAKE_ERRC( boost::system::errc::value_too_large );
    }

	t_pXferTask task(new XferTask);
	task->XType = eRegRead;
	task->Callback = Cb;
	task->TXBuf = NULL;
	task->RXBuf = rx;
	task->RegAddr = regAddr;
	task->ReadLen = rxLen;

    Logger::L().Log ( MODULENAME, severity_level::debug3, logStr );

	queue_mtx_.lock();
	boost::system::error_code retStat = Enqueue( task );
	queue_mtx_.unlock();

	return retStat;
}

boost::system::error_code ControllerBoardInterface::ReadAndClearRegister (uint32_t regAddr, uint32_t rxLen, t_ptxrxbuf rx, t_xferCallback Cb) {
	std::string logStr = boost::str(boost::format("ReadAndClearRegister:: regAddr: %0u   rxLen: %d") % regAddr % rxLen);

	//	if (!rx)
	//	{
	//		logStr.append( "   Error: invalid rx pointer" );
	//		Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );
	//		return MAKE_ERRC(boost::system::errc::invalid_argument);
	//	}

	if (rxLen > MaxTxRxSize()) {
		logStr.append(boost::str(boost::format("   Error: rxLen > %d") % MaxTxRxSize()));
		Logger::L().Log (MODULENAME, severity_level::error, logStr);
		return MAKE_ERRC(boost::system::errc::value_too_large);
	}

	t_pXferTask task(new XferTask);
	task->XType = eRegReadAndClear;
	task->Callback = Cb;
	task->TXBuf = NULL;
	task->RXBuf = rx;
	task->RegAddr = regAddr;
	task->ReadLen = rxLen;

	Logger::L().Log (MODULENAME, severity_level::debug3, logStr);

	queue_mtx_.lock();
	boost::system::error_code retStat = Enqueue(task);
	queue_mtx_.unlock();

	return retStat;
}

boost::system::error_code ControllerBoardInterface::Enqueue(t_pXferTask task)
{
	/*
	* Post a task to the private io_service thread.
	* Return an error if the io_service isn't created.
	*/
    std::string logStr = boost::str( boost::format( "Enqueing task:  taskindex: %06d" ) % taskindex );
    std::string taskTypeStr;
    uint16_t bufSize = 0;
    uint32_t taskCmd = 0;
    uint32_t taskType = 0;
    unsigned char * pData = NULL;

	if (!pCBIThread_->GetIoService())
	{
		// Apparently not accepting new work?
        logStr.append( "   Error: not connected" );
        Logger::L().Log ( MODULENAME, severity_level::error, logStr );
        return MAKE_ERRC(boost::system::errc::not_connected);
	}

    if ( !task )
    {
        logStr.append( "   Error: invalid task pointer" );
        Logger::L().Log ( MODULENAME, severity_level::error, logStr );
        return MAKE_ERRC( boost::system::errc::invalid_argument );
    }

    taskType = task->XType;
    if ( taskType != eRegWrite && taskType != eRegRead && taskType != eRegReadAndClear)
    {
        logStr.append( boost::str( boost::format( "   Error: unrecognized task type '%u' " ) % taskType ) );
        Logger::L().Log ( MODULENAME, severity_level::error, logStr );
        return MAKE_ERRC( boost::system::errc::invalid_argument );
    }

    if ( task->XType == eRegWrite )
    {
        if ( !task->TXBuf )
        {
            logStr.append( "   Error: invalid TXBUF pointer" );
            Logger::L().Log ( MODULENAME, severity_level::error, logStr );
            return MAKE_ERRC( boost::system::errc::invalid_argument );
        }
        else
        {
            pData = task->TXBuf->data();
            taskCmd = ( (uint32_t *) task->TXBuf->data() )[0];
            bufSize = (uint16_t)task->TXBuf->size();
        }
    }
    else if ( task->XType == eRegRead || task->XType == eRegReadAndClear)
    {
        bufSize = task->ReadLen;
    }

    if ( bufSize > 4095)
	{
        if ( task->XType == eRegWrite )
        {
            logStr.append( boost::str( boost::format( "  Error: txbuf size %d greater than 4095" ) % bufSize ) );
        }
        else
        {
            logStr.append( boost::str( boost::format( "  Error: readLen %d greater than 4095" ) % bufSize ) );
        }
        Logger::L().Log ( MODULENAME, severity_level::error, logStr );
        return MAKE_ERRC(boost::system::errc::file_too_large);
	}

	if (this->task_transaction_log)
	{
		std::string ts = (task->XType == eRegRead || task->XType == eRegReadAndClear) ? "RD" : "WR";
		task->logstring = boost::str(boost::format("%06d,%s,%d,%s") % taskindex % ts % task->RegAddr % boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::local_time()));
	}

	task->taskindex = taskindex;
	XFERQueue[taskindex] = task;

    taskTypeStr = ( taskType == eRegWrite ) ? "write" : " read";
    logStr.append( boost::str( boost::format( "  taskType: %s  bufSize: %d  QSize: %d" ) % taskTypeStr % bufSize % XFERQueue.size()) );

	if ( taskType == eRegWrite )
    {
        logStr.append( boost::str( boost::format( "  taskCmd: %02u" ) % taskCmd ) );
        //if ( NULL != pData )
        //{
        //    for ( uint16_t i = 0; i < bufSize; )
        //    {
        //        logStr.append( "\n\t" );
        //        for ( int x = 0; ( x < 8 ) && ( i < bufSize ); x++, i++ )
        //        {
        //            logStr.append( boost::str( boost::format( "%02X " ) % (uint32_t)pData[i] ) );
        //        }
        //    }
        //}
    }

    // Push task off to internal IO Service for later processing.
	pCBIThread_->Enqueue(std::bind(&ControllerBoardInterface::ProcessTask_, this, taskindex));

	if (++taskindex > 10000)
		taskindex = 0;

    //Logger::L().Log ( MODULENAME, severity_level::debug1, logStr );

    return MAKE_SUCCESS;
}

void ControllerBoardInterface::CancelQueue()
{
	ClearQueueWithStatus(boost::asio::error::operation_aborted);
}

void ControllerBoardInterface::ClearQueueWithStatus(boost::system::error_code ec)
{
	std::unique_lock<std::mutex> lock(queue_mtx_);
	for (t_taskqueue::iterator it = XFERQueue.begin(); it != XFERQueue.end(); it++)
	{
		if (it->second && it->second->Callback)
		{
			// this workflow does not require the user id, so do not use the transient user technique
			_pUpstream->post(std::bind(it->second->Callback, ec, 0xFFFF, it->second->TXBuf, it->second->RXBuf));
		}
	}
	XFERQueue.clear();
}

void ControllerBoardInterface::ProcessTask_(uint16_t taskID)
{
	/*
	* Called by the private io_service.run() thread.
	*
	* Locate task within map.
	* If task exists, remove it from the map
	*  and execute the task.
	*
	*  Splitting the execution out lets us release
	*  the queue earlier.
	*/
	queue_mtx_.lock();
	if (XFERQueue.find(taskID) == XFERQueue.end())
	{
		queue_mtx_.unlock();
		return;
	}

	t_pXferTask task = XFERQueue[taskID];
	XFERQueue.erase(taskID);
	queue_mtx_.unlock();

	if (task)
	{
		task->boardStatus = 0xFFFF;
		if (this->task_transaction_log) {
			task->logstring += "," + boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::local_time());
		}
		ProcessTask (task);

		if (this->task_transaction_log)
			task->logstring += "," + boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::local_time());

		// only work the callback if we're not taking down the system.
		if ( task->ec != boost::system::errc::operation_canceled )
		{
			if ( task->XType == eRegWrite )
			{
				//if ( ( task->TXBuf->data()[0] > 0 ) && ( task->TXBuf->data()[0] <= 10 ) )
				//{
				//	Logger::L().Log ( MODULENAME, severity_level::debug1, boost::str( boost::format( "ProcessTask_[%d]: 0x%04X" ) % taskID % boardStatus ) );
				//}
			}

			ProcessTaskCallback (task);
		}
	}

}

void ControllerBoardInterface::ProcessTaskCallback (t_pXferTask task) {

	std::string logStr = boost::str(boost::format("ProcessTaskCallback: taskindex: %06d") % task->taskindex);
	Logger::L().Log (MODULENAME, severity_level::debug3, logStr);
	
	logStr = boost::str(boost::format("ProcessTaskCallback: boardStatus (0x%04X): %s") % task->boardStatus % boardStatus_.load().getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug3, logStr);

	if (boardStatus_.load().isSet (BoardStatus::Error)) {	// printf("val = 0x%" PRIx64 "\n", val);
		logStr = boost::str(boost::format("ProcessTaskCallback:  \terrorStatus_ (0x%" PRIx64 "X): %s") % errorStatus_.load().get() % errorStatus_.load().getAsString());
		Logger::L().Log (MODULENAME, severity_level::debug3, logStr);
	}
	
	if (this->task_transaction_log)
		task->logstring += "," + boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::local_time());
	
	std::function<void()> callback = std::bind(task->Callback, task->ec, task->boardStatus, task->TXBuf, task->RXBuf);
	std::function<void()> wrapper = std::bind(&ControllerBoardInterface::TaskWrapper, this, task->logstring, callback);

	// this workflow does not require the user id, so do not use the transient user technique
	_pUpstream->post(wrapper);
}

void ControllerBoardInterface::ProcessTask (t_pXferTask task)
{

	std::lock_guard<std::mutex> lock(task_process_mtx_);
	assert(task);
	task->ec = MAKE_SUCCESS;
	task->boardStatus = 0xFFFF;

	// Ensure interface is open.
	task->ec = OpenSerial();
	if (task->ec != boost::system::errc::success) {
		return;
	}

	t_ptxrxbuf txbuf = CreateTXBUFForTask(task);

	// Send Transmit buffer
	std::size_t retries_remaining = 3;

	while (retries_remaining > 0)
	{
		/*
		* Flush out any remaining information in the FTDI buffers that might have been leftover
		*  from an earlier cycle.
		*/
		FlushReadWriteBuffers();

		retries_remaining--;

		DWORD bytesWritten;
		FT_STATUS fstat = FT_Write(deviceSerialHandle_, (VOID*)txbuf->data(), (DWORD)txbuf->size(), &bytesWritten);

		if (fstat != FT_OK || (bytesWritten != txbuf->size()))
		{
			task->ec = FTDI_Error_To_Boost(fstat);
			continue;
		}

		// Read 2 bytes (LEN) 
		t_ptxrxbuf rxbuf = std::make_shared<std::vector<unsigned char> >();
		rxbuf->resize(2);
		DWORD bytesRead;
		fstat = FT_Read(deviceSerialHandle_, (void*)rxbuf->data(), 2, &bytesRead);

		if (fstat != FT_OK)
		{
			task->ec = FTDI_Error_To_Boost(fstat);
			continue;
		}
		if (bytesRead != 2)
		{
			task->ec = MAKE_ERRC(boost::system::errc::stream_timeout);
			continue;
		}

		uint16_t replyLen = *(uint16_t*)rxbuf->data();

		// Create appropriately sized RX buffer (don't forget to account for existing bytes)
		rxbuf->resize(replyLen);

		// Read in LEN additional bytes
		fstat = FT_Read(deviceSerialHandle_, (void*)(rxbuf->data() + 2), replyLen - 2, &bytesRead);

		if (fstat != FT_OK)
		{
			task->ec = FTDI_Error_To_Boost(fstat);
			continue;
		}
		if (bytesRead != replyLen - 2)
		{
			task->ec = MAKE_ERRC(boost::system::errc::stream_timeout);
			continue;
		}

		// calculate and validate checksum
		if (!VerifyCRC(rxbuf->begin(), rxbuf->end()))
		{
			task->ec = MAKE_ERRC(boost::system::errc::bad_message);
			continue;
		}

		task->boardStatus = *((uint16_t*)(rxbuf->data() + 2));  //cmd_board_status_;
		boardStatus_ = task->boardStatus;

		// Eventually get success and copy the RX buffer over into the task.
		if (!task->RXBuf)
		{
			task->RXBuf = std::make_shared<std::vector<unsigned char> >();
		}

		uint16_t datalen = replyLen - 6;
		if (datalen > 0)
		{
			task->RXBuf->resize(datalen);
			auto b = rxbuf->begin() + 4;
			auto e = b + datalen;
			std::copy(b, e, task->RXBuf->begin());
		}
		else
		{
			task->RXBuf->clear();
		}
		break;
	}

}


void ControllerBoardInterface::TaskWrapper(std::string logstring, std::function<void()> cb)
{
	if (task_transaction_log)
		logstring += "," + boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::local_time());

	if (cb)
		cb();
	
	if (task_transaction_log)
	{
		logstring += "," + boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::local_time()) + "\n";
		fputs(logstring.c_str(), task_transaction_log);
	}
}

uint16_t ControllerBoardInterface::TxBufLenForTask(t_pXferTask task)
{
	/*
	* Command Packet layout:
	*    BYTE   LEN   VAL
	*      0     2    Packet Length (including CRC)
	*      2     1    Command (1:rd, 2:wr, 3:scatter rd)
	*      3     2    Address
	*  RD  5     2    NBytes
	*  WR  5     N    DataBytes
	*    7/5_N   2    CRC16
	*
	*  So: Register read  = 9
	*      Register write = WriteLen + 7
	*/

	if (!task)
		return 0;

	std::size_t len = 0;

	switch (task->XType)
	{
		case eRegRead:
		case eRegReadAndClear:
		{
			len = 9;
			break;
		}
		case eRegWrite:
		{
			if (!task->TXBuf)
				len = 7 + 0;
			else
				len = 7 + task->TXBuf->size();
			break;
		}
	}

	return (uint16_t)len;
}

uint16_t ControllerBoardInterface::TxCRCPosForTask(t_pXferTask task)
{
	assert(task);

	uint16_t pos = 0;
	switch (task->XType)
	{
		case eRegRead:
		case eRegReadAndClear:
			pos = 7;
			break;
		case eRegWrite:
		{
			if (task->TXBuf)
				pos = (uint16_t)(5 + task->TXBuf->size());
			else
				pos = 5 + 0;
			break;
		}
	}

	return pos;
}

uint16_t ControllerBoardInterface::CalculateCRC(t_txrxbuf::iterator start, t_txrxbuf::iterator finish)
{
	uint8_t CRCMSB = 255;
	uint8_t CRCLSB = 255;
	for (auto iter = start; iter != finish; iter++)
	{
		uint8_t X = *iter ^ CRCMSB;
		X = X ^ (X >> 4);
		CRCMSB = (CRCLSB ^ (X >> 3) ^ (X << 4)) & 0xFF;
		CRCLSB = (X ^ (X << 5)) & 0xFF;
	}

	return  ((uint16_t)(~CRCMSB << 8 | (~CRCLSB & 0xFF)));
}

bool ControllerBoardInterface::VerifyCRC(t_txrxbuf::iterator start, t_txrxbuf::iterator finish)
{
	uint16_t crc = CalculateCRC(start, finish);
	uint16_t not_crc = ~crc;
	return (not_crc == 0x1D0F);
}

ControllerBoardInterface::t_ptxrxbuf ControllerBoardInterface::CreateTXBUFForTask(t_pXferTask task)
{
	assert(task);
	/*
	* Command Packet layout:
	*    BYTE   LEN   VAL
	*      0     2    Packet Length (including CRC)
	*      2     1    Command (1:rd, 2:wr, 3:scatter rd)
	*      3     2    Address
	*  RD  5     2    NBytes
	*  WR  5     N    DataBytes
	*    7/5_N   2    CRC16
	*
	*  So: Register read  = 9
	*      Register write = WriteLen + 7
	*/

	t_ptxrxbuf txb(new t_txrxbuf());

	txb->resize(TxBufLenForTask(task));

	uint16_t* plen = (uint16_t*)(txb->data() + 0);
	*plen = TxBufLenForTask(task);

	uint16_t* addr = (uint16_t*)(txb->data() + 3);
	*addr = task->RegAddr;

	uint8_t* cmd = (uint8_t*)(txb->data() + 2);
	switch (task->XType)
	{
		case eRegRead:
		{
			*cmd = 1;
			uint16_t* dlen = (uint16_t*)(txb->data() + 5);
			*dlen = task->ReadLen;
			break;
		}
		case eRegWrite:
		{
			*cmd = 2;
			std::copy(task->TXBuf->begin(), task->TXBuf->end(), txb->begin() + 5);
			break;
		}
		case eRegReadAndClear:
		{
			*cmd = 3;
			uint16_t* dlen = (uint16_t*)(txb->data() + 5);
			*dlen = task->ReadLen;
			break;
		}
	}

	uint16_t crc;
	crc = CalculateCRC(txb->begin(), txb->begin() + TxCRCPosForTask(task));

	unsigned char* pcrc = (txb->data() + TxCRCPosForTask(task));
	*pcrc = (crc >> 8) & 0x0FF;
	pcrc++;
	*pcrc = (crc & 0x0FF);

	return txb;
}

void ControllerBoardInterface::FlushReadWriteBuffers()
{
	FT_Purge(deviceSerialHandle_, FT_PURGE_RX | FT_PURGE_TX);
}

void ControllerBoardInterface::StartLocalIOService()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "StartLocalIOService: <enter>");

	// Start local queue processing with no existing state.
	StopLocalIOService();

	if (!pCBIThread_)
	{
		pCBIThread_ = std::make_unique<HawkeyeThread>("ControllerBoardInterface_Thread");
	}
	Logger::L().Log (MODULENAME, severity_level::debug1, "StartLocalIOService: <exit>");
}

void ControllerBoardInterface::StopLocalIOService()
{
	if (pCBIThread_)
	{
		pCBIThread_.reset();
	}
}

void ControllerBoardInterface::PauseLocalIOService()
{
	if (pCBIThread_)
	{
		pCBIThread_->Pause();
	}
}

void ControllerBoardInterface::ResumeLocalIOService()
{
	if (pCBIThread_)
	{
		pCBIThread_->Resume();
	}
	else
	{
		StartLocalIOService();
	}
}

/**
 * \brief Return the cached hardware level value obtained from the FTDI EEPROM description.
 * \return short (int16_t) - hardware level value 
 */
uint16_t ControllerBoardInterface::GetBoardHardwareLevel() const
{
	return hardware_level_;
}

boost::system::error_code ControllerBoardInterface::SetGPIO(bool p1, bool p2, bool p3)
{
	if (!IsGPIOOpen())
	{
		return MAKE_ERRC(boost::system::errc::no_such_device);
	}

	unsigned char buf[3];
	buf[0] = 0x81; // cmd=Get Data bits LowByte
	DWORD bytesWritten = 0;

	/*
	* Get current status
	*/
	FT_STATUS stat = FT_Write(deviceGPIOHandle_, buf, 1, &bytesWritten);
	if (stat != FT_OK)
	{
		return FTDI_Error_To_Boost(stat);
	}
	stat = FT_Read(deviceGPIOHandle_, buf, 1, &bytesWritten);
	if (stat != FT_OK)
	{
		return FTDI_Error_To_Boost(stat);
	}

#if defined(REV1_FW)        // the new intitialization appears to be required for ALL new (V2 and later) boards
//#if defined(REV1_FW) || defined (REV2_FW)
    /*
    * Modify the low byte GPIO data and write it back
    */
    unsigned char mask = ( buf[0] & 0x0F ); // preserve DI,DO,CS,CLK
    if ( p1 ) mask |= 0x10;
    if ( p2 ) mask |= 0x20;
    if ( p3 ) mask |= 0x40;
    buf[0] = 0x80; // cmd=Set Data bits LowByte
    buf[1] = mask; // values
    buf[2] = 0xFB; // directions (0=input, 1=output)
#else
    unsigned char mask = 0x00;
    if ( p1 ) mask |= 0x1;
    if ( p2 ) mask |= 0x2;
    if ( p3 ) mask |= 0x4;
    // Set the high byte GPIO data
    buf[0] = 0x82; // cmd=Set Data bits HighByte
    buf[1] = mask; // values
    buf[2] = 0x07; // directions (0=input, 1=output)
#endif
    stat = FT_Write( deviceGPIOHandle_, buf, 3, &bytesWritten );
    return FTDI_Error_To_Boost( stat );
}

boost::system::error_code ControllerBoardInterface::GetGPIO(bool& p1, bool& p2, bool& p3)
{
	if (!IsGPIOOpen())
	{
		return MAKE_ERRC(boost::system::errc::no_such_device);
	}

	unsigned char buf[1];

	buf[0] = 0x81; // cmd=Get Data bits LowByte
	DWORD bytesWritten = 0;

	/*
	* Get current status
	*/
	FT_STATUS stat = FT_Write(deviceGPIOHandle_, buf, 1, &bytesWritten);
	if (stat != FT_OK)
	{
		return FTDI_Error_To_Boost(stat);
	}
	stat = FT_Read(deviceGPIOHandle_, buf, 1, &bytesWritten);
	if (stat != FT_OK)
	{
		return FTDI_Error_To_Boost(stat);
	}

	p1 = (buf[0] & 0x10) != 0;
	p2 = (buf[0] & 0x20) != 0;
	p3 = (buf[0] & 0x40) != 0;
	return MAKE_SUCCESS;
}

boost::system::error_code ControllerBoardInterface::SetGPIO(std::size_t pinNum, bool pinVal)
{
	/*
	* This is a bit wasteful since it ends up doing 2 full reads of the IO state,
	*  once in GetGPIO(p1,p2,p3), a second time in SetGPIO(p1,p2,p3).
	*/
	bool p1, p2, p3;
	boost::system::error_code ec = GetGPIO(p1, p2, p3);
	if (ec)
		return ec;

	switch (pinNum)
	{
		case 1:
			p1 = pinVal; break;
		case 2:
			p2 = pinVal; break;
		case 3:
			p3 = pinVal; break;
		default:
			return MAKE_ERRC(boost::system::errc::no_such_device_or_address);
	}
	return SetGPIO(p1, p2, p3);

}

boost::system::error_code ControllerBoardInterface::GetGPIO(std::size_t pinNum, bool& pinVal)
{
	bool p1, p2, p3;
	boost::system::error_code ec = GetGPIO(p1, p2, p3);
	if (ec)
		return ec;

	switch (pinNum)
	{
		case 1:
			pinVal = p1; break;
		case 2:
			pinVal = p2; break;
		case 3:
			pinVal = p3; break;
		default:
			return MAKE_ERRC(boost::system::errc::no_such_device_or_address);
	}
	return MAKE_SUCCESS;
}

bool ControllerBoardInterface::EnableK70FirmwareUpdate (boost::system::error_code& ec)
{
	uint8_t status;

	StopLocalIOService();

	ez_port_.reset(new EzPort(_pUpstream, deviceGPIOHandle_));

	if (!ez_port_->Init(deviceGPIOHandle_, hardware_level_, ec))
	{
		ResumeLocalIOService();
		return false;
	}
	if (!ez_port_->Connect(ec))
	{
		ResumeLocalIOService();
		return false;
	}
	if (!ez_port_->Purge(ec))
	{
		ResumeLocalIOService();
		return false;
	}
	if (!ez_port_->CheckStatus(status, ec) || !ez_port_->EnableProgramming(ec))
	{
		ResumeLocalIOService();
		//The status check did not pass.
		Logger::L().Log (MODULENAME, severity_level::error, "Something went wrong when starting to program to K70 chip.");
		ez_port_->Disconnect(ec);//must disconnect.
		return false;
	}
	return true;
}

bool ControllerBoardInterface::DisableK70FirmwareUpdate(boost::system::error_code& ec)
{
	if (!ez_port_->Disconnect(ec)) return false;

	SetGPIO(true, true, false);
	::Sleep(100);
	SetGPIO(true, true, true);

	StartLocalIOService();

	GetFirmwareVersion();

	return true;
}

bool ControllerBoardInterface::ProgramFirmwareK70(uint32_t & address, std::vector<uint8_t> & data, boost::system::error_code & ec)
{
	if (!ez_port_->Program(address, data, ec)) return false;
	return true;
}

bool ControllerBoardInterface::DisconnectFirmwareK70(boost::system::error_code & ec)
{
	if (!ez_port_->Disconnect(ec)) return false;
	return true;
}

void ControllerBoardInterface::ReadVersion(std::function<void(bool)> callback)
{
	auto onVersionRead = [this, callback](boost::system::error_code ec, uint16_t status, ControllerBoardInterface::t_ptxrxbuf tx, ControllerBoardInterface::t_ptxrxbuf rx) -> void
	{
		if (ec)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "cb_readVersion reports an error: " + ec.message());
			// this workflow does not require the user id, so do not use the transient user technique
			_pUpstream->post(std::bind(callback, false));
			return;
		}
		if (!rx)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "cb_readVersion received no buffer!");
			// this workflow does not require the user id, so do not use the transient user technique
			_pUpstream->post(std::bind(callback, false));
			return;
		}

		uint32_t* p = (uint32_t*)rx->data();
		if (!p)
		{
			Logger::L().Log (MODULENAME, severity_level::warning, "cb_readVersion received no data!  BoardStatus: " + BoardStatus(status).getAsString());
			// this workflow does not require the user id, so do not use the transient user technique
			_pUpstream->post(std::bind(callback, false));
			return;
		}

		firmwareVersionUint32_ = *p;
		firmwareVersion_ = boost::str(boost::format("%d.%d.%d.%d")
										% ((*p & 0xFF000000) >> 24)
										% ((*p & 0x00FF0000) >> 16)
										% ((*p & 0x0000FF00) >> 8)
										% ((*p & 0x000000FF)));

		std::string str;

		auto appId = firmwareVersion_.substr(0, 1);
		if (appId == "1")
		{			// "1" - Identifier for application firmware
			str = "firmware v" + firmwareVersion_;
		}
		else if (appId == "4")
		{	// "4" - Identifier for bootloader firmware
			str = "bootloader v" + firmwareVersion_;
		}
		else
		{
			str = "unknown firmware application";
		}

		Logger::L().Log (MODULENAME, severity_level::normal, "ControllerBoardInterface " + str);
		// this workflow does not require the user id, so do not use the transient user technique
		_pUpstream->post(std::bind(callback, true));

	};

	pCBIThread_->Enqueue(std::bind(&ControllerBoardInterface::ReadRegister, this, 0, 4, ControllerBoardInterface::t_ptxrxbuf(), onVersionRead));
}

std::string ControllerBoardInterface::GetFirmwareVersion()
{
	return firmwareVersion_;
}

uint32_t ControllerBoardInterface::GetFirmwareVersionUint32()
{
	return firmwareVersionUint32_;
}

bool ControllerBoardInterface::readEEPROM_Description (int device_number, std::string& descrip) const
{
	FT_HANDLE fthandle;
	FT_STATUS status;
	char Manufacturer[64];
	char ManufacturerId[64];
	char Description[64];
	char SerialNumber[64];
	FT_EEPROM_HEADER ft_eeprom_header;
	ft_eeprom_header.deviceType = FT_DEVICE_2232H; // FTxxxx device type to be	accessed
	FT_EEPROM_2232H ft_eeprom_2232h;
	ft_eeprom_2232h.common = ft_eeprom_header;
	ft_eeprom_2232h.common.deviceType = FT_DEVICE_2232H;
	status = FT_Open(device_number, &fthandle);
	if (status != FT_OK)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "Open EEPROM status not FT_OK. " + std::to_string(status));
		return false;
	}

	status = FT_EEPROM_Read(fthandle, &ft_eeprom_2232h, sizeof(ft_eeprom_2232h),
							Manufacturer, ManufacturerId, Description, SerialNumber);
	if (status != FT_OK)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "readEEPROM_Description: EEPROM read status not FT_OK: " + std::to_string(status) + " skipping");
		FT_Close(fthandle);
		return false;
	}
	else
	{
		descrip = Description;
	}
	FT_Close(fthandle);
	return true;
}
