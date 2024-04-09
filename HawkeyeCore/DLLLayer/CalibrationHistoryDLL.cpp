#include "stdafx.h"

#include "AppConfig.hpp"
#include "AuditLog.hpp"
#include "CalibrationHistoryDLL.hpp"
#include "ChronoUtilities.hpp"
#include "EnumConversion.hpp"
#include "InstrumentConfig.hpp"
#include "UserList.hpp"

static const char MODULENAME[] = "CalibrationHistory";
static const double DEFAULT_ACUP_SLOPE = 369.00;

//*****************************************************************************
// The calibration code has been reworked.There are now two factory default entries (one for concentration and one for size).
// New entries are appended to the CalibrationHistory.info file.
// When get the most recent entries, the vector containing the calibration history is searched in reverse.
// This results in using the most recent entries for conc / size.
// When clearing the history, the factory defaults are maintained.

//*****************************************************************************
static void loadCalibration (
	boost::property_tree::basic_ptree<std::string, std::string>::assoc_iterator& assoc_it,
	CalibrationDataDLL& calibrationData)
{
	auto timestamp = assoc_it->second.get<uint64_t>("timestamp");
	calibrationData.timestamp = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(timestamp);
	calibrationData.username = assoc_it->second.get<std::string>("user_id");
	calibrationData.type = (calibration_type)assoc_it->second.get<uint16_t>("type");
	calibrationData.slope = assoc_it->second.get<double>("slope");
	calibrationData.intercept = assoc_it->second.get<double>("intercept");
	calibrationData.imageCount = assoc_it->second.get<uint32_t>("image_count");
	auto str = assoc_it->second.get<std::string>("queue_id");
	calibrationData.worklistUUID = Uuid::FromStr (str.c_str());

	auto calibConsumRange = assoc_it->second.equal_range("consumable");
	for (auto it = calibConsumRange.first; it != calibConsumRange.second; ++it) {
		CalibrationConsumableDLL calibConsumable;
		calibConsumable.label = it->second.get<std::string>("label");
		calibConsumable.lotId = it->second.get<std::string>("lot_id");

		// Legacy data handling.  We truly want to store DAYS since 1/1/1970, not SECONDS
		// Things do not expire on that scale.
		if (it->second.find("expiration_date_days") == it->second.not_found())
		{
			calibConsumable.expirationDate = it->second.get<uint64_t>("expiration_date") / SECONDSPERDAY;
		}
		else
		{
			calibConsumable.expirationDate = it->second.get<uint64_t>("expiration_date_days") * SECONDSPERDAY;
		}

		calibConsumable.assayValue = it->second.get<double>("assay_value");

		calibrationData.calibrationConsumables.push_back(calibConsumable);
	}
}

//*****************************************************************************
static bool writeConsumableEntryHistory (CalibrationConsumableDLL& cc, boost::property_tree::ptree& pt_CalibrationEntry) {

	boost::property_tree::ptree pt_Consumable;
	pt_Consumable.put ("label", cc.label);
	pt_Consumable.put ("lot_id", cc.lotId);
	//	pt_Consumable.put ("expiration_date", std::to_string (cc.expirationDate));
	pt_Consumable.put("expiration_date_days", cc.expirationDate);
	pt_Consumable.put ("assay_value", cc.assayValue);

	pt_CalibrationEntry.add_child ("consumable", pt_Consumable);

	return true;
}

//*****************************************************************************
bool CalibrationHistoryDLL::LoadCurrentCalibration() {

	Logger::L().Log (MODULENAME, severity_level::debug2, "loadCurrentCalibration: <enter>");

	// Find the most recent calibration concentration/size entries.
	std::vector<CalibrationDataDLL>::reverse_iterator it;
	bool foundConcentration = false;
	bool foundSize = false;
	bool foundACupConcentration = false;

	currentConcentrationCalibration_.type = cal_UNKNOWN;
	currentSizeCalibration_.type = cal_UNKNOWN;
	currentACupConcentrationCalibration_.type = cal_UNKNOWN;

	for (it = calibrationDataDLL_.rbegin(); it != calibrationDataDLL_.rend(); ++it) {
		if (it->type == calibration_type::cal_Concentration && !foundConcentration) {
			currentConcentrationCalibration_ = *it;
			Logger::L().Log (MODULENAME, severity_level::normal,
				boost::str(boost::format("loadCurrentCalibration: concentration intercept/slope set (%.2f / %.2f)") 
					% currentConcentrationCalibration_.slope % currentConcentrationCalibration_.intercept));
			foundConcentration = true;
			continue;
		}

		if (it->type == calibration_type::cal_Size && !foundSize) {
			currentSizeCalibration_ = *it;
			Logger::L().Log (MODULENAME, severity_level::normal,
				boost::str(boost::format("loadCurrentCalibration: size intercept/slope set (%.2f / %.2f)") 
					% currentSizeCalibration_.slope % currentSizeCalibration_.intercept));
			foundSize = true;
			continue;
		}

		if (it->type == calibration_type::cal_ACupConcentration && !foundACupConcentration) {
			currentACupConcentrationCalibration_ = *it;
			Logger::L().Log (MODULENAME, severity_level::normal,
				boost::str(boost::format("loadCurrentCalibration: Acup concentration intercept/slope set (%.2f / %.2f)") 
					% currentACupConcentrationCalibration_.slope % currentACupConcentrationCalibration_.intercept));
			foundACupConcentration = true;
			continue;
		}
	}

	std::string loggedInUsername = UserList::Instance().GetLoggedInUsername();

	// If we could not find both calibrations then we're done.
	if (currentConcentrationCalibration_.type == calibration_type::cal_UNKNOWN) {
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_concentrationinterceptnotset,
			"Value not set"));
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_concentrationslopenotset,
			"Value not set"));
		Logger::L().Log (MODULENAME, severity_level::warning, "loadCurrentCalibration: concentration intercept/slope NOT set");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_precondition_notmet,
			instrument_error::instrument_precondition_instance::concentration_config,
			instrument_error::severity_level::error));
	}

	if (currentSizeCalibration_.type == calibration_type::cal_UNKNOWN) {
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_sizeinterceptnotset,
			"Value not set"));
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_sizeslopenotset,
			"Value not set"));
		Logger::L().Log (MODULENAME, severity_level::error, "loadCurrentCalibration: size intercept/slope NOT set");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_precondition_notmet,
			instrument_error::instrument_precondition_instance::size_config,
			instrument_error::severity_level::error));
	}


	// If we could not find both calibrations then we're done.
	if (currentACupConcentrationCalibration_.type == calibration_type::cal_UNKNOWN) {

		currentACupConcentrationCalibration_ = defaultACupConcentrationCalibration_;
		currentACupConcentrationCalibration_.type = calibration_type::cal_ACupConcentration;

		DBApi::DB_InstrumentConfigRecord instConfig = InstrumentConfig::Instance().Get();
		if (instConfig.ACupEnabled)
		{
			Logger::L().Log (MODULENAME, severity_level::warning,
				boost::str(boost::format("loadCurrentCalibration: ACup using defaulted concentration intercept/slope (%.2f / %.2f)") 
					% currentACupConcentrationCalibration_.slope % currentACupConcentrationCalibration_.intercept));
		}
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, "loadCurrentCalibration: <exit>");

	return true;
}


//*****************************************************************************
bool CalibrationHistoryDLL::LoadDefaultCalibration() {

	Logger::L().Log(MODULENAME, severity_level::debug2, "loadDefaultCalibration: <enter>");

	// Find the earliest calibration concentration/size entries.
	std::vector<CalibrationDataDLL>::iterator it;
	bool foundConcentration = false;
	bool foundSize = false;
	bool foundACupConcentration = false;

	// Clear the initial values
	defaultConcentrationCalibration_.type = calibration_type::cal_UNKNOWN;
	defaultACupConcentrationCalibration_.type = calibration_type::cal_UNKNOWN;
	defaultSizeCalibration_.type = calibration_type::cal_UNKNOWN;

	for (it = calibrationDataDLL_.begin(); it != calibrationDataDLL_.end(); ++it) {
		if (it->type == calibration_type::cal_Concentration && !foundConcentration) {
			defaultConcentrationCalibration_ = *it;
			Logger::L().Log(MODULENAME, severity_level::normal, 
				boost::str(boost::format("loadDefaultCalibration: concentration intercept/slope set (%.2f / %.2f)") 
					% defaultConcentrationCalibration_.slope % defaultConcentrationCalibration_.intercept));
			foundConcentration = true;
			continue;
		}

		if (it->type == calibration_type::cal_Size && !foundSize) {
			defaultSizeCalibration_ = *it;
			Logger::L().Log(MODULENAME, severity_level::normal,
				boost::str(boost::format("loadDefaultCalibration: size intercept/slope set (%.2f / %.2f)") 
					% defaultSizeCalibration_.slope % defaultSizeCalibration_.intercept));
			foundSize = true;
			continue;
		}

		if (it->type == calibration_type::cal_ACupConcentration && !foundACupConcentration) {
			defaultACupConcentrationCalibration_ = *it;
			Logger::L().Log(MODULENAME, severity_level::normal,
				boost::str(boost::format("loadDefaultCalibration: ACup concentration intercept/slope set (%.2f / %.2f)") 
					% defaultACupConcentrationCalibration_.slope % defaultACupConcentrationCalibration_.intercept));
			foundACupConcentration = true;
			continue;
		}
	}

	std::string loggedInUsername = UserList::Instance().GetLoggedInUsername();

	// If we could not find both calibrations then we're done.
	if (defaultConcentrationCalibration_.type == calibration_type::cal_UNKNOWN) {
		AuditLogger::L().Log(generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_concentrationinterceptnotset,
			"Value not set"));
		AuditLogger::L().Log(generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_concentrationslopenotset,
			"Value not set"));
		Logger::L().Log(MODULENAME, severity_level::warning, "loadDefaultCalibration: concentration intercept/slope NOT set");
		ReportSystemError::Instance().ReportError(BuildErrorInstance(
			instrument_error::instrument_precondition_notmet,
			instrument_error::instrument_precondition_instance::concentration_config,
			instrument_error::severity_level::error));
	}

	if (defaultSizeCalibration_.type == calibration_type::cal_UNKNOWN) {
		AuditLogger::L().Log(generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_sizeinterceptnotset,
			"Value not set"));
		AuditLogger::L().Log(generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_sizeslopenotset,
			"Value not set"));
		Logger::L().Log(MODULENAME, severity_level::error, "loadDefaultCalibration: size intercept/slope NOT set");
		ReportSystemError::Instance().ReportError(BuildErrorInstance(
			instrument_error::instrument_precondition_notmet,
			instrument_error::instrument_precondition_instance::size_config,
			instrument_error::severity_level::error));
	}


	// If we could not find both calibrations then we're done.
	if (defaultACupConcentrationCalibration_.type == calibration_type::cal_UNKNOWN) {

		// Defalt for A-cup will be identical to the default record for standard, just with a differet slope
		defaultACupConcentrationCalibration_ = defaultConcentrationCalibration_;
		defaultACupConcentrationCalibration_.type = calibration_type::cal_ACupConcentration;
		defaultACupConcentrationCalibration_.slope = DEFAULT_ACUP_SLOPE;

		DBApi::DB_InstrumentConfigRecord instConfig = InstrumentConfig::Instance().Get();
		if (instConfig.ACupEnabled)
		{
			Logger::L().Log(MODULENAME, severity_level::warning, 
				boost::str(boost::format("loadDefaultCalibration: ACup default not found in DB; using intercept/slope (%.2f/%.2f)") 
					% defaultACupConcentrationCalibration_.slope % defaultACupConcentrationCalibration_.intercept));;
		}
	}

	Logger::L().Log(MODULENAME, severity_level::debug2, "loadDefaultCalibration: <exit>");

	return true;
}

//*****************************************************************************
bool CalibrationHistoryDLL::Initialize()
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "Initialize: <enter>");

	std::vector<DBApi::DB_CalibrationRecord> dbCalibrationDLL_;
	DBApi::eQueryResult dbStatus = DBApi::DbGetCalibrationList(
		dbCalibrationDLL_,
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
	if (DBApi::eQueryResult::QueryOk == dbStatus)
	{
		for (auto& v : dbCalibrationDLL_)
		{
			CalibrationDataDLL cd;
			cd.FromDbStyle (v);
			calibrationDataDLL_.push_back (cd);
		}
	}
	else
	{
		if (DBApi::eQueryResult::NoResults != dbStatus)
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("Initialize: <exit, DbGetCalibrationList failed, status: %ld>") % (int32_t)dbStatus));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_writeerror,
				instrument_error::instrument_storage_instance::signatures,
				instrument_error::severity_level::error));
			return false;
		}

		calibrationDataDLL_.clear();

		CreateDefaultDatabaseEntries();

	} // End "if (DBApi::eQueryResult::QueryOk != dbStatus)"



	if (!LoadDefaultCalibration())
	{
		Logger::L().Log(MODULENAME, severity_level::warning, "Initialize: Failed to load default calibrations");
		return false;
	}
	if (!LoadCurrentCalibration()) {
		Logger::L().Log(MODULENAME, severity_level::warning, "Initialize: Failed to load current calibrations");
		return false;
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, "Initialize: <exit>");

	return true;
}

//*****************************************************************************
void CalibrationHistoryDLL::CreateDefaultDatabaseEntries()
{
	CalibrationDataDLL cdDLL = {};

	CalibrationConsumableDLL consumableDLL = {};

	consumableDLL.label = "Factory Default";
	consumableDLL.lotId = "n/a";
	consumableDLL.expirationDate = 4102444800;
	consumableDLL.assayValue = 2.0;
	cdDLL.calibrationConsumables.push_back (consumableDLL);

	consumableDLL.label = "Factory Default";
	consumableDLL.lotId = "n/a";
	consumableDLL.expirationDate = 4102444800;
	consumableDLL.assayValue = 4.0;
	cdDLL.calibrationConsumables.push_back (consumableDLL);

	consumableDLL.label = "Factory Default";
	consumableDLL.lotId = "n/a";
	consumableDLL.expirationDate = 4102444800;
	consumableDLL.assayValue = 10.0;
	cdDLL.calibrationConsumables.push_back (consumableDLL);

	// Create default stage-controller concentration slope
	cdDLL.timestamp = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(1514764800);
	cdDLL.username = "factory_admin";	
	cdDLL.type = calibration_type::cal_Concentration;
	cdDLL.slope = 368;
	cdDLL.intercept = 0;
	cdDLL.imageCount = 100;
	cdDLL.worklistUUID = {};	// Defaults are set to zeroes.
	Add (cdDLL);

	// Create default a-cup concentration slope
	cdDLL.type = calibration_type::cal_ACupConcentration;
	cdDLL.slope = DEFAULT_ACUP_SLOPE;
	Uuid::Clear(cdDLL.uuid);
	Add(cdDLL);

	// Only need to update the fields that change.
	cdDLL.type = calibration_type::cal_Size;
	cdDLL.slope = 0.53;
	cdDLL.intercept = -5.53;
	cdDLL.imageCount = 0;
	Uuid::Clear(cdDLL.uuid);
	Add (cdDLL);
}

//*****************************************************************************
std::vector<CalibrationDataDLL>& CalibrationHistoryDLL::Get()
{
	return calibrationDataDLL_;
}

//*****************************************************************************
void CalibrationHistoryDLL::GetLastCalibratedDates (uint64_t& concDate, uint64_t& sizeDate, uint64_t& acupConcDate) {

	concDate = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(currentConcentrationCalibration_.timestamp);
	sizeDate = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(currentSizeCalibration_.timestamp);
	acupConcDate = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(currentACupConcentrationCalibration_.timestamp);
}

//*****************************************************************************
void CalibrationHistoryDLL::GetCurrentConcentrationAndSizeCalibration (
	calibration_type calType, 
	double& concentrationIntercept, 
	double& concentrationSlope, 
	uint32_t& imageControlCount,
	double& sizeIntercept,
	double& sizeSlope)
{

	CalibrationDataDLL calData;
	
	if (calType == calibration_type::cal_Concentration)
	{
		calData = currentConcentrationCalibration_;
	}
	else if (calType == calibration_type::cal_ACupConcentration)
	{
		calData = currentACupConcentrationCalibration_;
	}
	//NOTE: size calibration is never set, uses derived values from characterization.

	concentrationIntercept = calData.intercept;
	concentrationSlope = calData.slope;
	imageControlCount = calData.imageCount;
	sizeIntercept = currentSizeCalibration_.intercept;
	sizeSlope = currentSizeCalibration_.slope;
}

//*****************************************************************************
uint32_t CalibrationHistoryDLL::GetConcentrationImageControlCount() {

	return currentConcentrationCalibration_.imageCount;
}

//*****************************************************************************
bool CalibrationHistoryDLL::Add (CalibrationDataDLL& cdDLL)
{
	DBApi::DB_CalibrationRecord dbCalibrationRecord = cdDLL.ToDbStyle();

	DBApi::eQueryResult dbStatus = DBApi::DbAddCalibration (dbCalibrationRecord);
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("Add: <exit, DbAddCalibration failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::calibration,
			instrument_error::severity_level::error));
		return false;
	}

	// update the record uuid for DB identification...
	cdDLL.uuid = dbCalibrationRecord.CalId;

	calibrationDataDLL_.push_back (cdDLL);

	return true;
}

//*****************************************************************************
HawkeyeError CalibrationHistoryDLL::SetConcentrationCalibration (CalibrationDataDLL& newConcentration)
{
	if (!Add(newConcentration))
	{
		return HawkeyeError::eDatabaseError;
	}

	CalibrationDataDLL oldConcentration;
	if (newConcentration.type == calibration_type::cal_Concentration)
	{
		oldConcentration = currentConcentrationCalibration_;
		currentConcentrationCalibration_ = newConcentration;
	}
	else
	{
		oldConcentration = currentACupConcentrationCalibration_;
		currentACupConcentrationCalibration_ = newConcentration;
	}

	std::string loggedInUsername = UserList::Instance().GetLoggedInUsername();

	AuditLogger::L().Log (generateAuditWriteData(
		loggedInUsername,
		audit_event_type::evt_concentrationinterceptset,
		boost::str(boost::format("Concentration Intercept Set\n\tNew Value: %.2f\n\tPrevious Value: %.2f") 
			% newConcentration.intercept
			% oldConcentration.intercept)));
	AuditLogger::L().Log (generateAuditWriteData(
		loggedInUsername,
		audit_event_type::evt_concentrationslopeset, 
		boost::str(boost::format("Concentration Slope Set\n\tNew Value: %.2f\n\tPrevious Value: %.2f") 
			% newConcentration.slope
			% oldConcentration.slope)));

	return HawkeyeError::eSuccess;
}

HawkeyeError CalibrationHistoryDLL::SetConcentrationCalibration_ScaleFromDefault(calibration_type cal_type, double scaling)
{
	// Get the default for this type
	CalibrationDataDLL newConcentration, oldConcentration;
	switch (cal_type)
	{
	case calibration_type::cal_ACupConcentration:
		newConcentration = defaultACupConcentrationCalibration_; 
		oldConcentration = defaultACupConcentrationCalibration_;
		break;
	case calibration_type::cal_Concentration:
		newConcentration = defaultConcentrationCalibration_;
		oldConcentration = defaultConcentrationCalibration_;
		break;
	default:
		return HawkeyeError::eInvalidArgs;
	}

	// Create a new calibration - scale the slope based on the value
	newConcentration.slope *= scaling;
	newConcentration.username = UserList::Instance().GetLoggedInUsername();
	Uuid::Clear(newConcentration.uuid);
	newConcentration.timestamp = ChronoUtilities::CurrentTime();

	// Add the new calibration
	if (!Add(newConcentration))
	{
		return HawkeyeError::eDatabaseError;
	}


	// update the current
	switch (cal_type)
	{
	case calibration_type::cal_ACupConcentration:
		currentACupConcentrationCalibration_ = newConcentration;
		break;
	case calibration_type::cal_Concentration:
		currentConcentrationCalibration_ = newConcentration;
		break;
	}

	// Add a log entry
	AuditLogger::L().Log(generateAuditWriteData(
		newConcentration.username,
		audit_event_type::evt_concentrationinterceptset,
		boost::str(boost::format("%sConcentration Intercept Set (scaling)\n\tNew Value: %.2f\n\tPrevious Value: %2.f")
			% (cal_type == calibration_type::cal_ACupConcentration ? "A-cup ": "")
			% newConcentration.intercept
			% oldConcentration.intercept)));
	AuditLogger::L().Log(generateAuditWriteData(
		newConcentration.username,
		audit_event_type::evt_concentrationslopeset,
		boost::str(boost::format("%sConcentration Slope Set (scaling)\n\tNew Value: %.2f\n\tPrevious Value: %.2f")
			% (cal_type == calibration_type::cal_ACupConcentration ? "A-cup " : "")
			% newConcentration.slope
			% oldConcentration.slope)));

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
// imports v1.2 data...
void CalibrationHistoryDLL::Import (boost::property_tree::ptree& ptConfig)
{
	auto calibrationDataRange = ptConfig.equal_range("calibration_entry");
	for (auto it = calibrationDataRange.first; it != calibrationDataRange.second; ++it)
	{
		CalibrationDataDLL cdDLL;
		loadCalibration (it, cdDLL);

		auto item = std::find_if (calibrationDataDLL_.begin(), calibrationDataDLL_.end(),
			[cdDLL](const auto& item) { 
				return item.timestamp == cdDLL.timestamp; 
			});

		// Only add calibration data if it doesn't already exist.
		if (item == calibrationDataDLL_.end())
		{
			Add (cdDLL);
		}
		else
		{
			Logger::L().Log(MODULENAME, severity_level::normal,
				boost::str (boost::format ("Import: duplicate calibration data found: %d>") 
					% ChronoUtilities::ConvertToString (item->timestamp)));
		}
	}
}

//*****************************************************************************
bool CalibrationHistoryDLL::IsCurrentConcentrationCalibrationValid() const
{
	return currentConcentrationCalibration_.type != calibration_type::cal_UNKNOWN
		&& currentConcentrationCalibration_.calibrationConsumables.empty() == false;
}

//*****************************************************************************
bool CalibrationHistoryDLL::IsCurrentSizeCalibrationValid() const
{
	return currentSizeCalibration_.type != calibration_type::cal_UNKNOWN
		&& currentSizeCalibration_.calibrationConsumables.empty() == false;
}

//*****************************************************************************
HawkeyeError CalibrationHistoryDLL::ClearCalibrationData (calibration_type cal_type, uint64_t clear_prior_to_time, bool clearAllACupData)
{
	std::vector<CalibrationDataDLL> calibrationDataToKeep;
	uint64_t factoryDefaultTime{ UINT64_MAX };


	std::map<calibration_type, bool> first_entries;

	// Assemble the list of log items to retain.
	for (auto& entry : calibrationDataDLL_)
	{
		/*
			Retain IF
			- first instance of any given type
			- type not specified for deletion
			- timestamp after cutoff time
		*/

		// Keep the first entry of any type.
		if (first_entries.find(entry.type) == first_entries.end())
		{
			// Retain.
			first_entries[entry.type] = true;
			calibrationDataToKeep.push_back(entry);
			continue;
		}

		// If not clearing ALL types, Keep all entries that do not match the specific cal_type being cleared.
		if (cal_type != calibration_type::cal_All && entry.type != cal_type)
		{
			// Retain.
			calibrationDataToKeep.push_back(entry);
			continue;
		}

		// Check to keep all factory defaults and entries later than the time to clear.
		auto timeStamp = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(entry.timestamp);
		if (timeStamp == factoryDefaultTime || timeStamp > clear_prior_to_time)
		{
			// Retain.
			calibrationDataToKeep.push_back (entry);
		}
	}

	// Retain most recent log entries
	std::map<calibration_type, bool> last_entries;
	for (auto it = calibrationDataDLL_.rbegin(); it != calibrationDataDLL_.rend(); ++it)
	{
		// Check for first entry of any type
		if (last_entries.find( it->type ) == last_entries.end())
		{
			// Skip if the most recent entry is already pushed for retention
			for (auto& local_it : calibrationDataToKeep)
			{
				if (local_it.timestamp == it->timestamp)
				{
					continue;
				}
			}

			// Retain.
			last_entries[it->type] = true;
			calibrationDataToKeep.push_back( *it );
			continue;
		}
	}

	
	// Compare the original list to the keep list.
	// If the cal_type and timestamp don't match what is in the keep list, then delete it.
	auto iter = calibrationDataDLL_.begin();
	while (iter != calibrationDataDLL_.end())
	{
		auto item = std::find_if(calibrationDataToKeep.begin(), calibrationDataToKeep.end(),
			[iter](const auto& entry)
			{
				return iter->timestamp == entry.timestamp && iter->type == entry.type;
			});
		
		if (item == calibrationDataToKeep.end())
		{
			// Delete the unmatched CalibrationData.
			DBApi::eQueryResult dbStatus = DBApi::DbRemoveCalibrationByUuid (iter->uuid);
			if (DBApi::eQueryResult::QueryOk != dbStatus)
			{
				Logger::L().Log(MODULENAME, severity_level::error,
					boost::str(boost::format("ClearCalibrationData: <exit, DbRemoveCalibrationByUuid failed, status: %ld>") % (int32_t)dbStatus));
				ReportSystemError::Instance().ReportError(BuildErrorInstance(
					instrument_error::instrument_storage_deleteerror,
					instrument_error::instrument_storage_instance::calibration,
					instrument_error::severity_level::error));

				return HawkeyeError::eDatabaseError;
			}
// Keep for debugging
//			Logger::L().Log( MODULENAME, severity_level::debug1,
//							 boost::str( boost::format( "ClearCalibrationData: DbRemoveCalibrationByUuid: %s>" ) % Uuid::ToStr( iter->uuid ) ) );
			iter = calibrationDataDLL_.erase( iter );
		}
		else
		{
			++iter;
		}
	}

	return HawkeyeError::eSuccess;
}
