#include "stdafx.h"

#include "QualityControlDLL.hpp"

static const char MODULENAME[] = "QualityControl";

//*****************************************************************************
std::string QualityControlDLL::ToString()
{
	uint8_t tabCount = 0;

	std::string celltypename = CellTypesDLL::getCellTypeName (cell_type_index);

	std::string str = boost::str(boost::format(
		"QC Name: %s"
		"%sUUID: %s"
		"%s%s (%ld)"
		"%sassay_type: %d"
		"%slot_information: %s"
		"%sassay_value: %.2f"
		"%splusminus_percentage: %f"
		"%sexpiration_date: %s"
		"%scomment_text: %s"
		"%sretired: %s")
		% qc_name
		% SetIndentation(tabCount + 1)
		% Uuid::ToStr(uuid)
		% SetIndentation(tabCount + 1)
		% (celltypename.empty() ? "CellType: INVALID" : ("CellType: \"" + celltypename + "\""))
		% cell_type_index
		% SetIndentation(tabCount + 1)
		% (int)assay_type
		% SetIndentation(tabCount + 1)
		% lot_information
		% SetIndentation(tabCount + 1)
		% assay_value
		% SetIndentation(tabCount + 1)
		% plusminus_percentage
		% SetIndentation(tabCount + 1)
		% ChronoUtilities::ConvertToString (ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(expiration_date))
		% SetIndentation(tabCount + 1)
		% comment_text
		% SetIndentation(tabCount + 1)
		% (retired ? "true" : "false"));
	
	return str;
}

////*****************************************************************************
std::string QualityControlDLL::UpdateQualityControl (const QualityControlDLL& inputQC, QualityControlDLL& currentQC)
{
	std::stringstream ss;

	Logger::L().Log (MODULENAME, severity_level::debug3, "Updating QualityControl...");

	if (inputQC.qc_name != currentQC.qc_name) {
		ss << boost::str (boost::format ("Name: %s -> %s\n") % currentQC.qc_name % inputQC.qc_name);
		currentQC.qc_name = inputQC.qc_name;
	}
	if (inputQC.cell_type_index != currentQC.cell_type_index) {
		ss << boost::str (boost::format ("CellTypeIndex: %ld -> %ld\n") % currentQC.cell_type_index % inputQC.cell_type_index);
		currentQC.cell_type_index = inputQC.cell_type_index;
	}
	if (inputQC.assay_type != currentQC.assay_type) {
		ss << boost::str (boost::format ("AssayType: %d -> %d\n") % std::to_string(currentQC.assay_type) % std::to_string(inputQC.assay_type));
		currentQC.assay_type = inputQC.assay_type;
	}
	if (inputQC.lot_information != currentQC.lot_information) {
		ss << boost::str (boost::format ("LotInformation: %s -> %s\n") % currentQC.lot_information % inputQC.lot_information);
		currentQC.lot_information = inputQC.lot_information;
	}
	if (inputQC.assay_value != currentQC.assay_value) {
		ss << boost::str (boost::format ("AssayValue: %3.2f -> %3.2f\n") % currentQC.assay_value % inputQC.assay_value);
		currentQC.assay_value = inputQC.assay_value;
	}
	if (inputQC.plusminus_percentage != currentQC.plusminus_percentage) {
		ss << boost::str (boost::format ("+- Percentage: %3lf -> %3lf\n") % currentQC.plusminus_percentage % inputQC.plusminus_percentage);
		currentQC.plusminus_percentage = inputQC.plusminus_percentage;
	}

	if (inputQC.expiration_date != currentQC.expiration_date) {
		ss << boost::str (boost::format ("ExpirationDate: %s -> %s\n") 
			% ChronoUtilities::ConvertToString (ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(currentQC.expiration_date))
			% ChronoUtilities::ConvertToString (ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(inputQC.expiration_date)));
		currentQC.expiration_date = inputQC.expiration_date;
	}

	if (inputQC.comment_text != currentQC.comment_text) {
		ss << boost::str (boost::format ("SharpnessLimit: %s -> %s\n") % currentQC.comment_text % inputQC.comment_text);
		currentQC.comment_text = inputQC.comment_text;
	}
	
	//NOTE: "retired" field is not checked since it cannot be changed by a user.

	if (ss.str().empty())
		return "";

	DBApi::DB_QcProcessRecord dbQcProcessRecord = currentQC.ToDbStyle();
	DBApi::DB_QcProcessRecord qcr = {};

	DBApi::eQueryResult dbStatus = DBApi::DbFindQcProcessByUuid(qcr, inputQC.uuid);
	if (dbStatus == DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log(MODULENAME, severity_level::normal, "UpdateQualityControl: successfully found QC by UUID");

		// Updating a CellType now forces the existing CellType to be "retired" and a new user-defined CellType created.		
		dbQcProcessRecord.Retired = true;
		dbQcProcessRecord.QcId = qcr.QcId;

		dbStatus = DBApi::DbModifyQcProcess(dbQcProcessRecord);
		if (DBApi::eQueryResult::QueryOk != dbStatus)
		{
			ReportSystemError::Instance().ReportError(BuildErrorInstance(
				instrument_error::instrument_storage_writeerror,
				instrument_error::instrument_storage_instance::celltype,
				instrument_error::severity_level::warning));

			Logger::L().Log(MODULENAME, severity_level::error,
				boost::str(boost::format("UpdateQualityControl: <exit, unable to modify QC, status: %ld>") % (int32_t)dbStatus));
		}
		else
		{
			Logger::L().Log(MODULENAME, severity_level::normal, "UpdateQualityControl: successfuly modified QC");
		}
	}
	else
	{
		Logger::L().Log(MODULENAME, severity_level::error,
			boost::str(boost::format("UpdateQualityControl: <exit, unable to find QC by UUID, status: %ld>") % (int32_t)dbStatus));
	}

	return ss.str();
}
