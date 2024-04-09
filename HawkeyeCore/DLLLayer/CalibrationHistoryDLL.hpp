#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "CalibrationHistory.hpp"
#include "ChronoUtilities.hpp"
#include "DataConversion.hpp"
#include "SystemStatus.hpp"
#include "UserList.hpp"
#include "uuid__t.hpp"

class CalibrationConsumableDLL
{
public:
	CalibrationConsumableDLL() {}
	CalibrationConsumableDLL(calibration_consumable cc)
	{
		DataConversion::convertToStandardString(label, cc.label);
		DataConversion::convertToStandardString(lotId, cc.lot_id);
		expirationDate = cc.expiration_date * SECONDSPERDAY;
		assayValue = cc.assay_value;
	}

	calibration_consumable ToCStyle()
	{
		calibration_consumable cc = {};

		DataConversion::convertToCharPointer(cc.label, label);
		DataConversion::convertToCharPointer(cc.lot_id, lotId);
		cc.expiration_date = (uint32_t)(expirationDate / SECONDSPERDAY);
		cc.assay_value = assayValue;

		return cc;
	}

	DBApi::cal_consumable_t ToDbStyle()
	{
		DBApi::cal_consumable_t consumable = {};

		consumable.label = label;
		consumable.lot_id = lotId;
		consumable.expiration_date = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(expirationDate);
		consumable.assay_value = static_cast<float>(assayValue);

		return consumable;
	}

	void FromDbStyle (DBApi::cal_consumable_t& consumable)
	{
		label = consumable.label;
		lotId = consumable.lot_id;
		expirationDate = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(consumable.expiration_date);
		assayValue = static_cast<double>(consumable.assay_value);
	}

	std::string label;
	std::string lotId;
	uint64_t	expirationDate; // days since 1/1/1970
	double		assayValue;
};

class CalibrationDataDLL
{
public:
	CalibrationDataDLL() {}
	CalibrationDataDLL (calibration_history_entry ch)
	{
		timestamp = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(ch.timestamp);
		DataConversion::convertToStandardString (username, ch.username);
		//		UserList::Instance().GetUserUUID (username, uuid);
		uuid = {};		// uuid is the calibration record uuid, not user uuid...
		type = ch.cal_type;
		imageCount = ch.image_count;
		DataConversion::convert_listOfCType_to_vecOfDllType (ch.consumable_list, ch.num_consumables, calibrationConsumables);
		slope = ch.slope;
		intercept = ch.intercept;
		worklistUUID = ch.uuid;
		instrumentSN = SystemStatus::Instance().systemVersion.system_serial_number;
	}

	calibration_history_entry ToCStyle()
	{
		calibration_history_entry ch = {};

		ch.timestamp = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(timestamp);
		DataConversion::convertToCharPointer (ch.username, username);
		ch.cal_type = type;
		ch.image_count = imageCount;
		DataConversion::convert_vecOfDllType_to_listOfCType (calibrationConsumables, ch.consumable_list, ch.num_consumables);
		ch.slope = slope;
		ch.intercept = intercept;
		ch.uuid = worklistUUID;

		return ch;
	}

	DBApi::DB_CalibrationRecord ToDbStyle()
	{
		DBApi::DB_CalibrationRecord dbCalibrationRecord = {};

		dbCalibrationRecord.CalId = uuid;
		dbCalibrationRecord.CalDate = timestamp;
		UserList::Instance().GetUserUUIDByName(username, dbCalibrationRecord.CalUserId);
		dbCalibrationRecord.CalType = type;
		dbCalibrationRecord.Slope = slope;
		dbCalibrationRecord.Intercept = intercept;
		dbCalibrationRecord.ImageCnt = static_cast<uint16_t>(imageCount);
		dbCalibrationRecord.QueueId = worklistUUID;
		dbCalibrationRecord.InstrumentSNStr = instrumentSN;

		for (auto& v : calibrationConsumables)
		{
			dbCalibrationRecord.ConsumablesList.push_back (v.ToDbStyle());
		}

		return dbCalibrationRecord;
	}

	void FromDbStyle (DBApi::DB_CalibrationRecord& dbCalibrationRecord)
	{
		uuid = dbCalibrationRecord.CalId;
		timestamp = dbCalibrationRecord.CalDate;
		UserList::Instance().GetUsernameByUUID (dbCalibrationRecord.CalUserId, username);
		type = static_cast<calibration_type>(dbCalibrationRecord.CalType);
		slope = dbCalibrationRecord.Slope;
		intercept = dbCalibrationRecord.Intercept;
		imageCount = dbCalibrationRecord.ImageCnt;
		worklistUUID = dbCalibrationRecord.QueueId;
		instrumentSN = dbCalibrationRecord.InstrumentSNStr;

		calibrationConsumables = {};

		for (auto& v : dbCalibrationRecord.ConsumablesList)
		{
			CalibrationConsumableDLL ccDLL = {};
			ccDLL.FromDbStyle (v);
			calibrationConsumables.push_back (ccDLL);
		}
	}

	uuid__t         uuid = {};
	system_TP		timestamp;
	std::string		username;
	calibration_type type;
	double			slope;
	double			intercept;
	uint32_t		imageCount;
	uuid__t         worklistUUID = {};
	std::string		instrumentSN;
	std::vector<CalibrationConsumableDLL> calibrationConsumables;
};

class CalibrationHistoryDLL
{
public:
	static CalibrationHistoryDLL& Instance()
	{
		static CalibrationHistoryDLL instance;
		return instance;
	}

	CalibrationHistoryDLL() {};
	virtual ~CalibrationHistoryDLL() {};
	
	bool Initialize();
	bool Add (CalibrationDataDLL& cdDLL);
	std::vector<CalibrationDataDLL>& CalibrationHistoryDLL::Get();

	HawkeyeError SetConcentrationCalibration (CalibrationDataDLL& newDLL);
	HawkeyeError SetConcentrationCalibration_ScaleFromDefault(calibration_type cal_type, double scaling);

	uint32_t GetConcentrationImageControlCount();
	void GetCurrentConcentrationAndSizeCalibration (
		calibration_type calType,
		double& concentrationIntercept,
		double& concentrationSlope,
		uint32_t& imageControlCount,
		double& sizeIntercept,
		double& sizeSlope);
	void GetLastCalibratedDates (uint64_t& concDate, uint64_t& sizeDate, uint64_t& acupConcDate);
	void Import (boost::property_tree::ptree& ptConfig);
	bool IsCurrentConcentrationCalibrationValid() const;
	bool IsCurrentSizeCalibrationValid() const;
	HawkeyeError ClearCalibrationData (calibration_type cal_type, uint64_t clear_prior_to_time, bool clearAllACupData=false);
	bool LoadCurrentCalibration();
	bool LoadDefaultCalibration();

private:
	void CreateDefaultDatabaseEntries();

	std::vector<CalibrationDataDLL> calibrationDataDLL_;
	CalibrationDataDLL currentConcentrationCalibration_;
	CalibrationDataDLL currentSizeCalibration_;
	CalibrationDataDLL currentACupConcentrationCalibration_;

	CalibrationDataDLL defaultConcentrationCalibration_;
	CalibrationDataDLL defaultSizeCalibration_;
	CalibrationDataDLL defaultACupConcentrationCalibration_;
	// ACup uses the same size calibration as Carousel and Plate.
};
