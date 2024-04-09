#include "stdafx.h"

#include <iostream>
#include <HawkeyeLogic.hpp>
#include "boost\filesystem.hpp"
#include <boost/algorithm/string.hpp>
#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/thread.hpp>

#include "FileHandlingUtilities.hpp"

/* Test Input for File Path, File Name and File Extension validate API's*/
// Relative Path
static std::string filePath_1 = "..\\Config\\Scout_Configuration.cfg";
// Obsolute path
static std::string filePath_2 = "C:\\Instrument\\Config\\Scout_Configuration.cfg";
// Just File Name
static std::string filePath_3 = "Scout_Configuration.cfg";
// File Extension
static std::string fileextension = ".cfg";
// Incorrect File extension
static std::string filePath_4 = "C:\\Instrument\\Config\\Scout_Configuration.config";
//NO file name
static std::string filePath_5 = "C:\\Instrument\\Config\\.";

void TestFileValidateUtilities(void)
{
	std::cout << "*TESTING START* : " << "Validate File Path, Name and Extension API's" << std::endl;
	auto testValidateApis = [](const std::string& filePath, const std::string& fileextension)->void {
		std::cout << "---------------------------------" << std::endl;
		std::cout << "VerifyUserAccessiblePath : " <<
			std::boolalpha << FileSystemUtilities::VerifyUserAccessiblePath(filePath) << std::endl;
		std::cout << "ValidateFileNameAndPath : " <<
			std::boolalpha << FileSystemUtilities::ValidateFileNameAndPath(filePath) << std::endl;
		std::cout << "CheckFileExtension : " <<
			std::boolalpha << FileSystemUtilities::CheckFileExtension(filePath, fileextension) << std::endl;
		std::cout << "---------------------------------\n" << std::endl;
	};

	std::cout << "Input Type :   " << "Relative Path : " << filePath_1 << std:: endl;
	testValidateApis(filePath_1, fileextension);
	std::cout << "Input Type :   " << "Obsolute path : " << filePath_2 << std:: endl;
	testValidateApis(filePath_2, fileextension);
	std::cout << "Input Type :   " << "Just File Name : " << filePath_3 << std:: endl;
	testValidateApis(filePath_3, fileextension);
	std::cout << "Input Type :   " <<  "Incorrect File extension : " << filePath_4 <<std::endl;
	testValidateApis(filePath_4, fileextension);
	std::cout << "Input Type :   " << "No file name in the Path : " << filePath_5 << std::endl;
	testValidateApis(filePath_5, fileextension);
	std::cout << "*TESTING END* : " << "Validate File Path, Name and Extension API's\n\n" << std::endl;
}

void TestImportExportConfigurationApis()
{
	std::cout << "*TESTING START* : " << "Export and Import Configuration API's" << std::endl;
	std::cout << "---------------------------------" << std::endl;
	std::cout << "ExportInstrumentConfiguration : " << HawkeyeErrorAsString(ExportInstrumentConfiguration(filePath_2.c_str())) << std::endl;
	std::cout << "---------------------------------" << std::endl;
	std::cout << "ImportInstrumentConfiguration : " << HawkeyeErrorAsString(ImportInstrumentConfiguration(filePath_2.c_str())) << std::endl;
	std::cout << "---------------------------------\n" << std::endl;
	std::cout << "*TESTING END* : " << "Export and Import Configuration API's\n\n" << std::endl;
}


void waitForHardwareInitialization(const boost::system::error_code&, boost::asio::deadline_timer *tim)
{

	InitializationState initializationState = IsInitializationComplete();
	switch (initializationState) {
		case eInitializationInProgress:
			std::cout << "Initialization In Progress...\n" << std::endl;
			tim->expires_at(tim->expires_at() + boost::posix_time::seconds(3));
			tim->async_wait(boost::bind(waitForHardwareInitialization, boost::asio::placeholders::error, tim));
			return;

		case eInitializationFailed:
			std::cout << "Initialization In Failed...\n" << std::endl;
			break;

		case eFirmwareUpdateInProgress:
			std::cout << "Firmware Update In Progress...\n" << std::endl;
			tim->expires_at(tim->expires_at() + boost::posix_time::seconds(3));
			tim->async_wait(boost::bind(waitForHardwareInitialization, boost::asio::placeholders::error, tim));
			return;

		case eInitializationComplete:
			std::cout << "Initialization complete...\n" << std::endl;
			break;

		default:
			std::cout << "Initialization Failed, Unknown Initialization Status...\n" << std::endl;
	}

}
int main(int argc, char* argv[])
{
	Initialize(true);

	boost::asio::io_context io;
	boost::asio::deadline_timer t(io, boost::posix_time::seconds(5));

	t.async_wait(boost::bind(waitForHardwareInitialization, boost::asio::placeholders::error, &t));
	io.run();

	TestFileValidateUtilities();
	TestImportExportConfigurationApis();
	
	LoginUser("bci_admin", "bci_admin");

	TestImportExportConfigurationApis();
	std::getchar();
	return 0;
}






