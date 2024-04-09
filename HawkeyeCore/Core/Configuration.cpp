#include "stdafx.h"

#include "Configuration.hpp"
#include "FileSystemUtilities.hpp"
#include "HawkeyeDataAccess.h"
#include "HawkeyeDirectory.hpp"

#include <mutex>

namespace pt = boost::property_tree;
namespace fs = boost::filesystem;
namespace errc = boost::system::errc;
namespace sys = boost::system;

using PtFnameMap = boost::bimap<t_pPTree, std::string>;

static PtFnameMap& getCachedConfigs(void)
{
	static PtFnameMap INSTANCE;
	return INSTANCE;
}

static std::mutex& getCacheMutex(void)
{
	static std::mutex INSTANCE;
	return INSTANCE;
}

/*
 * ec return values:
 *		no_such_file_or_directory 		empty filename / file does not exist / not a regular file
 * 		invalid_argument				unable to determine file type from file extension 
 * 		io_error						unable to parse configuration file
 */
enum eConfigType
{
	ctXML,
	ctJSON,
	ctINFO,
	ctINI,
	ctINVALID,
};

static eConfigType CheckConfigType_(const fs::path& filepath)
{
	eConfigType ctype;
	auto fileExtn = fs::extension(filepath);

	if (fileExtn == ".json")
		ctype = ctJSON;
	else if (fileExtn == ".info")
		ctype = ctINFO;
	else if (fileExtn == ".ini")
		ctype = ctINI;
	else if (fileExtn == ".xml")
		ctype = ctXML;
	else
		ctype= ctINVALID;
	
	return ctype;
}

static std::unique_ptr<::pt::ptree> DoReadConfigFile_(const std::string& filename, boost::system::error_code& ec)
{
	if (filename.empty())
	{
		ec = ::errc::make_error_code(::errc::no_such_file_or_directory);
		return {};
	}

	fs::path p(filename);

	eConfigType ctype_ = CheckConfigType_(p);
	if (ctype_ == ctINVALID)
	{
		ec = ::errc::make_error_code(::errc::invalid_argument);
		return {};
	}

	// TODO : make changes to Hawkeye Data Access library function to accommodate different file extension.
	if (ctype_ == eConfigType::ctINFO)
	{
		if (!HDA_EncryptedFileExists(filename))
		 {
			ec = ::errc::make_error_code(::errc::no_such_file_or_directory);
			return{};
		}
	}
	else
	{
		if (!fs::exists(p) || !fs::is_regular_file(p))
		{
			ec = ::errc::make_error_code(::errc::no_such_file_or_directory);
			return{};
		}
	}

	auto pt_root_ = std::make_unique<::pt::ptree>();

	try
	{
		switch (ctype_)
		{
			case  eConfigType::ctXML:
				::pt::read_xml(filename, *pt_root_);
				break;
			case  eConfigType::ctJSON:
				::pt::read_json(filename, *pt_root_);
				break;
			case  eConfigType::ctINFO:
				if (!HDA_ReadEncryptedPtreeFile(filename, *pt_root_))
				 {
					ec = ::errc::make_error_code(::errc::io_error);
					pt_root_.reset();
				}
				break;
			case  eConfigType::ctINI:
				::pt::read_ini(filename, *pt_root_);
				break;
			default:
				// This was handled above.
				return {};
		}
	}
	catch (...)
	{
		pt_root_.reset();
		ec = ::errc::make_error_code(::errc::io_error );
	}
	
	return pt_root_;
}

t_pPTree ConfigUtils::OpenConfigFile(const std::string& filename, boost::system::error_code& ec, bool cache)
{
	std::lock_guard<std::mutex> m{getCacheMutex()};
	
	ec = ::errc::make_error_code(::errc::success);
	
	auto cached_iter = getCachedConfigs().right.find(filename);
	if (cached_iter != getCachedConfigs().right.end())
	{
		// ->second is the t_pPTree data found for the string key
		return cached_iter->second;
	}
	
	t_pPTree pt_root_ = DoReadConfigFile_(filename, ec);
	
	// Do not cache pt_root_ if there was an error, because there is no way to
	// cache the specific error type.
	if (cache && pt_root_)
	{
		getCachedConfigs().insert(PtFnameMap::value_type(pt_root_, filename));
	}
	
	return pt_root_;
}

void ConfigUtils::ClearCachedConfigFile (const std::string& filename)
{

	auto cached_iter = getCachedConfigs().right.find(filename);
	if (cached_iter != getCachedConfigs().right.end())
	{
		getCachedConfigs().right.erase (cached_iter);
	}
}

void ConfigUtils::rereadCachedConfigFile(t_pPTree& cfg, boost::system::error_code& ec)
{
	std::lock_guard<std::mutex> m{getCacheMutex()};
	
	auto cached_iter_fname = getCachedConfigs().left.find(cfg);
	if (cached_iter_fname == getCachedConfigs().left.end())
	{
		ec = ::errc::make_error_code(::errc::invalid_argument);
		return;
	}
	
	// ->second is the string data found for the t_pPTree key
	auto pt_reread_ = DoReadConfigFile_(cached_iter_fname->second, ec);
	
	if (pt_reread_)
	{
		// Update the caller's t_pPTree
		// NOTE: This MUST update the UNDERLYING PTree object, NOT just the
		// shared_ptr object, because otherwise any other shared pointer instances
		// pointing to the same PTree will not be updated.
		::pt::swap(*cfg, *pt_reread_);
	}
	else
	{
		// Remove the now-invalid cached value.
		getCachedConfigs().left.erase(cached_iter_fname);
	}
	
}

void ConfigUtils::WriteCachedConfigFile(const std::string& filename, boost::system::error_code& ec)
{
	std::lock_guard<std::mutex> m{getCacheMutex()};
	
	auto cached_iter_cfg = getCachedConfigs().right.find(filename);
	if (cached_iter_cfg == getCachedConfigs().right.end())
	{
		ec = ::errc::make_error_code(::errc::invalid_argument);
		return;
	}
	
	// ->second is the t_pPTree data found for the string key
	WriteConfigFile(filename, cached_iter_cfg->second, ec);
	
}

void ConfigUtils::WriteCachedConfigFile(t_pPTree& cfg, boost::system::error_code& ec)
{
	std::lock_guard<std::mutex> m{getCacheMutex()};
	
	auto cached_iter_fname = getCachedConfigs().left.find(cfg);
	if (cached_iter_fname == getCachedConfigs().left.end())
	{
		ec = ::errc::make_error_code(::errc::invalid_argument);
		return;
	}
	
	// ->second is the string data found for the t_pPTree key
	WriteConfigFile(cached_iter_fname->second, cfg, ec);
}

void ConfigUtils::WriteConfigFile(const std::string& filename, const t_pPTree& cfg, boost::system::error_code& ec, bool cache)
{
	ec = ::errc::make_error_code(::errc::success);
	try
	{
		switch (CheckConfigType_(fs::path(filename)))
		{
			case ctXML:
				::pt::write_xml(filename, *cfg);
				break;
			case ctJSON:
				::pt::write_json(filename, *cfg);
				break;
			case ctINFO:
				if (!HDA_WriteEncryptedPtreeFile(*cfg, filename))
				{
					ec = ::errc::make_error_code(::errc::io_error);
					cache = false; // To prevent invalid data caching 
				}
				break;
			case ctINI:
				::pt::write_ini(filename, *cfg);
				break;
			default:
				// We should not get here, because invalid file types should not be cached.
				assert(false);
				// ... but just in case, in release mode:
				ec = ::errc::make_error_code(::errc::invalid_argument);
				// If there was an error, don't cache.
				cache = false;
		}
	}
	catch (...)
	{
		ec = ::errc::make_error_code(::errc::io_error);
		// If there was an error, don't cache.
		cache = false;
	}
	
	if (cache)
	{
		getCachedConfigs().insert(PtFnameMap::value_type(cfg, filename));
	}
	
}
