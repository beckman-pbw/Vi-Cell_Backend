#include "stdafx.h"

#include "AuditLog.hpp"
#include "CellTypesDLL.hpp"
#include "HawkeyeDirectory.hpp"
#include "QualityControlsDLL.hpp"
#include "SetIndentation.hpp"
#include "UserList.hpp"

static std::vector<QualityControlDLL> qualityControlsDLL_;

static const char MODULENAME[] = "QualityControls";
static bool isInitialized = false;

//*****************************************************************************
std::vector<QualityControlDLL>& QualityControlsDLL::Get()
{
	return qualityControlsDLL_;
}

//*****************************************************************************
static void loadQualityControl (boost::property_tree::basic_ptree<std::string, std::string>::assoc_iterator& it, QualityControlDLL& qc)
{
	qc.uuid = {};
	qc.qc_name = it->second.get<std::string>("qc_name");
	qc.cell_type_index = it->second.get<uint32_t>("cell_type_index");
	qc.assay_type = (assay_parameter)it->second.get<std::uint16_t>("assay_type");
	qc.lot_information = it->second.get<std::string>("lot_information");
	qc.assay_value = it->second.get<double>("assay_value");
	qc.plusminus_percentage = it->second.get<double>("plusminus_percentage");

	// Legacy data handling.  We truly want to store DAYS since 1/1/1970, not SECONDS
	// Things do not expire on that scale.
	if (it->second.find("expiration_date_days") == it->second.not_found())
	{
		qc.expiration_date = it->second.get<uint64_t>("expiration_date") / SECONDSPERDAY;
	}
	else
	{
		qc.expiration_date = it->second.get<uint64_t>("expiration_date_days");
	}

	qc.comment_text = it->second.get<std::string>("comment_text");
}

//*****************************************************************************
static void qualityControlToPtree (const QualityControlDLL& qc, boost::property_tree::ptree& pt_qualityControl)
{
	pt_qualityControl.put("qc_name", qc.qc_name);
	pt_qualityControl.put("cell_type_index", qc.cell_type_index);
	pt_qualityControl.put("assay_type", (std::uint16_t)qc.assay_type);
	pt_qualityControl.put("lot_information", qc.lot_information);
	pt_qualityControl.put("assay_value", qc.assay_value);
	pt_qualityControl.put("plusminus_percentage", qc.plusminus_percentage);
	//	pt_qualityControl.put("expiration_date", ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(qc.expiration_date));
	pt_qualityControl.put("expiration_date_days", qc.expiration_date);
	pt_qualityControl.put("comment_text", qc.comment_text);
}

//*****************************************************************************
std::pair<std::string, boost::property_tree::ptree> QualityControlsDLL::Export()
{
	boost::property_tree::ptree pt_qualityControls = {};
	for each (auto item in qualityControlsDLL_)
	{
		boost::property_tree::ptree pt_qc;
		qualityControlToPtree (item, pt_qc);
		pt_qualityControls.add_child("quality_control", pt_qc);
	}

	return std::make_pair ("quality_controls", pt_qualityControls);
}

//*****************************************************************************
void QualityControlsDLL::Import (boost::property_tree::ptree& ptParent) {

	Logger::L().Log(MODULENAME, severity_level::debug1, "Import: <enter>");

	Initialize();

	// If the "quality_controls" tag is found then this data is coming from 
	// the v1.2 QualityControl.info file whose parent is the "config" tag
	// <or> a v1.2 configuration file.
	auto pQualityControls = ptParent.get_child_optional ("quality_controls");
	if (pQualityControls) {
		ptParent = pQualityControls.get();
	}

	auto qualityControl = ptParent.equal_range("quality_control");
	for (auto it = qualityControl.first; it != qualityControl.second; ++it)
	{
		HawkeyeError status;
		QualityControlDLL qcDLL = {};

		loadQualityControl (it, qcDLL);
		if ((status = Add ("import", qcDLL)) != HawkeyeError::eSuccess)
		{
			if (status == HawkeyeError::eAlreadyExists)
			{
				Logger::L().Log(MODULENAME, severity_level::normal,
					boost::str (boost::format ("Import: duplicate qualitycontrol found: %s>") % qcDLL.qc_name));
			}
		}
	}

	Logger::L().Log(MODULENAME, severity_level::debug1, "Import: <exit>");
}

//*****************************************************************************
bool QualityControlsDLL::Initialize()
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "Initialize: <enter>");

	if (isInitialized)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "Initialize: <exit, already initialized>");
		return true;
	}

	std::vector<DBApi::DB_QcProcessRecord> dbQualityControlsDLL_;
	DBApi::eQueryResult dbStatus = DBApi::DbGetQcProcessList(
		dbQualityControlsDLL_,
		DBApi::eListFilterCriteria::NoFilter,
		"",
		"",
		-1,
		DBApi::eListSortCriteria::SortNotDefined,
		DBApi::eListSortCriteria::SortNotDefined,
		0,
		"",
		-1,
		-1,
		"");
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		if (DBApi::eQueryResult::NoResults != dbStatus)
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("Initialize: <exit, DbGetQcProcessList failed, status: %ld>") % (int32_t)dbStatus));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_writeerror,
				instrument_error::instrument_storage_instance::signatures,
				instrument_error::severity_level::error));
			return false;
		}

		qualityControlsDLL_.clear();

		return true;

	} // End "if (DBApi::eQueryResult::QueryOk != dbStatus)"

	// QualityControl definitions were found in DB, convert to QualityControlDLL.
	for (auto& v : dbQualityControlsDLL_)
	{
		QualityControlDLL qcDLL = {};
		qcDLL.FromDbStyle (v);
		qualityControlsDLL_.push_back (qcDLL);
	}

	isInitialized = true;

	Logger::L().Log (MODULENAME, severity_level::debug2, "Initialize: <exit>");

	return true;
}

//*****************************************************************************
HawkeyeError QualityControlsDLL::Add (std::string username, QualityControlDLL& qualityControlDLL)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "Add: <enter>");

	//TODO : Validate incoming quality control
	//TODO : Validate correctness of the quality control.Known cell type ? Sensible date ? Etc. ?

	auto qcItem = std::find_if (qualityControlsDLL_.begin(), qualityControlsDLL_.end(),
		[&qualityControlDLL](const QualityControlDLL& qcDLL) -> bool
		{
			return qcDLL.qc_name == qualityControlDLL.qc_name;
		});

	if (qcItem != qualityControlsDLL_.end())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "Add: <exit, already exists>");
		return HawkeyeError::eAlreadyExists;
	}

	std::string qcName = qualityControlDLL.qc_name;

	// Check if the QC name is the same as any CellType name.
	// Do not allow a QC to have the same name as a CellType.
	std::vector<CellTypeDLL>& celltypesDLL = CellTypesDLL::Get();
	auto ctItem = std::find_if (celltypesDLL.begin(), celltypesDLL.end(),
		[&qualityControlDLL](const CellTypeDLL& ctDLL) -> bool
		{
			return ctDLL.label == qualityControlDLL.qc_name;
		});

	if (ctItem != celltypesDLL.end())
	{
		if (username == "import")
		{ // During import if the QC and CellType names are the same, append "_QC" to the end of the QC name.
			qualityControlDLL.qc_name += "_*";
		}
		else
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "Add: <exit, QC name cannot be the same as CellType>");
			return HawkeyeError::eEntryInvalid;
		}
	}

	DBApi::DB_QcProcessRecord dbQCRecord = qualityControlDLL.ToDbStyle();

	DBApi::DB_CellType qcCellType = {};
	DBApi::eQueryResult dbStatus = DBApi::DbFindCellTypeByUuid( qcCellType, dbQCRecord.CellTypeId );
	if ( DBApi::eQueryResult::QueryOk != dbStatus )
	{
		Logger::L().Log( MODULENAME, severity_level::debug1, "Add: <exit, cell type not found>" );
		return HawkeyeError::eInvalidArgs;
	}
	else if ( qcCellType.Retired )
	{
		Logger::L().Log( MODULENAME, severity_level::debug1, "Add: <exit, cell type not valid>" );
		return HawkeyeError::eInvalidArgs;
	}

	dbStatus = DBApi::DbAddQcProcess(dbQCRecord);
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		Logger::L().Log(MODULENAME, severity_level::error,
			boost::str(boost::format("Add: <exit, DbAddQcProcess failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError(BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::qualitycontrol,
			instrument_error::severity_level::warning));
		return HawkeyeError::eDatabaseError;
	}

	qualityControlDLL.uuid = dbQCRecord.QcId;

	qualityControlsDLL_.push_back(qualityControlDLL);

	AuditLogger::L().Log (generateAuditWriteData(
		username,
		audit_event_type::evt_qcontrolcreate,
		qualityControlDLL.ToString()));

	Logger::L().Log (MODULENAME, severity_level::debug1, "Add: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
bool getQualityControlByName (const std::string qcName, QualityControlDLL& qc)
{
	for (const auto& v : qualityControlsDLL_)
	{
		if (qcName == v.qc_name)
		{
			qc = v;
			return true;
		}
	}

	return false;
}

//*****************************************************************************
bool getQualityControlByUUID (const uuid__t uuid, QualityControlDLL& qc)
{
	for (const auto& v : qualityControlsDLL_)
	{
		if (Uuid(uuid) == Uuid(v.uuid))
		{
			qc = v;
			return true;
		}
	}

	return false;
}

//*****************************************************************************
QcStatus IsSampleWithinQCLimits (
	const std::string& bpQcName,
	const uuid__t& qcUuid,
	const CellCounterResult::SResult& sresult)
{
	QcStatus qcStatus = QcStatus::eNotApplicable;

	// If this sample is using a QC check if the sample meets the QC acceptance criteria.
	if (!Uuid::IsClear(qcUuid))
	{
		std::string bp_qc_name;
		QualityControlDLL qc = {};
		getQualityControlByName (bpQcName, qc);

		/*
			Pass/Fail will depend on which type of QC we have (concentration/viability/size…),
			the assay value for the QC used and the +/- window established around that QC.
			Example: a Concentration QC would look at the overall concentration for the sample,
			and then give it a pass/fail based on whether that value is within the +/- tolerance
			around the QC’s assay value.
		*/

		/*
		 * 3/25/2022
		 * [2:23 PM] Mills, Phil (Phillip) M
		 * QC:
		 * Concentration: checked vs the whole set of cells counted (average size of all cells detected)
		 * Size: checked vs. the whole set of cell counted (concentration of all cells detected)
		 * Viability: checked vs the percentage of viable ("of interest") cells detected. (ratio of "of interest"/viable within the entire ("general") population.
		 */
		constexpr double TenToThe6th = 1000000;
		std::string assayTypeStr;
		double limit = 0;
		double minValue = 0;
		double maxValue = 0;
		double dataValue = 0;

		switch (qc.assay_type)
		{
			case ap_Concentration:
			{
				// "qc.assay_value" is 10 to the sixth.
				assayTypeStr = "Concentration";
				dataValue = sresult.Cumulative_results.dCellConcentration_GP;
				limit = qc.assay_value * TenToThe6th * (qc.plusminus_percentage / 100.0);
				minValue = qc.assay_value * TenToThe6th - limit;
				maxValue = qc.assay_value * TenToThe6th + limit;
				break;
			}
			case ap_PopulationPercentage: // (percent viability)
			{
				// "qc.assay_value" is a percentage here.
				assayTypeStr = "Population Percentage";
				dataValue = sresult.Cumulative_results.dPopulationPercentage;
				limit = qc.assay_value * (qc.plusminus_percentage / 100.0);
				minValue = qc.assay_value - limit;
				maxValue = qc.assay_value + limit;
				break;
			}
			case ap_Size:
			{
				// "qc.assay_value" is in microns (um).
				assayTypeStr = "Size";
				dataValue = sresult.Cumulative_results.dAvgDiameter_GP;
				limit = qc.assay_value * (qc.plusminus_percentage / 100.0);
				minValue = qc.assay_value - limit;
				maxValue = qc.assay_value + limit;
				break;
			}
		}

		std::string statusStr;
		if (dataValue >= minValue && dataValue <= maxValue)
		{
			statusStr = "is in range";
			qcStatus = QcStatus::ePass;
		}
		else
		{
			statusStr = "out of range";
			qcStatus = QcStatus::eFail;
		}

		Logger::L().Log (MODULENAME, severity_level::normal,
			boost::str (boost::format ("%s, assay_value: %5.2lf, acceptance_limit %%: %.2lf, data_value: %.6lf, limit: %.6lf, range: %.6lf to %.6lf, %s")
				% assayTypeStr
				% qc.assay_value
				% qc.plusminus_percentage
				% dataValue
				% limit
				% minValue
				% maxValue
				% statusStr));

	} // End "if (!Uuid::IsClear(sampleDef.parameters.qc_uuid))"

	return qcStatus;
}
