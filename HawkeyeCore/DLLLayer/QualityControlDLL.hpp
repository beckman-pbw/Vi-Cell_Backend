#pragma once

#include <string>

#include "CellTypesDLL.hpp"
#include "ChronoUtilities.hpp"
#include "DataConversion.hpp"
#include "QualityControl.hpp"

class QualityControlDLL
{
public:
	QualityControlDLL() { uuid = {}; }
	QualityControlDLL (QualityControl_t qc)
	{
		uuid = {};
		DataConversion::convertToStandardString(qc_name, qc.qc_name);
		cell_type_index = qc.cell_type_index;
		assay_type = qc.assay_type;
		DataConversion::convertToStandardString(lot_information, qc.lot_information);
		assay_value = qc.assay_value;
		plusminus_percentage = qc.plusminus_percentage;
		expiration_date = qc.expiration_date;
		DataConversion::convertToStandardString(comment_text, qc.comment_text);
		retired = false;		
	}

	QualityControl_t ToCStyle()
	{
		QualityControl_t qc = {};

		DataConversion::convertToCharPointer(qc.qc_name, qc_name);
		qc.cell_type_index = cell_type_index;
		qc.assay_type = assay_type;
		DataConversion::convertToCharPointer(qc.lot_information, lot_information);
		qc.assay_value = assay_value;
		qc.plusminus_percentage = plusminus_percentage;
		qc.expiration_date = expiration_date;
		DataConversion::convertToCharPointer(qc.comment_text, comment_text);

		return qc;
	}

	//*****************************************************************************
	DBApi::DB_QcProcessRecord ToDbStyle() const
	{
		DBApi::DB_QcProcessRecord dbQC = {};

		dbQC.QcId = uuid;
		dbQC.QcName = qc_name;
		dbQC.QcType = static_cast<uint16_t>(assay_type);
		dbQC.CellTypeIndex = cell_type_index;
		CellTypeDLL celltypeDLL = CellTypesDLL::getCellTypeByIndex (cell_type_index);
		dbQC.CellTypeId = celltypeDLL.uuid;
		//dbQC.CellTypeId = cell_type_index;	//TODO: for future use when QCs are referenced by UUID...
		dbQC.LotInfo = lot_information;
		system_TP expDate = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(expiration_date * SECONDSPERDAY);
		dbQC.LotExpiration = ChronoUtilities::ConvertToString (expDate, "%Y-%m-%d");
		dbQC.AssayValue = assay_value;
		dbQC.AllowablePercentage = plusminus_percentage;
		dbQC.Comments = comment_text;
		dbQC.Retired = retired;

		return dbQC;
	}

	void FromDbStyle (const DBApi::DB_QcProcessRecord& dbQCRecord)
	{
		uuid = dbQCRecord.QcId;	//TODO: for future use...
		qc_name = dbQCRecord.QcName;
		cell_type_index = dbQCRecord.CellTypeIndex;
		assay_type = static_cast<assay_parameter>(dbQCRecord.QcType);
		lot_information = dbQCRecord.LotInfo;
		assay_value = dbQCRecord.AssayValue;
		plusminus_percentage = dbQCRecord.AllowablePercentage;
		system_TP expDate = ChronoUtilities::ConvertToTimePoint (dbQCRecord.LotExpiration, "%Y-%m-%d %H:%M:%S");
		expiration_date = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(expDate) / SECONDSPERDAY;
		comment_text = dbQCRecord.Comments;
		retired = dbQCRecord.Retired;
	}

	// Expiration : An item expires as soon as the current date is greater than what is configured.
	// Ex: a QC with a stated expiration of 12/31/2020 is valid on 12/31/2020, but expired on 1/1/2021
	bool isExpired()
	{
		boost::gregorian::date today = boost::gregorian::day_clock::local_day();
		boost::gregorian::date local_epoch = boost::gregorian::from_simple_string("1970-01-01");
		boost::gregorian::days now_days = today - local_epoch;

		return now_days.days() > this->expiration_date;
	}

	static std::string UpdateQualityControl (const QualityControlDLL& inputQC, QualityControlDLL& currentQC);
	std::string ToString();

	std::string qc_name;
	uuid__t uuid;
	uint32_t cell_type_index;
	assay_parameter assay_type;
	std::string lot_information;
	double assay_value;
	double plusminus_percentage;
	uint64_t expiration_date; // days since 1/1/1970 local; UI provides this value in "days" not "seconds".
	std::string comment_text;
	bool retired;
};
