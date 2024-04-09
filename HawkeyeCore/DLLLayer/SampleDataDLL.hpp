#pragma once

#include <cstdint>
#include <string>

#include "AnalysisDefinitionsDLL.hpp"
#include "DataConversion.hpp"
#include "SampleData.hpp"
#include "SampleDefinitionDLL.hpp"


struct SampleDataDLL
{
	SampleDataDLL& operator= (const SampleData& sp)
	{
		return *this;
	}

	SampleData ToCStyle()
	{
		SampleData sd = {};

		return sd;
	}

	DBApi::DB_SampleRecord ToDbStyle (const SampleDefinitionDLL& sd)
	{
		DBApi::DB_SampleRecord sr = {};
	}

	//*****************************************************************************
	static void Log (std::stringstream& ss, const SampleDataDLL& sp)
	{
	}

	int64_t sampleIdNum;
	uuid__t sampleUuid;
	int32_t status;
	SampleParametersDLL parameters;
	uuid__t imageSetUuid;
	uuid__t imageAnalysisUuid;

	uint16_t numReagents;
	
	std::string instrumentSN;
	std::string username;

	std::vector<std::string> reagentTypeNameList;       // list of the names of the reagents in the packs formatted as “type - idnum" for each list entry
	std::vector<std::string> reagentPackNumList;        // reagent pack numbers used for the acquisition of this sample
	std::vector<std::string> packLotNumList;            // pack lot numbers used during acquisition of this sample
	std::vector<system_TP>   packLotExpirationList;     // array of time stamps; pack manufacture expiration dates
	std::vector<system_TP>   packInServiceList;         // array of time stamps; date placed in service
	std::vector<system_TP>   packServiceExpirationList; // array of time stamps; expiration after being placed in service


};
