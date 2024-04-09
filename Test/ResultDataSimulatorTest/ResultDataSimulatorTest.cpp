#pragma once

#include "ResultDataSimulatorTest.h"
#include <vector>
#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)
#include "HawkeyeLogic.hpp"
#include "TestAPIs.hpp"

static void Print_ReagentPack(ReagentContainerState status)
{
	std::cout << "*****ReagentContainerState*****" << std::endl;
	std::cout << "identifier : " << status.identifier << std::endl;
	std::cout << "bci_part_number : " << status.bci_part_number << std::endl;
	std::cout << "exp_date : " << status.exp_date << std::endl;
	std::cout << "in_service_date : " << status.in_service_date << std::endl;
	std::cout << "lot_information : " << status.lot_information << std::endl;
	std::cout << "status : " << (uint32_t)status.status << std::endl;
	std::cout << "events_remaining : " << status.events_remaining << std::endl;
	std::cout << "position : " << status.position << std::endl;
	std::cout << "num_reagents : " << (int)status.num_reagents << std::endl;
	std::cout << "reagent_states : " << std::endl;
	std::cout << "{" << std::endl;
	std::cout << "  " << "reagent_index : " << status.reagent_states->reagent_index << std::endl;
	std::cout << "  " << "lot_information : " << status.reagent_states->lot_information << std::endl;
	std::cout << "  " << "events_possible : " << status.reagent_states->events_possible << std::endl;
	std::cout << "  " << "events_remaining : " << status.reagent_states->events_remaining << std::endl;
	std::cout << "  " << "valve_location : " << (int)status.reagent_states->valve_location << std::endl;
	std::cout << "}" << std::endl;
	std::cout << "*******************************" << std::endl;
}

static void Print_ReagentDefinition(ReagentDefinition rd)
{
	std::cout << "*****ReagentDefinition*****" << std::endl;
	std::cout << "reagent_index : " << rd.reagent_index << std::endl;
	std::cout << "label : " << rd.label << std::endl;
	std::cout << "mixing_cycles : " << (int)rd.mixing_cycles << std::endl;
	std::cout << "***************************" << std::endl;
}

static void Test_ReagentPack()
{
	uint16_t reagent_num = 0;
	ReagentContainerState* status = nullptr;
	
	std::cout << "*****GetReagentContainerStatus(0, status)*****" << std::endl;
	std::cout << std::endl;
	if (GetReagentContainerStatus(reagent_num, status) == HawkeyeError::eSuccess)
	{
		Print_ReagentPack(*status);		
	}
	std::cout << std::endl;
	std::cout << "**********************************************" << std::endl;

	std::cout << std::endl << std::endl;

	uint16_t reagent_nums;
	ReagentContainerState*  statusList;
	std::cout << "*****GetReagentContainerStatusAll(reagent_nums, statusList)*****" << std::endl;
	std::cout << std::endl;
	if (GetReagentContainerStatusAll(reagent_nums, statusList) == HawkeyeError::eSuccess)
	{
		for (int16_t index = 0; index < reagent_nums; index++)
		{
			Print_ReagentPack(statusList[index]);
		}	
	}
	std::cout << std::endl;
	std::cout << "****************************************************************" << std::endl;
	
	std::cout << std::endl << std::endl;

	uint32_t num_reagents;
	ReagentDefinition* rdList;
	std::cout << "*****GetReagentDefinitions(num_reagents, rdList)*****" << std::endl;
	std::cout << std::endl;
	if (GetReagentDefinitions(num_reagents, rdList) == HawkeyeError::eSuccess)
	{
		for (uint32_t index = 0; index < num_reagents; index++)
		{
			Print_ReagentDefinition(rdList[index]);
		}
	}
	std::cout << std::endl;
	std::cout << "*****************************************************" << std::endl;

	std::cout << std::endl << std::endl;

	std::cout << "*****FreeReagentDefinitions(rdList, num_reagents)*****" << std::endl;
	std::cout << std::endl;
	FreeReagentDefinitions(rdList, num_reagents);
	std::cout << std::endl;
	std::cout << "******************************************************" << std::endl;	

	std::cout << "*****FreeListOfReagentContainerState(statusList, reagent_nums)*****" << std::endl;
	std::cout << std::endl;
	FreeListOfReagentContainerState(statusList, reagent_nums);
	std::cout << std::endl;
	std::cout << "*************************************************************" << std::endl;	
}

static void TestAPIList()
{
	std::cout << "Service user Login" << std::endl;
	/*std::cout << "Administrator Login" << std::endl;*/
	auto userName = "factory_admin"; // "bci_service";
	char* password = "Vi-CELL#0";	//	"222379";
	LoginUser(userName, password);

	//std::cout << std::endl;
	//TestAPIs::Test_Signatures();
	//std::cout << std::endl;
	//TestAPIs::Test_Analysis();
	//std::cout << std::endl;
	//TestAPIs::Test_Bioprocess();
	//std::cout << std::endl;
	//TestAPIs::Test_CellType(); 
	//std::cout << std::endl;
	//TestAPIs::Test_QualityControl();
	//std::cout << std::endl;
	//TestAPIs::Test_ReagentPack();
	//std::cout << std::endl;
	//TestAPIs::Test_Results();
	//std::cout << std::endl;	
	//TestAPIs::Test_SystemStatus();
	//std::cout << std::endl;
	//TestAPIs::Test_CommonUtilities();
	//std::cout << std::endl;	
	////TestAPIs::Test_DeadlineTimerHelper();
	//std::cout << std::endl;
	//TestAPIs::Test_BrightfieldDustSubtract();
	std::cout << std::endl;
	TestAPIs::Test_UserManagement(password);
	//std::cout << std::endl;
	/*TestAPIs::Test_Services(password);
	std::cout << std::endl;
	TestAPIs::Test_Services_Analysis();
	std::cout << std::endl;*/
	//TestAPIs::Test_StageController();
	//TestAPIs::Test_Reports();

	std::cout << std::endl;
	std::cout << "API calls which were unsuccessful!!!" << std::endl;

	HANDLE  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, 12); //red

	auto failedAPIs = TestAPIs::Test_GetFailedCalls();
	for(auto item : failedAPIs)
	{
		std::cout << item << std::endl;
	}
	failedAPIs.clear();

	SetConsoleTextAttribute(hConsole, 7); //white
	std::cout << std::endl;	
  
	LogoutUser();
}

void waitForHardwareInitialization(const boost::system::error_code& /*e*/, boost::asio::deadline_timer *tim)
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

		case InitializationState::eInitializationStopped_CarosuelTubeDetected:
			std::cout << "Carousel tube found during initialization:\n" << std::endl;
			std::cout << "Initializing DLL module:\n" << std::endl;
			Initialize(true);
			tim->expires_at(tim->expires_at() + boost::posix_time::seconds(5));
			tim->async_wait(boost::bind(waitForHardwareInitialization, boost::asio::placeholders::error, tim));
			return;

		case eInitializationComplete:
			std::cout << "Initialization is complete:\n" << std::endl;
			TestAPIList();
			break;

		default:
			std::cout << "Unknown initialization status:\n" << std::endl;
	}
}

ResultDataSimulatorTest::ResultDataSimulatorTest()
{
	
}

ResultDataSimulatorTest::~ResultDataSimulatorTest()
{

}

void ResultDataSimulatorTest::ExecuteSimulator()
{
	/*ResultDataDecoder decoder;

	std::vector<ResultsGPPOISettings> blobDataVec;
	FillImageBlobData(blobDataVec);
	image_blobs_tDLL imageBlobData;
	decoder.Decode(&imageBlobData, blobDataVec);

	{
		std::vector<ResultsGPPOISettings> resultDataVec;
		FillResultData(resultDataVec);
		BasicResultAnswers basicResult;
		decoder.Decode(&basicResult, resultDataVec);
	}

	std::vector<CellIdentificationParam> analysisParamDataVec;
	FillAnalysisParamData(analysisParamDataVec);
	std::vector<AnalysisParameterDLL> analysisParams;
	decoder.Decode(analysisParams, analysisParamDataVec);*/

	Initialize(false);	

	boost::asio::io_context io;
	boost::asio::deadline_timer t(io, boost::posix_time::seconds(5));

	t.async_wait(boost::bind(waitForHardwareInitialization, boost::asio::placeholders::error, &t));
	io.run();	

	/*std::string csvfilePath = "ImageInfo_1.csv";
	ResultDataSimulator* simulator = new ResultDataSimulator();

	simulator->LoadResultData(csvfilePath);

	BasicResultAnswers resultData = simulator->GetBasicAnswerResult();

	delete simulator;*/
}
