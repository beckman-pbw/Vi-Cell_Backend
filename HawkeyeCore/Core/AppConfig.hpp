#pragma once

#include "Configuration.hpp"

using boost::property_tree::ptree;

class AppConfig {
public:
	AppConfig();
	t_opPTree init (const std::string& filename, bool force = false, bool retry = false);
	t_opPTree initwithtag (const std::string& filename, const std::string& rootTag, bool force = false);
	t_opPTree init (t_pPTree pRoot, const std::string& rootTag);
   bool write();
   bool write (t_pPTree pTree, std::string filename);
   void putChild (std::string tag, const boost::property_tree::ptree& ptree);
   t_opPTree findSection (const std::string& tags);
   t_opPTree findTagInSection (t_opPTree pTree, std::string section);
   bool setField  (t_opPTree pTree, const std::string& field, const std::string& value);
   void print();
   void print (ptree& tree, int16_t level);

   template <typename T_TYPE>
   static bool findField (t_opPTree pTree, const std::string& field, bool isRequired, T_TYPE defaultValue, T_TYPE& value) {

      if (isRequired && (*pTree).find(field) == (*pTree).not_found()) {
         //std::string str = "Error in configuration file \"" + filename_
         //      +"\" - Required field \"" + field
         //      + "\" in " + section_
         //      + " not found.";
         //Logger::L().Log ("AppConfig", severity_level::error, str);
         return false;
      }

      value = (*pTree).get<T_TYPE> (field, defaultValue);

      return true;
   }


private:
   struct my_tokenizer_func {
       template<typename It>
       bool operator()(It& next, It end, std::string & tok)
       {
           if (next == end) {
               return false;
           }
           char const * delimiter = "/";
           auto pos = std::search(next, end, delimiter, delimiter+1);
           tok.assign (next, pos);
           next = pos;
           if (next != end) {
               std::advance(next, 1);
           }
           return true;
       }

	   static void reset() {}
   };

   std::string filename_;
   t_pPTree pRoot_;
   t_opPTree rootTree_;
   std::string section_;
};
