#include "stdafx.h"

#include <fstream>
#include <map>
#include <vector>

#include "FileSystemUtilities.hpp"
#include "HawkeyeDataAccess.h"
#include "Logger.hpp"
#include "MemoryStreamBuf.hpp"
#include "SecurityHelpers.hpp"
#include "SResultBinStorage.hpp"

#define SRESULT_VERSION 1.0
#define SIMAGESTATS_VERSION 1.0
#define SINPUTSETTINGS_VERSION 1.0
#define SBLOBDATA_VERSION 1.0
#define SLARGECLUSTERDATA_VERSION 1.0

static const std::string MODULENAME = "SResultBinStorage";

#define SRESULTBIN_SALT  "Beckman Coulter Life Sciences Loveland CO - Proprietary Cell Counting Data "
#define Repetitions 12


//This API is not currently used
static std::string GenerateHashKey(const std::string& data)
{
	if (data.empty())
		return std::string();

	std::string salt(SRESULTBIN_SALT);
	std::vector<unsigned char> salt_vec(salt.begin(), salt.end());
	SecurityHelpers::RunningSHADigest sha(salt_vec, Repetitions);
	sha.UpdateDigest(data);

	return	sha.FinalizeDigestToHash();
}

//This API is not currently used
static bool VerifyHashKey(std::istream* ifs)
{
	char *hashkey_buf = nullptr;
	char *sresult_bin_data_buf = nullptr;

	try
	{
		if (!ifs->good())
			return false;

		uint32_t hashkey_length = 0;
		// Get the Size of the Bin data, Move the handler to EOF
		ifs->seekg(0L, std::ios::end);
		uint64_t file_size = (uint64_t)ifs->tellg();

		uint16_t sizeof_hashKey_length = (uint16_t)sizeof(hashkey_length);
		// Read the Hash key Size and key
		// Last 4 bytes are length of Hash key
		ifs->seekg(-sizeof_hashKey_length, std::ios::end);
		ifs->read((char*)&hashkey_length, sizeof_hashKey_length);

		// Last "hashkey_length" bytes before  "sizeof_hashKey_length" are Hash data.
		int offset_pos = (hashkey_length + sizeof_hashKey_length);
		ifs->seekg(-offset_pos, std::ios::end);
		hashkey_buf = new char[hashkey_length];
		ifs->read(hashkey_buf, hashkey_length);

		std::string key_to_verify = {};
		key_to_verify.append(hashkey_buf, hashkey_length);

		// Clear the buffer memory;
		delete[] hashkey_buf;
		hashkey_buf = nullptr;

		// If no Hash Key is present then It's a failure
		if (key_to_verify.empty())
			return false;

		uint64_t sresult_data_length = (uint64_t)(file_size - offset_pos);

		ifs->seekg(0L, std::ios::beg);
		sresult_bin_data_buf = new char[sresult_data_length];
		ifs->read((char*)sresult_bin_data_buf, sresult_data_length);
		std::string sresult_bin_data_str = {};
		sresult_bin_data_str.append(sresult_bin_data_buf, sresult_data_length);

		// Clear the buffer memory
		delete[] sresult_bin_data_buf;
		sresult_bin_data_buf = nullptr;

		std::string key_to_verify_against = GenerateHashKey(sresult_bin_data_str);

		if (key_to_verify_against.empty() || key_to_verify != key_to_verify_against)
			return false;

		// Move the Handler to the beginning
		ifs->seekg(0L, std::ios::beg);
	}
	catch (...)
	{
		if (sresult_bin_data_buf)
			delete[] sresult_bin_data_buf;
		if (hashkey_buf)
			delete[] hashkey_buf;

		return false;
	}

	return true;
}

//This API is not currently used
static bool AddHashKey(const std::string& filename)
{
	char * buf = nullptr;
	try
	{
		std::ifstream ifs_temp(filename, std::ios::binary | std::ios::ate);
		ifs_temp.exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);

		if (!ifs_temp.is_open())
			return false;

		uint64_t temp_file_size = ifs_temp.tellg();
		ifs_temp.seekg(0L, std::ios::beg);
		buf = new char[temp_file_size];

		ifs_temp.read(buf, temp_file_size);

		std::string data_to_generate_hashkey = {};
		data_to_generate_hashkey.append(buf, temp_file_size);
		delete[] buf;
		buf = nullptr;

		std::string HashKey = GenerateHashKey(data_to_generate_hashkey);
		if (HashKey.empty())
			return false;

		ifs_temp.close();

		std::ofstream ofs_final(filename, std::ios::binary | std::ios::app | std::ios::ate);
		ofs_final.exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);

		uint32_t HashKey_size = (uint32_t)HashKey.size();
		ofs_final.write(HashKey.c_str(), HashKey_size);
		ofs_final.write((char*)&HashKey_size, sizeof(HashKey_size));
		ofs_final.close();
	}
	catch (...)
	{
		if (buf)
			delete[] buf;
		return false;
	}

	return true;
}

/******************************************************************************/
template<typename vec_data_type>
bool WriteVectorData(std::ostream* ofs, const std::vector<vec_data_type>& vec_data)
{
	try
	{
		if (!ofs->good())
			return false;

		// Write the Size
		uint32_t vec_size = (uint32_t)vec_data.size();
		ofs->write((char*)&vec_size, sizeof(vec_size));
		for (auto vdata : vec_data)
			ofs->write((char*)&vdata, sizeof(vdata));
	}
	catch (...)
	{
		return false;
	}

	return true;
}

template<class map_data_type>
bool WriteMapData(std::ostream* ofs, const map_data_type& map_data)
{
	try
	{
		if (!ofs->good())
			return false;

		// Write the Size
		uint32_t map_size = (uint32_t)map_data.size();
		ofs->write((char*)&map_size, sizeof(map_size));

		for (auto mdata : map_data)
		{
			ofs->write((char*)&mdata.first, sizeof(mdata.first));
			ofs->write((char*)&mdata.second, sizeof(mdata.second));
		}
	}
	catch (...)
	{
		return false;
	}
	return true;
}

static bool WriteStruct(std::ostream* ofs, const CellCounterResult::SInputSettings& ips)
{
	try
	{
		if (!ofs->good())
			return false;

		auto WriteCellIdentiParamLambda = [&](const auto& vec_cip)->void
		{
			uint32_t vec_size = (uint32_t)vec_cip.size();
			ofs->write((char*)&vec_size, sizeof(vec_size));
			for (auto cip : vec_cip)
			{
				auto charcteristic = std::get<0>(cip);
				ofs->write((char*)&std::get<0>(charcteristic), sizeof(std::get<0>(charcteristic)));
				ofs->write((char*)&std::get<1>(charcteristic), sizeof(std::get<1>(charcteristic)));
				ofs->write((char*)&std::get<2>(charcteristic), sizeof(std::get<2>(charcteristic)));

				ofs->write((char*)&std::get<1>(cip), sizeof(std::get<1>(cip)));
				ofs->write((char*)&std::get<2>(cip), sizeof(std::get<2>(cip)));
			}

		};
		float ips_ver = SINPUTSETTINGS_VERSION;
		ofs->write((char*)&ips_ver, sizeof(ips_ver));

		if (!WriteMapData<std::map<ConfigParameters::E_CONFIG_PARAMETERS, double>>(ofs, ips.map_InputConfigParameters))
			return false;
		// GP
		WriteCellIdentiParamLambda(ips.v_CellIdentificationParameters);
		// POI 
		WriteCellIdentiParamLambda(ips.v_POIIdentificationParameters);
	}
	catch (...)
	{
		return false;
	}

	return true;
}

static bool WriteStruct(std::ostream* ofs, const CellCounterResult::SImageStats& is)
{
	try
	{
		if (!ofs->good())
			return false;

		float is_ver = SIMAGESTATS_VERSION;
		ofs->write((char*)&is_ver, sizeof(is_ver));

		ofs->write((char*)&is, sizeof(is));
	}
	catch (...)
	{
		return false;
	}

	return true;

}

static bool WriteStruct(std::ostream* ofs, const Blob::SBlobData& blb)
{
	try
	{
		if (!ofs->good())
			return false;

		float blb_ver = SBLOBDATA_VERSION;
		ofs->write((char*)&blb_ver, sizeof(blb_ver));

		uint32_t map_size = (uint32_t)blb.map_Characteristics.size();
		ofs->write((char*)&map_size, sizeof(map_size));
		for (auto bc : blb.map_Characteristics)
		{
			auto charcteristic = bc.first;
			ofs->write((char*)&std::get<0>(charcteristic), sizeof(std::get<0>(charcteristic)));
			ofs->write((char*)&std::get<1>(charcteristic), sizeof(std::get<1>(charcteristic)));
			ofs->write((char*)&std::get<2>(charcteristic), sizeof(std::get<2>(charcteristic)));
			ofs->write((char*)&bc.second, sizeof(bc.second));
		}
		ofs->write((char*)&blb.oPointCenter, sizeof(blb.oPointCenter));

		if (!WriteVectorData<cv::Point>(ofs, blb.v_oCellBoundaryPoints))
			return false;
	}
	catch (...)
	{
		return false;
	}

	return true;
}

static bool WriteStruct(std::ostream* ofs, const CellCounterResult::SLargeClusterData& lcd)
{
	try
	{
		if (!ofs->good())
			return false;

		float lcd_ver = SLARGECLUSTERDATA_VERSION;
		ofs->write((char*)&lcd_ver, sizeof(lcd_ver));

		ofs->write((char*)&lcd.nNoOfCellsInCluster, sizeof(lcd.nNoOfCellsInCluster));
		if (!WriteVectorData<cv::Point>(ofs, lcd.v_oCellBoundaryPoints))
			return false;
		ofs->write((char*)&lcd.BoundingBox, sizeof(lcd.BoundingBox));
	}
	catch (...)
	{
		return false;
	}

	return true;
}

// Input process settings
static bool WriteParam(std::ostream* ofs, const CellCounterResult::SInputSettings& ps)
{
	return WriteStruct(ofs, ps);
}

// Cumulative Cell counter result structure
static bool WriteParam(std::ostream* ofs, const CellCounterResult::SImageStats& cr)
{
	return WriteStruct(ofs, cr);
}

// Map containing the image statistic result
static bool WriteParam(std::ostream* ofs, const std::map<int, CellCounterResult::SImageStats>& map_is)
{
	try
	{
		if (!ofs->good())
			return false;

		uint32_t map_size = (uint32_t)map_is.size();
		ofs->write((char*)&map_size, sizeof(map_size));

		for (auto is : map_is)
		{
			ofs->write((char*)&is.first, sizeof(is.first));
			if (!WriteStruct(ofs, is.second))
				return false;
		}
	}
	catch (...)
	{
		return false;
	}

	return true;
}

// Map containing the blob information per image
static bool WriteParam(std::ostream* ofs, const std::map<int, std::shared_ptr<std::vector<Blob::SBlobData>>>& map_bfi)
{
	try
	{
		if (!ofs->good())
			return false;

		uint32_t map_size = (uint32_t)map_bfi.size();
		ofs->write((char*)&map_size, sizeof(map_size));
		for (auto bfi : map_bfi)
		{
			ofs->write((char*)&bfi.first, sizeof(bfi.first));

			uint32_t vec_size = (uint32_t)bfi.second->size();
			ofs->write((char*)&vec_size, sizeof(vec_size));
			for (auto blob : *bfi.second)
			{
				if (!WriteStruct(ofs, blob))
					return false;
			}
		}
	}
	catch (...)
	{
		return false;
	}

	return true;
}

// Map containing Maximum number of SubPeaks in each FL Channel
static bool WriteParam(std::ostream* ofs, const std::map<int, std::map<int, int>>& map_fl_ch)
{
	try
	{
		if (!ofs->good())
			return false;

		uint32_t map_size = (uint32_t)map_fl_ch.size();
		ofs->write((char*)&map_size, sizeof(map_size));
		for (auto pfl : map_fl_ch)
		{
			ofs->write((char*)&pfl.first, sizeof(pfl.first));
			if (!WriteMapData<std::map<int, int>>(ofs, pfl.second))
				return false;
		}
	}
	catch (...)
	{
		return false;
	}
	return true;
}

// Large cluster information
static bool WriteParam(std::ostream* ofs, const std::map<int, std::vector<CellCounterResult::SLargeClusterData>>& map_lcd)
{
	try
	{
		if (!ofs->good())
			return false;

		uint32_t map_size = (uint32_t)map_lcd.size();
		ofs->write((char*)&map_size, sizeof(map_size));
		for (auto lcd : map_lcd)
		{
			ofs->write((char*)&lcd.first, sizeof(lcd.first));
			uint32_t vec_size = (uint32_t)lcd.second.size();
			ofs->write((char*)&vec_size, sizeof(vec_size));
			for (auto vec_lcd : lcd.second)
			{
				if (!WriteStruct(ofs, vec_lcd))
					return false;
			}
		}
	}
	catch (...)
	{
		return false;
	}
	return true;
}

// Map containing the Max sub peak for sample dataset
static bool WriteParam(std::ostream* ofs, const std::map<int, int>& map_flch_cum)
{
	if (!ofs->good())
		return false;

	return WriteMapData<std::map<int, int>>(ofs, map_flch_cum);
}

bool SResultBinStorage::SerializeSResult(const CellCounterResult::SResult& sr, std::string filename)
{
	try
	{
		std::string encryptedFilename = {};
		if (!FileSystemUtilities::ValidateFileNameAndPath(filename) ||
			!HDA_GetEncryptedFileName(filename, encryptedFilename))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Invalid SResult Filename or Path : \"" + filename + "\"");
			return false;
		}

		std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
		if (!ofs.is_open())
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Invalid Bin Filename or Path. Failed to create : \"" + filename + "\"");
			return false; 
		}
		// Must set the exceptions
		ofs.exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);

		double version_info = SRESULT_VERSION;
		ofs.write((char*)&version_info, sizeof(version_info));

		//sr.Processing_Settings;
		if (!WriteParam(&ofs, sr.Processing_Settings))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to write Processing_Settings");
			return false;
		}

		//sr.Cumulative_results;
		if (!WriteParam(&ofs, sr.Cumulative_results))	//NOTE: DetailedImageResults
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to write Cumulative_results");
			return false;
		}

		//sr.map_Image_Results;
		if (!WriteParam(&ofs, sr.map_Image_Results))	//NOTE: DetailedImageResults
//TODO: is "int" just the image number.
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to write map_Image_Results");
			return false;
		}

		//sr.map_Blobs_for_image;
		if (!WriteParam(&ofs, sr.map_Blobs_for_image))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to write map_Blobs_for_image");
			return false;
		}

		//sr.map_MaxNoOfPeaksFLChannel;
		if (!WriteParam(&ofs, sr.map_MaxNoOfPeaksFLChannel))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to write map_MaxNoOfPeaksFLChannel");
			return false;
		}

		// sr.map_v_stLargeClusterData;
		if (!WriteParam(&ofs, sr.map_v_stLargeClusterData))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to write map_v_stLargeClusterData");
			return false;
		}

		//sr.map_MaxNoOfPeaksFLChannel_Cumulative;
		if (!WriteParam(&ofs, sr.map_MaxNoOfPeaksFLChannel_Cumulative))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to write map_MaxNoOfPeaksFLChannel_Cumulative");
			return false;
		}

		ofs.flush();
		ofs.close();

		if (!HDA_FileEncrypt (filename.c_str(), encryptedFilename.c_str()))
		{
			std::remove(filename.c_str());
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to encrypt SResult Binary file");
			return false;
		}

		std::remove(filename.c_str());
		//TODO: Perform Compression here
	}
	catch (std::exception &e)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Failed to Serialize the SResult : " + std::string(e.what()));
		return false;
	}

	return true;
}

/******************************************************************************/
template<typename vec_data_type>
bool ReadVectorData(std::istream* ifs, std::vector<vec_data_type>& vec_data)
{
	try
	{
		if (!ifs->good())
			return false;

		// Read the Size
		uint32_t vec_size = 0;
		vec_data.clear();

		ifs->read((char*)&vec_size, sizeof(vec_size));
		vec_data.reserve(vec_size);
		for (uint32_t index = 0; index < vec_size; index++)
		{
			vec_data_type vdata = {};
			ifs->read((char*)&vdata, sizeof(vdata));
			vec_data.push_back(vdata);
		}
	}
	catch (...)
	{
		return false;
	}

	return true;
}

template<class key, class value>
bool ReadMapData(std::istream* ifs, std::map<key, value>& map_data)
{
	try
	{
		if (!ifs->good())
			return false;

		// Read the Size
		uint32_t map_size = 0;
		map_data.clear();
		ifs->read((char*)&map_size, sizeof(map_size));
		for (uint32_t index = 0; index < map_size; index++)
		{
			key first = {};
			value second = {};

			ifs->read((char*)&first, sizeof(first));
			ifs->read((char*)&second, sizeof(second));

			map_data[first] = second;
		}
	}
	catch (...)
	{
		return false;
	}

	return true;
}

static bool ReadStruct(std::istream* ifs, CellCounterResult::SInputSettings& ips)
{
	try
	{
		if (!ifs->good())
			return false;

		auto ReadCellIdentiParamlambda = [&](auto& vec_cip)
		{
			uint32_t vec_size = 0;
			ifs->read((char*)&vec_size, sizeof(vec_size));
			vec_cip.clear();
			vec_cip.reserve(vec_size);
			for (uint32_t index = 0; index < vec_size; index++)
			{
				uint16_t tuple_param1 = 0;
				uint16_t tuple_param2 = 0;
				uint16_t tuple_param3 = 0;
				ifs->read((char*)&tuple_param1, sizeof(tuple_param1));
				ifs->read((char*)&tuple_param2, sizeof(tuple_param2));
				ifs->read((char*)&tuple_param3, sizeof(tuple_param3));

				auto characteristic_tuple = std::make_tuple(tuple_param1, tuple_param2, tuple_param3);
				float cip_val = 0;
				E_POLARITY polarity;
				ifs->read((char*)&cip_val, sizeof(cip_val));
				ifs->read((char*)&polarity, sizeof(polarity));

				vec_cip.emplace_back(characteristic_tuple, cip_val, polarity);
			}
		};


		float ips_ver = 0;
		ifs->read((char*)&ips_ver, sizeof(ips_ver));

		if (ips_ver == SINPUTSETTINGS_VERSION)
		{
			// Param 1
			if (!ReadMapData<ConfigParameters::E_CONFIG_PARAMETERS, double>(ifs, ips.map_InputConfigParameters))
				return false;
			// Param 2
			ReadCellIdentiParamlambda(ips.v_CellIdentificationParameters);
			// Param 3
			ReadCellIdentiParamlambda(ips.v_POIIdentificationParameters);
		}
		else
		{
			return false;
		}
	}
	catch (...)
	{
		return false;
	}

	return true;
}

static bool ReadStruct(std::istream* ifs, CellCounterResult::SImageStats& is)
{
	try
	{
		if (!ifs->good())
			return false;

		float is_ver = 0;
		ifs->read((char*)&is_ver, sizeof(is_ver));

		if (is_ver == SIMAGESTATS_VERSION)
		{
			// Reading all the Params in one go
			ifs->read((char*)&is, sizeof(is));
		}
		else
		{
			return false;
		}
	}
	catch (...)
	{
		return false;
	}

	return true;

}

static bool ReadStruct(std::istream* ifs, Blob::SBlobData& blb)
{
	try
	{
		if (!ifs->good())
			return false;

		float blb_ver = 0;
		ifs->read((char*)&blb_ver, sizeof(blb_ver));

		if (blb_ver == SBLOBDATA_VERSION)
		{
			uint32_t map_size = 0;
			ifs->read((char*)&map_size, sizeof(map_size));

			// Param 1
			for (uint32_t index = 0; index < map_size; index++)
			{
				uint16_t tuple_param1 = 0;
				uint16_t tuple_param2 = 0;
				uint16_t tuple_param3 = 0;
				ifs->read((char*)&tuple_param1, sizeof(tuple_param1));
				ifs->read((char*)&tuple_param2, sizeof(tuple_param2));
				ifs->read((char*)&tuple_param3, sizeof(tuple_param3));

				auto characteristic_tuple_first = std::make_tuple(tuple_param1, tuple_param2, tuple_param3);
				float second = 0;
				ifs->read((char*)&second, sizeof(second));
				
				blb.map_Characteristics[characteristic_tuple_first] = second;
			}
			// Param 2
			ifs->read((char*)&blb.oPointCenter, sizeof(blb.oPointCenter));

			// Param 3
			if (!ReadVectorData<cv::Point>(ifs, blb.v_oCellBoundaryPoints))
				return false;
		}
		else
		{
			return false;
		}
	}
	catch (...)
	{
		return false;
	}

	return true;
}

static bool ReadStruct(std::istream* ifs, CellCounterResult::SLargeClusterData& lcd)
{
	try
	{
		if (!ifs->good())
			return false;

		float lcd_ver = 0;
		ifs->read((char*)&lcd_ver, sizeof(lcd_ver));

		if (lcd_ver == SLARGECLUSTERDATA_VERSION)
		{
			// Param 1
			ifs->read((char*)&lcd.nNoOfCellsInCluster, sizeof(lcd.nNoOfCellsInCluster));
			// Param 2
			if (!ReadVectorData<cv::Point>(ifs, lcd.v_oCellBoundaryPoints))
				return false;
			// Param 3
			ifs->read((char*)&lcd.BoundingBox, sizeof(lcd.BoundingBox));
		}
		else
		{
			return false;
		}
	}
	catch (...)
	{
		return false;
	}

	return true;
}

// Input process settings
static bool ReadParam(std::istream* ifs, CellCounterResult::SInputSettings& ps)
{
	return ReadStruct(ifs, ps);
}

// Cumulative Cell counter result structure
static bool ReadParam(std::istream* ifs, CellCounterResult::SImageStats& cr)
{
	return ReadStruct(ifs, cr);
}

// Map containing the image statistic result
static bool ReadParam(std::istream* ifs, std::map<int, CellCounterResult::SImageStats>& map_ir)
{
	try
	{
		if (!ifs->good())
			return false;

		uint32_t map_size = 0;
		map_ir.clear();

		ifs->read((char*)&map_size, sizeof(map_size));
		for (uint32_t index = 0; index < map_size; index++)
		{
			int first = 0;
			CellCounterResult::SImageStats second = {};
			ifs->read((char*)&first, sizeof(first));
			if (!ReadStruct(ifs, second))
				return false;

			map_ir[first] = second;
		}

	}
	catch (...)
	{
		return false;
	}

	return true;
}

// Map containing the blob information per image
static bool ReadParam(std::istream* ifs, std::map<int, std::shared_ptr<std::vector<Blob::SBlobData>>>& map_bfi)
{
	try
	{
		if (!ifs->good())
			return false;

		uint32_t map_size = 0;
		map_bfi.clear();

		ifs->read((char*)&map_size, sizeof(map_size));
		for (uint32_t index_map = 0; index_map < map_size; index_map++)
		{
			uint32_t first = 0, vec_size = 0;
			ifs->read((char*)&first, sizeof(first));

			std::shared_ptr<std::vector<Blob::SBlobData>> second_blob_vec =
				std::make_shared<std::vector<Blob::SBlobData>>();

			ifs->read((char*)&vec_size, sizeof(vec_size));
			second_blob_vec->resize(vec_size);
			for (uint32_t index_vec = 0; index_vec < vec_size; index_vec++)
			{
				if (!ReadStruct(ifs, second_blob_vec->at(index_vec)) )
					return false;
			}

			map_bfi[first] = second_blob_vec;
		}
	}
	catch (...)
	{
		return false;
	}

	return true;
}

// Map containing Maximum number of SubPeaks in each FL Channel
static bool ReadParam(std::istream* ifs, std::map<int, std::map<int, int>>& map_fl_ch)
{
	try
	{
		if (!ifs->good())
			return false;

		uint32_t map_size = 0;
		map_fl_ch.clear();

		ifs->read((char*)&map_size, sizeof(map_size));
		for (uint32_t index = 0; index < map_size; index++)
		{
			int first = 0;
			std::map<int, int> second_flowCha_map;

			ifs->read((char*)&first, sizeof(first));
			if (!ReadMapData<int, int>(ifs, second_flowCha_map))
				return false;

			map_fl_ch[first] = second_flowCha_map;
		}
	}
	catch (...)
	{
		return false;
	}

	return true;
}

// Large cluster information
static bool ReadParam(std::istream* ifs, std::map<int, std::vector<CellCounterResult::SLargeClusterData>>& map_lcd)
{
	try
	{
		if (!ifs->good())
			return false;

		uint32_t map_size = 0;
		map_lcd.clear();
		ifs->read((char*)&map_size, sizeof(map_size));
		for (uint32_t index_map = 0; index_map < map_size; index_map++)
		{
			int first = 0, vec_size = 0;
			ifs->read((char*)&first, sizeof(first));

			std::vector<CellCounterResult::SLargeClusterData> second_lc_vec;
			ifs->read((char*)&vec_size, sizeof(vec_size));
			second_lc_vec.resize(vec_size);
			for (int index_vec = 0; index_vec < vec_size; index_vec++)
			{
				if (!ReadStruct(ifs, second_lc_vec[index_vec]))
					return false;
			}

			map_lcd[first] = second_lc_vec;
		}
	}
	catch (...)
	{
		return false;
	}

	return true;
}

// Map containing the Max sub peak for sample dataset
static bool ReadParam(std::istream* ifs, std::map<int, int>& map_flch_cum)
{
	if (!ifs->good())
		return false;

	return ReadMapData<int, int>(ifs, map_flch_cum);
}

bool SResultBinStorage::DeSerializeSResult(CellCounterResult::SResult& sresult, std::string filename)
{
	try
	{
		std::vector<byte> decryptedData;
		if (!HDA_ReadEncryptedBinFile (filename, decryptedData))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "DeSerializeSResult: <exit, failed to decrypt SResult bin file : \"" + filename + ">\"");
			return false;
		}

		MemoryStreamBuf dataBuf (reinterpret_cast<char*>(decryptedData.data()), decryptedData.size());
		std::istream dataStream(&dataBuf, std::ios::binary);

		// Read the sresult version.
		double version_info = 0;
		dataStream.read ((char*)&version_info, sizeof(version_info));

		sresult = {};

		if (version_info == SRESULT_VERSION)
		{
			if (!ReadParam(&dataStream, sresult.Processing_Settings))
			{
				Logger::L().Log (MODULENAME, severity_level::error, "Failed to read Processing_Settings");
				return false;
			}

			if (!ReadParam(&dataStream, sresult.Cumulative_results))
			{
				Logger::L().Log (MODULENAME, severity_level::error, "Failed to read Cumulative_results");
				return false;
			}

			if (!ReadParam(&dataStream, sresult.map_Image_Results))
			{
				Logger::L().Log (MODULENAME, severity_level::error, "Failed to read map_Image_Results");
				return false;
			}

			if (!ReadParam(&dataStream, sresult.map_Blobs_for_image))
			{
				Logger::L().Log (MODULENAME, severity_level::error, "Failed to read map_Blobs_for_image");
				return false;
			}

			if (!ReadParam(&dataStream, sresult.map_MaxNoOfPeaksFLChannel))
			{
				Logger::L().Log (MODULENAME, severity_level::error, "Failed to read map_MaxNoOfPeaksFLChannel");
				return false;
			}

			if (!ReadParam(&dataStream, sresult.map_v_stLargeClusterData))
			{
				Logger::L().Log (MODULENAME, severity_level::error, "Failed to read map_v_stLargeClusterData");
				return false;
			}

			if (!ReadParam(&dataStream, sresult.map_MaxNoOfPeaksFLChannel_Cumulative))
			{
				Logger::L().Log (MODULENAME, severity_level::error, "Failed to read map_MaxNoOfPeaksFLChannel_Cumulative");
				return false;
			}
		}
		else
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Invalid SResult version : ");
		}
	}
	catch (std::exception & e)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "SResult Bin file Deserialization Failed : " + std::string(e.what()));
		return false;
	}

	return true;
}
