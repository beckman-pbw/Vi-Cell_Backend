#include "stdafx.h"

#include "AppConfig.hpp"
#include "Logger.hpp"
#include "HawkeyeDataAccess.h"
using namespace boost::system;

static const char MODULENAME[]="AppConfig";
static t_opPTree treeNotFound_;

//*****************************************************************************
AppConfig::AppConfig () 
	: pRoot_(nullptr)
{
}

//*****************************************************************************
t_opPTree AppConfig::init (const std::string& filename, bool force, bool retry) {
	boost::system::error_code ec;

	pRoot_ = ConfigUtils::OpenConfigFile (filename, ec, true);
	if (ec == ::errc::no_such_file_or_directory)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "Configuration file \"" + filename + "\" does not exist or is not a regular file.");

		if (!force || retry)
			return treeNotFound_;

		Logger::L().Log (MODULENAME, severity_level::notification, "Forcing creation of configuration file \"" + filename + "\"");
	
		boost::property_tree::ptree cfg;
		cfg.add_child("config", boost::property_tree::ptree());
				
		if (!HDA_WriteEncryptedPtreeFile(cfg, filename))
			return treeNotFound_;
	
		return init(filename, force, true);

	}
	if (ec == ::errc::invalid_argument)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Configuration file \"" + filename + "\" type could not be determined.");
		return treeNotFound_;
	}
	if (ec == ::errc::io_error)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Unable to parse configuration file \"" + filename + "\".");
		return treeNotFound_;
	}

	filename_ = filename;
	rootTree_ = *pRoot_;
	return rootTree_;
}

//*****************************************************************************
t_opPTree AppConfig::initwithtag (const std::string& filename, const std::string& rootTag, bool force) {
	t_opPTree pConfig;

	t_opPTree pRoot = init  (filename, force);
	if (!pRoot)
	{
		return pConfig;
	}
	
	pConfig = findSection (rootTag);
	if (!pConfig) 
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "init: <\"" + rootTag + "\"> tag not found");
	}
	return pConfig;
}

//*****************************************************************************
t_opPTree AppConfig::init (t_pPTree pRoot, const std::string& rootTag) {
	t_opPTree pConfig;

	pRoot_ = pRoot;
	rootTree_ = *pRoot_;
	if (pRoot) {
		pConfig = findSection (rootTag);
		if (!pConfig) {
			Logger::L().Log (MODULENAME, severity_level::critical, "init: <exit, <config> tag not found>");
		}
	}

	return pConfig;
}

#include "CommandParser.hpp"
//*****************************************************************************
t_opPTree AppConfig::findSection (const std::string& tags) {

	std::vector<std::string> parsed_tags{};
	CommandParser::parse (".", tags, parsed_tags);

	t_opPTree pTree = *pRoot_;

	for (auto tag : parsed_tags) {
		pTree = findTagInSection (pTree, tag);
		if (!pTree) {
			return pTree;
		}
	}

	return pTree;
}

//*****************************************************************************
t_opPTree AppConfig::findTagInSection (t_opPTree pTree, const std::string section) {

	if (!rootTree_) {
		return treeNotFound_;
	}

	for (ptree::const_iterator it = pTree->begin(); it != pTree->end(); ++it) {
//std::cout << it->first << ": " << it->second.get_value<std::string>() << std::endl;
		if (it->first.compare(section) == 0) {
			section_ = section;
			return (*pTree).get_child(section);
		}
	}

	return treeNotFound_;
}

//*****************************************************************************
bool AppConfig::setField (t_opPTree pTree, const std::string& field, const std::string& value) {

   if ((*pTree).find(field) == (*pTree).not_found()) {
      (*pTree).add (field, value);
   } else {
      (*pTree).put (field, value);
   }

   return true;
}

//*****************************************************************************
void AppConfig::putChild (std::string tag, const boost::property_tree::ptree& ptree) {

	if (!tag.empty()) {
		pRoot_->put_child (tag, ptree);
	}
}

//*****************************************************************************
bool AppConfig::write() {

	boost::system::error_code ec;
	ConfigUtils::WriteConfigFile (filename_, pRoot_, ec, false);
	if (ec != boost::system::errc::success) {
		return false;
	}
	
	return true;
}

//*****************************************************************************
bool AppConfig::write (t_pPTree pTree, std::string filename) {

	filename_ = filename;

	boost::system::error_code ec;

	ConfigUtils::WriteConfigFile (filename, pTree, ec, false);
	if (ec != boost::system::errc::success) {
		return false;
	}

	return true;
}

//*****************************************************************************
void AppConfig::print() {

   print (*pRoot_, -1);

   std::cout << std::endl;
}

//*****************************************************************************
static std::string indent (int level) {
  std::string s;
  for (int i=0; i<level; i++) s += "   ";
  return s;
}

//*****************************************************************************
void AppConfig::print (ptree& tree, int16_t level) {


   if (tree.empty()) {
      std::cout << tree.data();

   } else {
      if (level >= 0) {
         std::cout << std::endl;
         std::cout << indent(level) << "{" << std::endl;
      }

      for (ptree::iterator pos = tree.begin(); pos != tree.end();) {
         std::cout << indent(level+1) << pos->first << " ";
		 print (pos->second, level+1);
         ++pos;
         std::cout << std::endl;
      }
      if (level != -1) {
         std::cout << indent(level) << "}";
      }
   }

}
