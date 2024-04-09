#pragma once

#include <boost/bimap.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <memory>
#include <string>

typedef std::shared_ptr<boost::property_tree::ptree> t_pPTree;
typedef boost::optional<boost::property_tree::ptree&> t_opPTree;

enum class ShutdownOrRebootEnum : int16_t
{
	Shutdown = 0,
	Reboot = 1,
};

namespace ConfigUtils
{
	/*
	 * ec return values:
	 *		no_such_file_or_directory               empty filename / file does not exist / not a regular file
	 * 		invalid_argument                        unable to determine file type from file extension
	 * 		io_error                                unable to parse configuration file
	 */
	t_pPTree OpenConfigFile(const std::string& filename, boost::system::error_code& ec, bool cache=false);
	
    /*
     * Clears the filename from the collection of cached data and filenames.
     */
	void ClearCachedConfigFile (const std::string& filename);

	/*
	 * Force a cached config file to be reread.
	 *
	 * ec return values:
	 *		no_such_file_or_directory               empty filename / file does not exist / not a regular file
	 * 		invalid_argument                        cached value was not found, OR unable to determine file type from file extension
	 * 		io_error                                unable to parse configuration file
	 * NOTE: if an io_error or no_such_file_or_directory occurs, this will REMOVE
	 * the value from the cache!
	 * In this case, the input t_pPTree will not be modified.
	 * NOTE: This is NOT threadsafe.
	 */
	void rereadCachedConfigFile(t_pPTree& cfg, boost::system::error_code& ec);
	
	/*
	 * Write out a MODIFIED config file; the original config file MUST have been
	 * loaded into a t_pPTree using OpenConfigFile().
	 *
	 * ec return values:
	 *		invalid_argument                        cached value was not found
	 * 		io_error                                unable to write configuration file
	 * Note: invalid_argument could also be returned if the file type cannot be
	 * determined from the file extension, but in practice filenames with invalid
	 * filetypes are not cached anyway.
	 */
	void WriteCachedConfigFile(const std::string& filename, boost::system::error_code& ec);
	void WriteCachedConfigFile(t_pPTree& cfg, boost::system::error_code& ec);
	
	/* Write out an ARBITRARY config file.
	 * Arguments are ordered to match 'boost::property_tree::write_*()'.
	 *
	 * ec return values:
	 *		invalid_argument                        unable to determine file type from file extension
	 * 		io_error                                unable to write configuration file
	 */
	void WriteConfigFile (const std::string& filename, const t_pPTree& cfg, boost::system::error_code& ec, bool cache=false);
}

