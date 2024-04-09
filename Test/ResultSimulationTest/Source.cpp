#include "stdafx.h"

using namespace std;
#include "RetrieveAPIsTest.hpp"

#include <functional>
#include <io.h>  
#include <ios>
#include <boost/algorithm/string.hpp>
#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)
#include <boost/foreach.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>


static boost::asio::io_context io_svc_;

void waitForHardwareInitialization(const boost::system::error_code&, boost::asio::deadline_timer *tim)
{
	InitializationState initializationState = IsInitializationComplete();
	switch (initializationState)
	{
		case eInitializationInProgress:
			std::cout << "Initialization is not complete:\n" << std::endl;
			tim->expires_at(tim->expires_at() + boost::posix_time::seconds(3));
			tim->async_wait(boost::bind(waitForHardwareInitialization, boost::asio::placeholders::error, tim));
			return;

		case eInitializationFailed:
			std::cout << "Initialization failed:\n" << std::endl;
			break;

		case eFirmwareUpdateInProgress:
			std::cout << "Firmware update in progress:\n" << std::endl;
			tim->expires_at(tim->expires_at() + boost::posix_time::seconds(3));
			tim->async_wait(boost::bind(waitForHardwareInitialization, boost::asio::placeholders::error, tim));
			return;

		case eInitializationComplete:
			std::cout << "Initialization is complete:\n" << std::endl;
			break;

		default:
			std::cout << "Unknown initialization status:\n" << std::endl;
	}
}



bool  mainMenu()
{
	std::string input;
	std::cin.clear();
	std::cout << "\nSelect the following option:" << std::endl;
	std::cout << "1. Perform Reanalysis" << std::endl;
	std::cout << "2. Perform retrieveImageTest" << std::endl;
	std::cout << "3. Perform Delete Operation" << std::endl;
	std::cout << "4. Perform RetrieveResultRecordTest" << std::endl;
	std::cout << "5. Perform export data test" << std::endl;
	std::cout << "6. Cancel export data" << std::endl;
	std::cout << "7. Perform SampleRecordTest" << std::endl;
	std::cout << "8. Perform EEPROM Test" << std::endl;
	std::cout << "9. To Quit" << std::endl;

	std::cin >> input;
	RetrieveAPIsTest testObj;
	switch (std::stoi(input))
	{
		case 1:
		{
			char charInput;
			std::cout << "Enter: \n\t\"y\" for reanalysis with images OR \n\t\"n\" for reanalysis with image data" << std::endl;
			cin >> charInput;
			testObj.Reanalyzetest(charInput == 'y');
			return true;
		}
		
		case 2:
		{
			testObj.RetrieveImageTest();
			return true;
		}
		
		case 3:
		{
			testObj.DeleteResultsDataTest();
			return true;
		}
		
		case 4:
		{
			char charInput;
			std::cout << "Enter: \n\t\"y\" to print results OR \n\t\"n\" to skip printing" << std::endl;
			cin >> charInput;
			testObj.ResultrecordTest(charInput == 'y');
			return true;
		}
		
		case 5:
		{
			testObj.ExportDataTest();
			return true;
		}
		
		case 6:
		{
			auto he = CancelExportData();
			if (he != HawkeyeError::eSuccess)
				std::cout << "Failed to cancel the Export Data : " << std::string(HawkeyeErrorAsString(he));
			else
				std::cout << "Export Data Cancelled: " << std::endl;
			return true;
		}

		case 7:
		{
			testObj.SampleRecordTest();
			return true;
		}
		case 8:
		{
			auto he = svc_SetSystemSerialNumber("Vi-CELLBLU_0067", "222379");
			if (he != HawkeyeError::eSuccess)
				std::cout << "Failed to set the serial number : " << std::string(HawkeyeErrorAsString(he));
			else
				std::cout << "Set system serial number success" << std::endl;
			
			return true;
		}
		case 9:
		{
			Shutdown();
			return false;
		}
		
		default:
			return true;
	}
}
//*****************************************************************************
int main(int argc, char* argv[])
{
	Initialize(false);
	boost::asio::io_context io;
	boost::asio::deadline_timer t(io, boost::posix_time::seconds(5));

	t.async_wait(boost::bind(waitForHardwareInitialization, boost::asio::placeholders::error, &t));
	io.run();

	LoginUser("bci_service", "222379");

	//LoginUser("factory_admin", "Vi-CELL#0");

	std::string Qc = "BCI QualityControl Name#1";
	std::string Bp = "BCI BioProcess Name#1";

	bool displayMain = true;
	while (displayMain)
	{
		displayMain = mainMenu();
	}
	
	//testObj.CellTypeTest();
	//testObj.GetCellTypeIndicesTest();
	//testObj.AnalysisTest();
	//testObj.TempCellTypeTest();
	//testObj.TempAnalysisTest();
	//testObj.WorkqueueRecordTest();
	//testObj.SampleRecordTest();
	//testObj.SampleImageSetRecordTest();
	//testObj.ImageRecordTest();

	/*testObj.SampleRecordForBPQCTest(Bp);
	testObj.SampleRecordForBPQCTest(Qc);*/
	

	//testObj.RetrieveSampleDataTest();
	/*testObj.BiprocessTest();
	testObj.QualityControlTest();
	testObj.ResultrecordTest();
	testObj.SampleRecordTest();

	testObj.resRecUUIDList.size();
	testObj.imageRecUUIDList.size();*/

	//ImageWrapper_t *ww;
	
	//RetrieveAnnotatedImage(testObj.resRecUUIDList[0], testObj.imageRecUUIDList[10], ww);
	//RetrieveBWImage(testObj.imageRecUUIDList[10], ww);
	//RetrieveImage(testObj.imageRecUUIDList[10], ww);


#if 0
	error_log_entry *errorlog;
	uint32_t numOfEntries = 0;
	RetrieveInstrumentErrorLogRange(0, 123456789, numOfEntries, errorlog);

	audit_log_entry *auditlog;
	RetrieveAuditTrailLogRange(0, 123456789, numOfEntries, auditlog);

	sample_activity_entry *sampleEntry;
	RetrieveSampleActivityLogRange(0, 123456789, numOfEntries, sampleEntry);

	calibration_history_entry *calibEntry;
	RetrieveCalibrationActivityLogRange((calibration_type)1, 0, 123456789, numOfEntries, calibEntry);
#endif

	system("pause");
	Shutdown();
	return 0;
}




