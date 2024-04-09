#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <boost/format.hpp>

#include <DBif_Api.h>

#include "AnalysisDefinitionDLL.hpp"
#include "ChronoUtilities.hpp"
#include "GetAsStrFunctions.hpp"
#include "Logger.hpp"
#include "QualityControlDLL.hpp"
#include "ReportsCommon.hpp"
#include "SamplePositionDLL.hpp"
#include "SampleDefinitionDLL.hpp"
#include "SampleParametersDLL.hpp"
#include "uuid__t.hpp"
#include "UserList.hpp"


/// Sample Parameters
///  This structure represents a single "sample" in the system.
///  All members with the name "____Index" are indices into the appropriate list of items
///   (ex: "celltypeIndex" is an index identifying a particular item in the system-wide list of 
///    cell types.)
///  A sample may select 1 or 2 reagents which will be mixed at a ratio of 1:1 (or (0.5 + 0.5):1 if two reagents are selected).
///  A sample may select 0 or more fluorescent illumination sources (Hunter).  The Brightfield source will always be used.
///  Within a queue, the following must be true at the time it is submitted for processing:
///   - sd locations are not duplicated
///   - All Indices must be valid (Valid: within range of the system-wide list AND 
///      where applicable, valid for the currently-logged-in user
///      (ex: If sd.celltypeIndex is '4', then '4' must be a valid index into the system-wide celltype list
///       AND the logged-in-user must have '4' in their list of allowed celltypes)

bool getQualityControlByName (const std::string qcName, QualityControlDLL& qc);

struct SampleDefinitionDLL
{
	//*****************************************************************************
	SampleDefinitionDLL()
	{
		sampleSetUuid = {};
		sampleDefUuid = {};
		sampleDataUuid = {};
		timestamp = {};
		position = {};
		status = eSampleStatus::eNotProcessed;
		parameters = {};
		runUserID = {};
		maxImageCount = boost::none;  /*(uint16_t)(0);*/
	}

	//*****************************************************************************
	//Note: This conversion function will not convert cellTypeIndex and analysisIndex in "SampleDefinition" TO
	// CellTypeDLL and AnalysisDefinitionDLL in SampleDefinitionDLL.
	//*****************************************************************************
	SampleDefinitionDLL (const SampleDefinition& sd) : SampleDefinitionDLL()
	{
		sampleSetUuid = sd.sampleSetUuid;
		sampleDefUuid = sd.sampleDefUuid;
		sampleDataUuid = sd.sampleDataUuid;
		index = sd.index;
		sampleSetIndex = sd.sampleSetIndex;
		DataConversion::convertToStandardString (username, sd.username);
		timestamp = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(sd.timestamp);
		position = SamplePositionDLL(sd.position);
		status = sd.status;
		parameters = sd.parameters;
		UserList::Instance().GetUserUUIDByName(username, runUserID);
	}

	//*****************************************************************************
	SampleDefinition ToCStyle()
	{
		SampleDefinition sd = {};

		sd.sampleSetUuid = sampleSetUuid;
		sd.sampleDefUuid = sampleDefUuid;
		sd.sampleDataUuid = sampleDataUuid;
		sd.index = index;
		sd.sampleSetIndex = sampleSetIndex;
		DataConversion::convertToCharPointer (sd.username, username);
		sd.timestamp = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(timestamp);
		sd.position = position.getSamplePosition();
		sd.status = status;
		sd.parameters = parameters.ToCStyle();
				
		return sd;
	}

	//*****************************************************************************
	DBApi::DB_SampleItemRecord ToDbStyle() const
	{
		DBApi::DB_SampleItemRecord si = {};

		si.SampleItemIdNum = 0;
		si.SampleSetId = sampleSetUuid;
		si.SampleItemId = sampleDefUuid;
		si.SampleId = sampleDataUuid;
		si.RunDateTP = timestamp;
		si.OwnerNameStr = username;
		si.SampleItemStatus = SampleItemStatusToDbStyle (status);

		si.SampleItemNameStr = parameters.label;
		si.ItemLabel = parameters.tag;
		si.Dilution = ( int16_t ) parameters.dilutionFactor;
		si.WashTypeIndex = static_cast<uint16_t>(parameters.postWash);
		si.SaveNthImage = (int16_t)parameters.saveEveryNthImage;

		si.ImageAnalysisParamId = {};	//TODO: remove, not used...

		DBApi::DB_AnalysisDefinitionRecord dbadef = parameters.analysis.ToDbStyle();
		si.AnalysisDefIndex = parameters.analysis.analysis_index;
		si.AnalysisDefId = parameters.analysis.uuid;

		si.AnalysisParamId = {};

		DBApi::DB_CellTypeRecord dbct = parameters.celltype.ToDbStyle();
		si.CellTypeIndex = parameters.celltype.celltype_index;
		si.CellTypeId = parameters.celltype.uuid;

		si.QcProcessNameStr = parameters.bp_qc_name;
		si.QcProcessId = parameters.qc_uuid;

//TODO:		si.WorkflowId = ;

		si.SampleRow = position.getRow();
		si.SampleCol = (uint8_t)position.getColumn();
		si.RotationCount = 0;	//TODO: so far we're not keeping track of carousel rotations.

		return si;
	}

	//*****************************************************************************
	static const int32_t SampleItemStatusToDbStyle (eSampleStatus status)
	{
		switch (status)
		{
			case eNotProcessed:
				return static_cast<int32_t>(DBApi::eSampleItemStatus::ItemNotRun);

			case eInProcess_Aspirating:		// several states of "in process"
			case eInProcess_Mixing:
			case eInProcess_ImageAcquisition:
			case eInProcess_Cleaning:
			case eAcquisition_Complete:
				return static_cast<int32_t>(DBApi::eSampleItemStatus::ItemRunning);

			case eCompleted:
				return static_cast<int32_t>(DBApi::eSampleItemStatus::ItemComplete);

			case eSkip_Error:
				return static_cast<int32_t>(DBApi::eSampleItemStatus::ItemSkipped);

			case eSkip_Manual:
			case eSkip_NotProcessed:		// For samples that have not been processed when a SampleSet is cancelled.
				return static_cast<int32_t>(DBApi::eSampleItemStatus::ItemCanceled);

			default:
				return static_cast<int32_t>(DBApi::eSampleItemStatus::NoItemStatus);
		}
	}

	//*****************************************************************************
	void FromDbStyle (const DBApi::DB_SampleItemRecord& dbsi)
	{
		sampleSetUuid = dbsi.SampleSetId;
		sampleDefUuid = dbsi.SampleItemId;
		sampleDataUuid = dbsi.SampleId;
		username = dbsi.OwnerNameStr;

		// The UI will assign these fields.
		index = 0;
		sampleSetIndex = 0;

		timestamp = dbsi.RunDateTP;

		position.setRowColumn (dbsi.SampleRow, dbsi.SampleCol);

		switch (static_cast<DBApi::eSampleItemStatus>(dbsi.SampleItemStatus))
		{
			default:
			case DBApi::eSampleItemStatus::ItemNotRun:
				status = eNotProcessed;
				break;
			case DBApi::eSampleItemStatus::ItemComplete:
				status = eCompleted;
				break;
			case DBApi::eSampleItemStatus::ItemCanceled:
				status = eSkip_Manual;
				break;
			case DBApi::eSampleItemStatus::ItemSkipped:
				status = eSkip_Error;
				break;
		}

		parameters.FromDbStyle (dbsi);
	}

	uuid__t sampleSetUuid;		// belongs to the owning sample-set containing this sample definition=
	uuid__t sampleDefUuid;		// This should belong to the resulting sample object, once the sample is acquired/processed.
	uuid__t sampleDataUuid;		// This should belong to the resulting sample object, once the sample is acquired/processed.
	uint16_t index;				// Data structure positional info for UI to be able to update the correct sample.
	uint16_t sampleSetIndex;	// Data structure positional info for UI to be able to update the correct sample.
	std::string username;
	uuid__t runUserID;			// Whoever hit 'Play' in the UI
	system_TP timestamp;		// Time sample was completed, stored in the DB.
	SamplePositionDLL position;
	eSampleStatus status;
	SampleParametersDLL parameters;
	boost::optional<uint16_t> maxImageCount;

	// Convert to "sample_completion_status" from "eSampleStatus"
	sample_completion_status ToSampleLogCompletionStatus() const
	{
		sample_completion_status to = sample_completion_status::sample_errored;
		switch (status)
		{
			case eSampleStatus::eCompleted:
				to = sample_completion_status::sample_completed;
				break;
			case eSampleStatus::eNotProcessed:
				to = sample_completion_status::sample_not_run;
				break;
			case eSampleStatus::eSkip_Manual:
				to = sample_completion_status::sample_skipped;
				break;
			case eSampleStatus::eSkip_Error:
			default:
				to = sample_completion_status::sample_errored;
		}
		return to;
	}

	// Convert to "eSampleStatus" from "sample_completion_status".
	static eSampleStatus FromSampleLogCompletionStatus (sample_completion_status status)
	{
		eSampleStatus to = eSampleStatus::eSkip_Error;
		switch (status)
		{
			case sample_completion_status::sample_completed:
				to = eSampleStatus::eCompleted;
				break;
			case sample_completion_status::sample_not_run:
				to = eSampleStatus::eNotProcessed;
				break;
			case sample_completion_status::sample_skipped:
				to = eSampleStatus::eSkip_Manual;
				break;
			case sample_completion_status::sample_errored:
			default:
				to = eSampleStatus::eSkip_Error;
		}
		return to;
	}

	//*****************************************************************************
	static void Log (std::stringstream& ss, std::string title, const SampleDefinitionDLL& sd) {
		ss << boost::str (boost::format ("\n\t sampleSetUuid: %s\n") % Uuid::ToStr (sd.sampleSetUuid));
		ss << boost::str (boost::format ("\t sampleDefUuid: %s\n") % Uuid::ToStr (sd.sampleDefUuid));
		ss << boost::str (boost::format ("\tsampleDataUuid: %s\n") % Uuid::ToStr (sd.sampleDataUuid));
		ss << boost::str (boost::format ("\t%s\n") % title);
		ss << boost::str (boost::format ("\tSampleSetIndex: %d, Index: %d\n") % sd.sampleSetIndex % sd.index);
		ss << boost::str (boost::format ("\tPosition: %s\n") % SamplePositionDLL(sd.position).getAsStr());
		ss << boost::str (boost::format ("\tStatus: %s\n") % getSampleStatusAsStr(sd.status));

		SampleParametersDLL::Log (ss, sd.parameters);
	}

	//*****************************************************************************
	static void Log (std::string title, const SampleDefinitionDLL& sd) {
		std::stringstream ss;

		Log (ss, title, sd);

		Logger::L ().Log ("SampleDefinitionDLL", severity_level::debug1, ss.str());
	}

};
