#pragma once

#include <DBif_Api.h>

#include "DBif_QueryEnum.hpp"
#include "Logger.hpp"
#include "SampleDefinitionDLL.hpp"
#include "SampleSet.hpp"
#include "uuid__t.hpp"

struct SampleSetDLL
{
public:
	//*****************************************************************************
	SampleSetDLL ()
	{
		uuid = {};
		userUuid = {};
		index = 0;
		carrier = eCarrierType::eCarousel;
		samplesetIdNum = 0;
	}

	//*****************************************************************************
	SampleSetDLL (const SampleSet& ss)
	{
		uuid = ss.uuid;
		index = ss.index;
		timestamp = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(ss.timestamp);
		carrier = ss.carrier;
		DataConversion::convert_listOfCType_to_vecOfDllType (ss.samples, ss.numSamples, samples, true);
		DataConversion::convertToStandardString (name, ss.name);
		DataConversion::convertToStandardString (username, ss.username);
		status = DBApi::eSampleSetStatus::SampleSetNotRun;
	}

	//*****************************************************************************
	SampleSet ToCStyle()
	{
		SampleSet ss = {};

		ss.uuid = uuid;
		ss.index = index;
		ss.timestamp = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(timestamp);
		ss.carrier = carrier;
		DataConversion::convert_vecOfDllType_to_listOfCType (samples, ss.samples, ss.numSamples);
		DataConversion::convertToCharPointer (ss.name, name);
		DataConversion::convertToCharPointer (ss.username, username);
		ss.status = status;

		return ss;
	}

	//*****************************************************************************
	DBApi::DB_SampleSetRecord ToDbStyle() const
	{
		DBApi::DB_SampleSetRecord dbss;

		dbss.RunDateTP = timestamp;
		dbss.SampleSetIdNum = samplesetIdNum;
		dbss.SampleSetId = uuid;
		dbss.SampleSetStatus = static_cast<int32_t>(status);
		dbss.SampleSetNameStr = name;
		dbss.SampleSetLabel = {};	// Not currently used.
		dbss.Comments = {};			// Not currently used.
		dbss.OwnerId = userUuid;
		dbss.OwnerNameStr = username;
		dbss.WorklistId = worklistUuid;
		dbss.CarrierType = static_cast<uint16_t>(carrier);

		dbss.SampleItemCount = ( int16_t ) samples.size();
		dbss.ProcessedItemCount = 0;

		for (auto& v : samples)
		{
			SampleDefinitionDLL sdefdll = v;
			DBApi::DB_SampleItemRecord dbsi = sdefdll.ToDbStyle();
			if ( dbsi.SampleItemStatus == static_cast<int32_t>(DBApi::eSampleItemStatus::ItemComplete) )
			{
				dbss.ProcessedItemCount++;
			}

			dbss.SSItemsList.push_back( dbsi );
		}

		return dbss;
	}

	//*****************************************************************************
	void FromDbStyle (const DBApi::DB_SampleSetRecord& dbss)
	{
		timestamp = dbss.RunDateTP;
		samplesetIdNum = dbss.SampleSetIdNum;
		uuid = dbss.SampleSetId;
		status = static_cast<DBApi::eSampleSetStatus>(dbss.SampleSetStatus);
		name = dbss.SampleSetNameStr;
		userUuid = dbss.OwnerId;
		username = dbss.OwnerNameStr;
		worklistUuid = dbss.WorklistId;		

		index = 0; // UI will set this.

//TODO: DB_SampleSetRecord does not contain a carrier type...
		carrier = eCarrierType::eCarousel;

		for (auto& v : dbss.SSItemsList)
		{
			SampleDefinitionDLL sdDLL = {};
			sdDLL.FromDbStyle (v);
			samples.push_back (sdDLL);
		}
	}


	uuid__t worklistUuid;	// Filled in when stored in DB if part of a Worklist,
							// otherwise is set from current Worklist uuid.
	uuid__t uuid;			// Filled in when stored in DB.
	uint16_t index;			// Data structure positional info for UI to be able to update the correct sample.
	system_TP timestamp;	// Time sample was written to the DB.
	eCarrierType carrier;
	std::vector<SampleDefinitionDLL> samples;
	std::string name;
	std::string username;
	uuid__t userUuid;
	std::string setLabel;	// This is the Tag field.
	DBApi::eSampleSetStatus status;
	int64_t samplesetIdNum;	// Updated for use by DB.  Not used in the backend code.

	void PlateSort(ePrecession precession)
	{
		if (precession == eRowMajor)
		{
			std::sort(samples.begin(), samples.end(), sortSamplesByPlateRow());
		}
		else
		{
			std::sort(samples.begin(), samples.end(), sortSamplesByPlateColumn());
		}
	}
	
	//*****************************************************************************
	// Sort by row, then column.
	//*****************************************************************************
	struct sortSamplesByPlateRow
	{
		bool operator() (const SampleDefinitionDLL& sampleA, const SampleDefinitionDLL& sampleB) const
		{
			if (sampleA.position.getRow() != sampleB.position.getRow())
			{
				return (sampleA.position.getRow() < sampleB.position.getRow());
			}
			return (sampleA.position.getColumn() < sampleB.position.getColumn());
		}
	};

	//*****************************************************************************
	// Sort by column, then row.
	//*****************************************************************************
	struct sortSamplesByPlateColumn
	{
		bool operator() (const SampleDefinitionDLL& sampleA, const SampleDefinitionDLL& sampleB) const
		{
			if (sampleA.position.getColumn() != sampleB.position.getColumn())
			{
				return (sampleA.position.getColumn() < sampleB.position.getColumn());
			}
			return (sampleA.position.getRow() < sampleB.position.getRow());
		}
	};

	//*****************************************************************************
	static void Log (std::stringstream& ss, std::string title, const SampleSetDLL& sampleSet) {
		if (Logger::L().IsOfInterest(severity_level::debug1)) {
			ss << title;
			for (size_t i = 0; i < sampleSet.samples.size(); i++) {
				SampleDefinitionDLL::Log (ss, boost::str(boost::format("Sample[%d]") % i), sampleSet.samples[i]);
			}
		}
	}
};

